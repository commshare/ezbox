/* ============================================================================
 * Project Name : ezbox Configuration Daemon
 * Module Name  : rc_init.c
 *
 * Description  : ezbox run init service
 *
 * Copyright (C) 2010 by ezbox-project
 *
 * History      Rev       Description
 * 2010-11-03   0.1       Write it from scratch
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

#include "ezcd.h"
#include "pop_func.h"

int rc_init(int flag)
{
	char *init_argv[] = { "/sbin/init", NULL };

	pop_etc_inittab(flag);

	switch (flag) {
	case RC_BOOT :
		/* run in root HOME path */
		chdir(ROOT_HOME_PATH);
		/* fall through */

	case RC_START :
		/* run init */
		execv(init_argv[0], init_argv);
		break;

	case RC_STOP :
		kill(1, SIGKILL);
		break;

	}
	return (EXIT_SUCCESS);
}