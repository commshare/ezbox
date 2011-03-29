/* ============================================================================
 * Project Name : ezbox Configuration Daemon
 * Module Name  : pop_etc_mtab.c
 *
 * Description  : ezbox /etc/mtab file generating program
 *
 * Copyright (C) 2010-2011 by ezbox-project
 *
 * History      Rev       Description
 * 2011-02-28   0.1       Write it from scratch
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

int pop_etc_mtab(int flag)
{
	char cmdline[64];
	snprintf(cmdline, sizeof(cmdline), "%s -rf /etc/mtab", CMD_RM);
	system(cmdline);
	symlink("/proc/mounts", "/etc/mtab");
	return (EXIT_SUCCESS);
}