/* ============================================================================
 * Project Name : ezbox configuration utilities
 * File Name    : libezcfg-master.c
 *
 * Description  : interface to configurate ezbox information
 *
 * Copyright (C) 2010 by ezbox-project
 *
 * History      Rev       Description
 * 2010-07-12   0.1       Write it from scratch
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stddef.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/un.h>
#include <assert.h>
#include <pthread.h>

#include "libezcfg.h"
#include "libezcfg-private.h"

/*
 * ezcfg_master:
 *
 * Opaque object handling one event source.
 * Multi-threads model - master part.
 */
struct ezcfg_master {
	struct ezcfg *ezcfg;
	int stop_flag; /* Should we stop event loop */
	char *nvram; /* Non-volatile memory */
	int threads_max; /* MAX number of threads */
	int num_threads; /* Number of threads */
	int num_idle; /* Number of idle threads */

	pthread_mutex_t mutex; /* Protects (max|num)_threads */
	pthread_rwlock_t rwlock; /* Protects options, callbacks */
	pthread_cond_t thr_cond; /* Condvar for thread sync */

	struct ezcfg_socket *listening_sockets;
	struct ezcfg_socket *queue; /* Accepted sockets */
	int sq_len; /* Length of the socket queue */
	int sq_head; /* Head of the socket queue */
	int sq_tail; /* Tail of the socket queue */
	pthread_cond_t empty_cond; /* Socket queue empty condvar */
	pthread_cond_t full_cond;  /* Socket queue full condvar */
};

static void ezcfg_master_delete(struct ezcfg_master *master)
{
	if (master == NULL)
		return;
	if (master->nvram) {
		free(master->nvram);
	}
	if (master->queue) {
		free(master->queue);
	}
	ezcfg_socket_list_delete(master->listening_sockets);
	free(master);
}

/**
 * ezcfg_master_new:
 *
 * Create ezcfg master.
 *
 * Returns: a new ezcfg master
 **/
static struct ezcfg_master *ezcfg_master_new(struct ezcfg *ezcfg)
{
	struct ezcfg_master *master = NULL;

	master = calloc(1, sizeof(struct ezcfg_master));
	if (master) {
		/* initialize ezcfg library context */
		memset(master, 0, sizeof(struct ezcfg_master));
		master->sq_len = EZCFG_MASTER_SOCKET_QUEUE_LENGTH;

		master->nvram = calloc(EZCFG_NVRAM_SPACE, sizeof(char));
		if(master->nvram == NULL) {
			err(ezcfg, "calloc nvram.");
			goto fail_exit;
		}

		/* initialize nvram */
		memset(master->nvram, 0, EZCFG_NVRAM_SPACE);

		/* initialize socket queue */
		master->queue = ezcfg_socket_calloc(ezcfg, master->sq_len);
		if(master->queue == NULL) {
			err(ezcfg, "calloc socket queue.");
			goto fail_exit;
		}

		/*
		 * ignore SIGPIPE signal, so if client cancels the request, it
		 * won't kill the whole process.
		 */
		signal(SIGPIPE, SIG_IGN);

		/* initialize thread mutex */
		pthread_rwlock_init(&master->rwlock, NULL);
		pthread_mutex_init(&master->mutex, NULL);
		pthread_cond_init(&master->thr_cond, NULL);
		pthread_cond_init(&master->empty_cond, NULL);
		pthread_cond_init(&master->full_cond, NULL);

		/* set ezcfg library context */
		master->ezcfg = ezcfg;

		return master;
	}
fail_exit:
	ezcfg_master_delete(master);
	return NULL;
}

/**
 * ezcfg_master_add_socket:
 * @ezcfg: ezcfg master context
 * @socket_path: unix socket path
 *
 * Add a specified socket to the master thread. The
 * path to a socket either points to an existing socket file, or if
 * the socket path starts with a '@' character, an abstract namespace
 * socket will be used.
 *
 * A socket file will not be created. If it does not already exist,
 * it will fall-back and connect to an abstract namespace socket with
 * the given path. The permissions adjustment of a socket file, as
 * well as the later cleanup, needs to be done by the caller.
 *
 * Returns: socket, or NULL, in case of an error
 **/
static struct ezcfg_socket *ezcfg_master_add_socket(struct ezcfg_master *master, int family, const char *socket_path)
{
	struct ezcfg_socket *listener = NULL;

	if (master == NULL)
		return NULL;
	if (socket_path == NULL)
		return NULL;

	/* initialize unix domain socket */
	if ((listener = ezcfg_socket_new(master->ezcfg, family, socket_path)) == NULL) 
		return NULL;

	if (ezcfg_socket_list_insert(&(master->listening_sockets), listener) == -1) {
		err(master->ezcfg, "ezcfg_socket_list_insert fail");
		goto fail_exit;
	}
	return listener;
fail_exit:
	if (listener != NULL)
		ezcfg_socket_delete(listener);
	return NULL;
}

/**
 * ezcfg_master_new_from_socket:
 * @ezcfg: ezcfg library context
 * @socket_path: unix socket path
 *
 * Create new ezcfg master and connect to a specified socket. The
 * path to a socket either points to an existing socket file, or if
 * the socket path starts with a '@' character, an abstract namespace
 * socket will be used.
 *
 * A socket file will not be created. If it does not already exist,
 * it will fall-back and connect to an abstract namespace socket with
 * the given path. The permissions adjustment of a socket file, as
 * well as the later cleanup, needs to be done by the caller.
 *
 * Returns: a new ezcfg master, or #NULL, in case of an error
 **/
static struct ezcfg_master *ezcfg_master_new_from_socket(struct ezcfg *ezcfg, const char *socket_path)
{
	struct ezcfg_master *master = NULL;
	struct ezcfg_socket *sp = NULL;

	if (ezcfg == NULL)
		return NULL;
	if (socket_path == NULL)
		return NULL;

	master = ezcfg_master_new(ezcfg);
	if (master == NULL) {
		err(ezcfg, "new master fail: %m\n");
		return NULL;
	}

	sp = ezcfg_master_add_socket(master, AF_LOCAL, socket_path);
	if (sp == NULL) {
		err(ezcfg, "add socket [%s] fail: %m\n", socket_path);
		goto fail_exit;
	}

	if (ezcfg_socket_enable_receiving(sp) < 0) {
		err(ezcfg, "enable socket [%s] receiving fail: %m\n", socket_path);
		goto fail_exit;
	}
fail_exit:
	/* don't delete sp, ezcfg_master_delete will do it! */
	ezcfg_master_delete(master);

	return master;
}

/*
 * Deallocate ezcfg master context, free up the resources
 */
static void ezcfg_master_finish(struct ezcfg_master *master)
{
	//close_all_listening_sockets(master);

	/* Wait until all threads finish */
	pthread_mutex_lock(&master->mutex);
	while (master->num_threads > 0)
		pthread_cond_wait(&master->thr_cond, &master->mutex);
	master->threads_max = 0;
	pthread_mutex_unlock(&master->mutex);

	pthread_rwlock_destroy(&master->rwlock);
	pthread_mutex_destroy(&master->mutex);
	pthread_cond_destroy(&master->thr_cond);
	pthread_cond_destroy(&master->empty_cond);
	pthread_cond_destroy(&master->full_cond);

        /* signal ezcd_stop() that we're done */
        master->stop_flag = 2;
}

static void add_to_set(int fd, fd_set *set, int *max_fd)
{
	FD_SET(fd, set);
	if (fd > *max_fd)
	{
		*max_fd = (int) fd;
	}
}

// Master thread adds accepted socket to a queue
static void put_socket(struct ezcfg_master *master, const struct ezcfg_socket *sp)
{
	int stacksize = sizeof(struct ezcfg_master) * 2;
	
	pthread_mutex_lock(&master->mutex);

	// If the queue is full, wait
	while (master->sq_head - master->sq_tail >= master->sq_len) {
		pthread_cond_wait(&master->full_cond, &master->mutex);
	}
	assert(master->sq_head - master->sq_tail < master->sq_len);

	// Copy socket to the queue and increment head
	ezcfg_socket_queue_set_socket(master->queue, master->sq_head % master->sq_len, sp);
	master->sq_head++;
	info(master->ezcfg, "%s: queued socket %d", __func__, ezcfg_socket_get_sock(sp));

	// If there are no idle threads, start one
	if (master->num_idle == 0 && master->num_threads < master->threads_max) {
		struct ezcfg_worker *worker;
		worker = ezcfg_worker_new(master);
		if (worker) {
			if (ezcfg_thread_start(master->ezcfg, stacksize, (ezcfg_thread_func_t) ezcfg_worker_thread, worker) != 0) {
				err(master->ezcfg, "Cannot start thread: %d", errno);
			} else {
				master->num_threads++;
			}
		} else {
			err(master->ezcfg, "Cannot prepare worker thread: %d", errno);
		}
	}

	pthread_cond_signal(&master->empty_cond);
	pthread_mutex_unlock(&master->mutex);
}

static void accept_new_connection(const struct ezcfg_socket *listener,
                                  struct ezcfg_master *master) {
	struct ezcfg_socket *accepted;
	bool allowed;

	accepted = ezcfg_socket_new_accepted_socket(listener);
	if (accepted == NULL)
		return;

	//allowed = check_acl(ctx, &accepted.rsa) == MG_SUCCESS;
	allowed = true;

	if (allowed == true) {
		// Put accepted socket structure into the queue
		info(master->ezcfg, "%s: accepted socket %d",
		     __func__, ezcfg_socket_get_sock(accepted));
		put_socket(master, accepted);
	} else {
		info(master->ezcfg, "%s: %s is not allowed to connect",
		     __func__, ezcfg_socket_get_remote_socket_path(accepted));
		ezcfg_socket_close_sock(accepted);
	}
	free(accepted);
}

void ezcfg_master_thread(struct ezcfg_master *master) 
{
	fd_set read_set;
	struct ezcfg_socket *sp;
	struct timeval tv;
	int max_fd;

	while (master->stop_flag == 0) {
		FD_ZERO(&read_set);
		max_fd = -1;

		/* Add listening sockets to the read set */
		for (sp = master->listening_sockets; sp != NULL; sp = ezcfg_socket_list_next(&sp)) {
			add_to_set(ezcfg_socket_get_sock(sp), &read_set, &max_fd);
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if (select(max_fd + 1, &read_set, NULL, NULL, &tv) < 0) {
			/* do nothing */
			do {} while(0);
		} else {
			for (sp = master->listening_sockets; sp != NULL; sp = ezcfg_socket_list_next(&sp))
				if (FD_ISSET(ezcfg_socket_get_sock(sp), &read_set))
					accept_new_connection(sp, master);
		}
	}

	/* Stop signal received: somebody called ezcfg_stop. Quit. */
	ezcfg_master_finish(master);
}

struct ezcfg_master *ezcfg_master_start(struct ezcfg *ezcfg)
{
	struct ezcfg_master *master = NULL;
	int stacksize = sizeof(struct ezcfg_master) * 2;
	struct ezcfg_socket * sp;

	if (ezcfg == NULL)
		return NULL;

	/* There must be a ctrl socket */
	master = ezcfg_master_new_from_socket(ezcfg, EZCFG_CTRL_SOCK_PATH);
	if (master == NULL)
		return NULL;

	sp = ezcfg_master_add_socket(master, AF_LOCAL, EZCFG_MASTER_SOCK_PATH);
	if (sp == NULL) {
		err(ezcfg, "can not add master socket");
	}

	/* Start master (listening) thread */
	ezcfg_thread_start(ezcfg, stacksize, (ezcfg_thread_func_t) ezcfg_master_thread, master);
	return master;
}

void ezcfg_master_stop(struct ezcfg_master *master)
{
	if (master == NULL)
		return;

	master->stop_flag = 1;

	/* Wait until ezcfg_master_finish() stops */
	while (master->stop_flag != 2)
		sleep(1);

	assert(master->num_threads == 0);
	ezcfg_master_delete(master);
}

void ezcfg_master_set_threads_max(struct ezcfg_master *master, int threads_max)
{
	if (master == NULL)
		return;
	master->threads_max = threads_max;
}

struct ezcfg *ezcfg_master_get_ezcfg(struct ezcfg_master *master)
{
	return master->ezcfg;
}

bool ezcfg_master_is_stop(struct ezcfg_master *master)
{
	return (master->stop_flag == 0);
}

bool ezcfg_master_get_socket(struct ezcfg_master *master, struct ezcfg_socket *sp)
{
	return true;
}

void ezcfg_master_stop_worker(struct ezcfg_master *master)
{
	pthread_mutex_lock(&master->mutex);
	master->num_threads--;
	master->num_idle--;
	pthread_cond_signal(&master->thr_cond);
	assert(master->num_threads >= 0);
	pthread_mutex_unlock(&master->mutex);
}
