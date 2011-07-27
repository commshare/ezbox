/* ============================================================================
 * Project Name : ezbox Configuration Daemon
 * Module Name  : rc_emc2.c
 *
 * Description  : ezbox run EMC2 Enhanced Machine Controller service
 *
 * Copyright (C) 2008-2011 by ezbox-project
 *
 * History      Rev       Description
 * 2011-07-05   0.1       Write it from scratch
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
#include "rc_func.h"

#if 1
#define DBG(format, args...) do {\
	FILE *fp = fopen("/tmp/emc2.debug", "a"); \
	if (fp) { \
		fprintf(fp, format, ## args); \
		fclose(fp); \
	} \
} while(0)
#else
#define DBG(format, args...)
#endif

static int generate_latexit_sh(const char *path)
{
	FILE * file;
	file = fopen(path, "w");
	if (file == NULL) {
		return EXIT_FAILURE;
	}

	fprintf(file, "L=$((halcmd gets sj; halcmd gets bj;\n");
	fprintf(file, "  if [ -f $HOME/.latency ]; then cat $HOME/.latency; fi\n");
	fprintf(file, "  ) | sort -n | tail -1)\n");
	fprintf(file, "echo $L > $HOME/.latency\n");

	fclose(file);
	return EXIT_SUCCESS;

}

static int generate_test_hal_file(const char *path, long base_period, long servo_period)
{
	FILE * file;
	file = fopen(path, "w");
	if (file == NULL) {
		return EXIT_FAILURE;
	}

	fprintf(file, "loadrt threads name1=fast period1=%ld name2=slow period2=%ld\n", base_period, servo_period);
	fprintf(file, "loadrt timedelta count=2\n");
	fprintf(file, "addf timedelta.0 fast\n");
	fprintf(file, "addf timedelta.1 slow\n");
	fprintf(file, "start\n");
	fprintf(file, "loadusr -Wn lat pyvcp lat.xml\n");
	fprintf(file, "net sl timedelta.1.max => lat.sl\n");
	fprintf(file, "net sj timedelta.1.jitter => lat.sj\n");
	fprintf(file, "net st timedelta.1.out => lat.st\n");
	fprintf(file, "net bl timedelta.0.max => lat.bl\n");
	fprintf(file, "net bj timedelta.0.jitter => lat.bj\n");
	fprintf(file, "net bt timedelta.0.out => lat.bt\n");
	fprintf(file, "net reset lat.reset => timedelta.0.reset timedelta.1.reset\n");
	fprintf(file, "waitusr lat\n");
	fprintf(file, "loadusr -w sh latexit.sh\n");

	fclose(file);
	return EXIT_SUCCESS;
}

int rc_emc2_latency_test(int flag)
{
	int rc;
	char *p;
	char buf[64];
	char cmd[128];
	char ini_file[64];
	char ini_dir[64];
	char old_dir[64];
	char test_file[64];
	long base_period, servo_period;

	rc = nvram_match(NVRAM_SERVICE_OPTION(EMC2, LAT_TEST_ENABLE), "1");
	if (rc < 0) {
		return (EXIT_FAILURE);
	}

	switch (flag) {
	case RC_START :
		/* create configs dir */
		rc = ezcfg_api_nvram_get(NVRAM_SERVICE_OPTION(EMC2, INIFILE), ini_file, sizeof(ini_file));
		if (rc <= 0) {
			return (EXIT_FAILURE);
		}

		if (getcwd(old_dir, sizeof(old_dir)) == NULL) {
			snprintf(old_dir, sizeof(old_dir), "%s", ROOT_HOME_PATH);
		}
		strcpy(ini_dir, ini_file);
		p = strrchr(ini_dir, '/');
		if (p != NULL) {
			*p = '\0';
		}
		else {
			strcpy(ini_dir, old_dir);
		}

		snprintf(cmd, sizeof(cmd), "%s -p %s", CMD_MKDIR, ini_dir);
		DBG("huedbg: %s[%d] cmd=[%s]\n", __func__, __LINE__, cmd);
		system(cmd);

		pop_etc_emc2_rtapi_conf(RC_START);

		pop_etc_emc2_configs(RC_START);

		/* generate latexit.sh */
		snprintf(buf, sizeof(buf), "%s/latexit.sh", ini_dir);
		generate_latexit_sh(buf);

		/* get base period */
		rc = ezcfg_api_nvram_get(NVRAM_SERVICE_OPTION(EMC2, LAT_BASE_PERIOD), buf, sizeof(buf));
		if (rc <= 0) {
			return (EXIT_FAILURE);
		}
		base_period = atol(buf);

		/* get servo period */
		rc = ezcfg_api_nvram_get(NVRAM_SERVICE_OPTION(EMC2, LAT_SERVO_PERIOD), buf, sizeof(buf));
		if (rc <= 0) {
			return (EXIT_FAILURE);
		}
		servo_period = atol(buf);

		/* generate lat.hal */
		snprintf(test_file, sizeof(test_file), "%s/lat.hal", ini_dir);
		generate_test_hal_file(test_file, base_period, servo_period);

		/* export NML_FILE */
		rc = ezcfg_api_nvram_get(NVRAM_SERVICE_OPTION(EMC2, CONF_EMC_NML_FILE), buf, sizeof(buf));
		if (rc <= 0) {
			snprintf(buf, sizeof(buf), "/usr/share/emc/emc.nml");
		}
		setenv("NMLFILE", buf, 1);

		/* start EMC2 Enhanced Machine Controller latency test */
		/* change dir to ini_dir */
		chdir(ini_dir);

		/* Start REALTIME */
		rc_realtime(RC_START);

		/* sleep a second */
		sleep(1);

		/* Run latency test in background */
		/* $HALCMD -i "$INIFILE" -f $CFGFILE */
		snprintf(cmd, sizeof(cmd), "/usr/bin/halcmd -i %s -f %s", ini_file, test_file);
		DBG("huedbg: %s[%d] cmd=[%s]\n", __func__, __LINE__, cmd);
		system(cmd);

		/* restore to original dir */
		chdir(old_dir);

		break;

	case RC_STOP :
		/* Stop latency test in background */
		/* $HALCMD stop */
		snprintf(cmd, sizeof(cmd), "/usr/bin/halcmd stop");
		DBG("huedbg: %s[%d] cmd=[%s]\n", __func__, __LINE__, cmd);
		system(cmd);

		/* $HALCMD unload all */
		snprintf(cmd, sizeof(cmd), "/usr/bin/halcmd unload all");
		DBG("huedbg: %s[%d] cmd=[%s]\n", __func__, __LINE__, cmd);
		system(cmd);

		/* Stop REALTIME */
		rc_realtime(RC_STOP);
		break;

	case RC_RESTART :
		rc = rc_emc2_latency_test(RC_STOP);
		sleep(1);
		rc = rc_emc2_latency_test(RC_START);
		break;
	}
	return (EXIT_SUCCESS);
}