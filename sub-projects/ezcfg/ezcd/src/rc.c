/* ============================================================================
 * Project Name : ezbox Configuration Daemon
 * Module Name  : rc.c
 *
 * Description  : ezbox rc program
 *
 * Copyright (C) 2008-2011 by ezbox-project
 *
 * History      Rev       Description
 * 2010-11-02   0.1       Write it from scratch
 * ============================================================================
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "ezcd.h"

#if 0
#define DBG(format, args...) do {\
	FILE *fp = fopen("/dev/kmsg", "a"); \
	if (fp) { \
		fprintf(fp, format, ## args); \
		fclose(fp); \
	} \
} while(0)
#else
#define DBG(format, args...)
#endif

#define RCSO_PATH_PREFIX "/lib/rc"
#define RCSO_PATH_PREFIX2 "/usr/lib/rc"

int rc_main(int argc, char **argv)
{
	bool b_sleep = false;
	struct timespec req;
	int fd = -1;
	pid_t pid;
	int ret = EXIT_FAILURE;
	int key, semid;
	struct sembuf require_res, release_res;
	char *p;
	char path[128];
	char name[32];
	void *handle;
	int (*function)(char *);

	/* only accept two/three arguments */
	/* argv[1] : action */
	/* argv[2] : arguments */
	/* argv[3] : sleep seconds */
	if ((argc != 3) && (argc != 4))
		return (EXIT_FAILURE);

	if (argc == 4) {
		req.tv_sec = strtol(argv[3], &p, 10);
		if ((p != NULL) && (*p == '.')) {
			p++;
			req.tv_nsec = strtol(p, (char **) NULL, 10);
		}
		if ((req.tv_sec > 0) || ((req.tv_sec == 0) && (req.tv_nsec > 0))) {
			b_sleep = true;
		}
	}

	if (getuid() != 0) {
		exit(EXIT_FAILURE);
	}

	/* set umask before creating any file/directory */
	ret = chdir("/");
	umask(0022);

	/* before opening new files, make sure std{in,out,err} fds are in a same state */
	fd = open("/dev/null", O_RDWR);
	if (fd < 0) {
		exit(EXIT_FAILURE);
	}
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	if (fd > STDERR_FILENO)
		close(fd);

	/* daemonize */
	pid = fork();
	switch (pid) {
	case 0:
		/* child process */
		ret = EXIT_SUCCESS;
		break;

	case -1:
		/* error */
		return (EXIT_FAILURE);

	default:
		/* parant process */
		DBG("<6>rc: child pid = [%d]\n", pid);
		return (EXIT_SUCCESS);
	}

	/* child process main */
	snprintf(name, sizeof(name), "rc_%s", argv[1]);
	snprintf(path, sizeof(path), "%s/%s.so", RCSO_PATH_PREFIX, name);
	handle = dlopen(path, RTLD_NOW);
	if (!handle) {
		DBG("<6>rc: dlopen(%s) error %s\n", path, dlerror());
		snprintf(path, sizeof(path), "%s/%s.so", RCSO_PATH_PREFIX2, name);
		handle = dlopen(path, RTLD_NOW);
		if (!handle) {
			DBG("<6>rc: dlopen(%s) error %s\n", path, dlerror());
			return (EXIT_FAILURE);
		}
	}

	/* clear any existing error */
	dlerror();

	*(void **) (&function) = dlsym(handle, name);

	if ((p = dlerror()) != NULL)  {
		DBG("<6>rc: dlsym error %s\n", p);
		goto rc_exit;
	}

	/* prepare semaphore */
	key = ftok(EZCFG_SEM_EZCFG_PATH, EZCFG_SEM_PROJID_EZCFG);
	if (key == -1) {
		DBG("<6>rc:pid=[%d] ftok error.\n", getpid());
		goto rc_exit;
	}

	/* create a semaphore set that only includes one semaphore */
	/* rc semaphore has been initialized in ezcd */
	semid = semget(key, EZCFG_SEM_NUMBER, 00666);
	if (semid < 0) {
		DBG("<6>rc: semget error\n");
		goto rc_exit;
	}

	/* now require available resource */
	require_res.sem_num = EZCFG_SEM_RC_INDEX;
	require_res.sem_op = -1;
	require_res.sem_flg = 0;

	if (semop(semid, &require_res, 1) == -1) {
		DBG("<6>rc: semop require_res error\n");
		goto rc_exit;
	}

	/* handle rc operations */
	/* wait s seconds */
	if (b_sleep == true)
		nanosleep(&req, NULL);

	ret = function(argv[2]);

	/* now release resource */
	release_res.sem_num = EZCFG_SEM_RC_INDEX;
	release_res.sem_op = 1;
	release_res.sem_flg = 0;

	if (semop(semid, &release_res, 1) == -1) {
		DBG("<6>rc: semop release_res error\n");
		goto rc_exit;
	}

rc_exit:
	/* close loader handle */
	dlclose(handle);

	/* special actions for rc_system stop/restart */
#if 0
	if (strcmp("system", argv[1]) == 0) {
		if (strcmp("stop", argv[2]) == 0) {
			if (utils_ezcd_wait_down(0) == true) {
				DBG("<6>rc: system stop.\n");
				/* { SIGUSR1, SIGUSR2, SIGTERM } for ezbox init/halt, poweroff, reboot */
				ret = kill(1, SIGUSR2);
			}
		}
		else if (strcmp("restart", argv[2]) == 0) {
			if (utils_ezcd_wait_down(0) == true) {
				DBG("<6>rc: system restart.\n");
				/* { SIGUSR1, SIGUSR2, SIGTERM } for ezbox init/halt, poweroff, reboot */
				ret = kill(1, SIGTERM);
			}
		}
	}
#endif

	return (ret);
}
