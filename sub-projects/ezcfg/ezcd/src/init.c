/* ============================================================================
 * Project Name : ezbox Configuration Daemon
 * Module Name  : init.c
 *
 * Description  : ezbox initramfs init program
 *
 * Copyright (C) 2008-2011 by ezbox-project
 *
 * History      Rev       Description
 * 2011-10-29   0.1       Write it from scratch
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
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/un.h>
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
#include <termios.h>
#include <sys/ioctl.h>

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

static void init_reap(int sig)
{
	pid_t pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		DBG("<6>init: reaped %d\n", pid);
	}
}

int init_main(int argc, char **argv)
{
	char *p;
	void *handle;
	union {
		rc_function_t func;
		void * obj;
	} alias;
	char *start_argv[] = { "action", "system_start", NULL };
	sigset_t sigset;

	/* unset umask */
	umask(0);

	/* make the command line just say "init"  - thats all, nothing else */
	strncpy(argv[0], "init", strlen(argv[0]));
	/* wipe argv[1]-argv[N] so they don't clutter the ps listing */
	while (*++argv)
		memset(*argv, 0, strlen(*argv));

	signal(SIGCHLD, init_reap);

	sigemptyset(&sigset);

	/* run start processes */
	handle = dlopen("/lib/rcso/rc_action.so", RTLD_NOW);
	if (!handle) {
		DBG("<6>init: dlopen(%s) error %s\n", "/lib/rcso/rc_action.so", dlerror());
		return (EXIT_FAILURE);
	}

	/* clear any existing error */
	dlerror();

	alias.obj = dlsym(handle, "rc_action");

	if ((p = dlerror()) != NULL)  {
		DBG("<6>init: dlsym error %s\n", p);
		dlclose(handle);
		return (EXIT_FAILURE);
	}

	alias.func(ARRAY_SIZE(start_argv) - 1, start_argv);

	/* close loader handle */
	dlclose(handle);

	/* run main loop forever */
	while (1) {
		sigsuspend(&sigset);
	}

	/* should never run to this place!!! */
	return (EXIT_FAILURE);
}