/* ============================================================================
 * Project Name : ezbox configuration utilities
 * File Name    : ssi/ssi.c
 *
 * Description  : interface to configurate ezbox information
 *
 * Copyright (C) 2008-2011 by ezbox-project
 *
 * History      Rev       Description
 * 2011-11-15   0.1       Write it from scratch
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>

#include <locale.h>
#include <libintl.h>

#include "ezcfg.h"
#include "ezcfg-private.h"

#if 0
#define DBG(format, args...) do { \
	pid_t pid; \
	char path[256]; \
	FILE *fp; \
	pid = getpid(); \
	snprintf(path, 256, "/tmp/%d-debug.txt", pid); \
	fp = fopen(path, "a"); \
	if (fp) { \
		fprintf(fp, format, ## args); \
		fclose(fp); \
	} \
} while(0)
#else
#define DBG(format, args...)
#endif

struct ssi_directive_entry {
	char *elem;
	char **attrs;
	int (*handler)(struct ezcfg_ssi *ssi, char *buf, size_t size);
};

struct ssi_directive_data {
	char *path;
	FILE *fp;
	bool need_unlink;
};

struct ezcfg_ssi {
	struct ezcfg *ezcfg;
	struct ezcfg_nvram *nvram;
	char *document_root;
	char *path;
	FILE *fp;
	char *remaining_data;
	struct ssi_directive_entry *directive_entry;
	char **directive_values;
	struct ssi_directive_data *directive_data;
};

#define SSI_STARTING_SEQUENCE     "<!--#"
#define SSI_STARTING_SEQUENCE_LEN 5
#define SSI_ENDING_SEQUENCE       "-->"
#define SSI_ENDING_SEQUENCE_LEN   3

#define SSI_ARGC_MAX              4

/**
 * functions declaration
 */

static int include_handler(struct ezcfg_ssi *ssi, char *buf, size_t size);
static int exec_handler(struct ezcfg_ssi *ssi, char *buf, size_t size);
static int echo_handler(struct ezcfg_ssi *ssi, char *buf, size_t size);

static char *include_attrs[] = { "virtual", NULL };
static char *exec_attrs[] = { "cmd", NULL };
static char *echo_attrs[] = { "var", "ns", NULL };

static struct ssi_directive_entry supported_directives[] = {
	{ "include", include_attrs, include_handler },
	{ "exec", exec_attrs, exec_handler },
	{ "echo", echo_attrs, echo_handler },
	{ NULL, NULL, NULL }
};

/**
 * Private functions
 **/
void remove_escape_backslash(char *s)
{
	char *p = s;
	while(*s != '\0') {
		if (*s == '\\' && *(s+1) == '"') s++;
		*p++ = *s++;
	}
	*p = '\0';
}

void delete_directive_values(char **values)
{
	int i;

	for (i = 0; values[i] != NULL; i++) {
		free(values[i]);
	}
	free(values);
}

void delete_directive_data(struct ssi_directive_data *data)
{
	if (data->fp != NULL) {
		fclose(data->fp);
	}
	if (data->path != NULL) {
		if (data->need_unlink == true) {
			unlink(data->path);
		}
		free(data->path);
	}
	free(data);
}

bool ssi_parse_directive_entry(struct ezcfg_ssi *ssi, char *buf)
{
	struct ssi_directive_entry *entry;
	char *s, *p;
	int i, len, argc;
	char *values_s[SSI_ARGC_MAX];
	char *values_e[SSI_ARGC_MAX];
	char **values = NULL;

	DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);

	/* find element */
	for (entry = supported_directives; entry->elem != NULL; entry++) {
		s = entry->elem;
		len = strlen(s);

		/* find out argc */
		for (argc = 0; entry->attrs[argc] != NULL; argc++)

		DBG("mydebug: %s-%s(%d) s=[%s]\n", __FILE__, __func__, __LINE__, s);
		DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);
		DBG("mydebug: %s-%s(%d) len=[%d]\n", __FILE__, __func__, __LINE__, len);
		DBG("mydebug: %s-%s(%d) argc=[%d]\n", __FILE__, __func__, __LINE__, argc);
		if (strncmp(s, buf, len) == 0 && buf[len] == ' ') {
			/* find match element */
			p = buf+len+1;
			DBG("mydebug: %s-%s(%d) p=[%s]\n", __FILE__, __func__, __LINE__, p);
			for (i = 0; i < argc; i++) {
				while (*p == ' ') p++;
				s = (entry->attrs)[i];
				len = strlen(s);
		DBG("mydebug: %s-%s(%d) s=[%s]\n", __FILE__, __func__, __LINE__, s);
		DBG("mydebug: %s-%s(%d) p=[%s]\n", __FILE__, __func__, __LINE__, p);
		DBG("mydebug: %s-%s(%d) len=[%d]\n", __FILE__, __func__, __LINE__, len);
				if (strncmp(s, p, len) != 0)
					break;
				if (p[len] != '=')
					break;
				if (p[len+1] != '"')
					break;
				s = p+len+2;
				/* try to find ending '"' */
				p = strchr(s, '"');
				while(p != NULL && *(p-1) == '\\')
					p = strchr(p+1, '"');
				if (p == NULL)
					break;
		DBG("mydebug: %s-%s(%d) s=[%s]\n", __FILE__, __func__, __LINE__, s);
		DBG("mydebug: %s-%s(%d) p=[%s]\n", __FILE__, __func__, __LINE__, p);
				values_s[i] = s; /* s point to charactor after '"' */
				values_e[i] = p; /* p point to '"' */
				p++;
			}
		DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			if (i == argc) {
				/* find match directive format */
				values = calloc(argc+1, sizeof(char *));
				if (values == NULL) {
		DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
					goto fail_exit;
				}
				/* copy values */
				for (i = 0; i < argc; i++) {
					s = values_s[i];
					p = values_e[i];
					len = p -s;
					values[i] = calloc(len+1, sizeof(char));
					if(values[i] == NULL) {
		DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
						goto fail_exit;
					}
					p = values[i];
					strncpy(p, s, len);
					*(p+len) = '\0';
					remove_escape_backslash(p);
				}
				/* NULL-terminated values */
				values[argc] = NULL;

		DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
				/* OK, finish parse */
				ssi->directive_entry = entry;
				ssi->directive_values = values;
				return true;
			}
		}
	}

fail_exit:
	if (values != NULL) {
		delete_directive_values(values);
	}
	return false;
}

static int include_handler(struct ezcfg_ssi *ssi, char *buf, size_t size)
{
	struct ssi_directive_data *data;

	data = ssi->directive_data;
	if (buf == NULL) {
		/* clean up directive data buffer */
		if (data != NULL) {
			delete_directive_data(data);
			ssi->directive_data = NULL;
		}
		return 0;
	}

	if (data == NULL) {
		data = calloc(1, sizeof(struct ssi_directive_data));
		if (data == NULL) {
			goto fail_exit;
		}
		data->path = NULL;
		data->fp = NULL;
		data->need_unlink = false;

		/* "virtual" value is in ssi->directive_values[0] */
		data->path = malloc(strlen(ssi->document_root) + strlen(ssi->directive_values[0]) + 1);
		if (data->path == NULL) {
			goto fail_exit;
		}
		sprintf(data->path, "%s%s", ssi->document_root, ssi->directive_values[0]);

		data->fp = fopen(data->path, "r");
		if (data->fp == NULL) {
			goto fail_exit;
		}
		ssi->directive_data = data;
	}

	/* OK, data is valid, should never run into fail_exit, or data will be freed !!! */
	if (fgets(buf, size, data->fp) == NULL) {
		if (feof(data->fp) != 0 && ferror(data->fp) == 0) {
			return 0;
		}
		else {
			return -1;
		}
	}

	return strlen(buf);

fail_exit:
	if (data != NULL) {
		delete_directive_data(data);
	}
	return -1;
}

static int exec_handler(struct ezcfg_ssi *ssi, char *buf, size_t size)
{
	struct ssi_directive_data *data;

	data = ssi->directive_data;
	if (buf == NULL) {
		/* clean up directive data buffer */
		if (data != NULL) {
			delete_directive_data(data);
			ssi->directive_data = NULL;
		}
		return 0;
	}

	return 0;
}

static int echo_handler(struct ezcfg_ssi *ssi, char *buf, size_t size)
{
	struct ssi_directive_data *data;

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	data = ssi->directive_data;
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	if (buf == NULL) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		/* clean up directive data buffer */
		if (data != NULL) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			delete_directive_data(data);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			ssi->directive_data = NULL;
		}
		return 0;
	}

	if (data == NULL) {
		char path[256];
		FILE *fp;

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		data = calloc(1, sizeof(struct ssi_directive_data));
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		if (data == NULL) {
			goto fail_exit;
		}
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		data->path = NULL;
		data->fp = NULL;
		data->need_unlink = false;

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		snprintf(path, sizeof(path), "/tmp/%d-%d-ssi-echo", getpid(), (int)time(NULL));

		/* "var" value is in ssi->directive_values[0] */
		/* "domain" value is in ssi->directive_values[1] */
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		data->path = malloc(strlen(path) + 1);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		if (data->path == NULL) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			goto fail_exit;
		}
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		strcpy(data->path, path);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		fp = fopen(path, "w");
		if (fp != NULL) {
			char *value;

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			if (strcmp(ssi->directive_values[1], "nvram") == 0) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	DBG("mydebug: %s-%s(%d) ssi->nvram=[%x]\n", __FILE__, __func__, __LINE__, (unsigned int)ssi->nvram);
	DBG("mydebug: %s-%s(%d) ssi->ssi->directive_values[0]=[%s]\n", __FILE__, __func__, __LINE__, ssi->directive_values[0]);
				if (ezcfg_nvram_get_entry_value(ssi->nvram, ssi->directive_values[0], &value) == false) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
					fclose(fp);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
					goto fail_exit;
				}
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
				fprintf(fp, "%s", value);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
				free(value);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			}

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			fclose(fp);
			data->need_unlink = true;
		}

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		data->fp = fopen(data->path, "r");
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		if (data->fp == NULL) {
			goto fail_exit;
		}
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		ssi->directive_data = data;
	}

	/* OK, data is valid, should never run into fail_exit, or data will be freed !!! */
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	if (fgets(buf, size, data->fp) == NULL) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		if (feof(data->fp) != 0 && ferror(data->fp) == 0) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			return 0;
		}
		else {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			return -1;
		}
	}

	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	return strlen(buf);

fail_exit:
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	if (data != NULL) {
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
		delete_directive_data(data);
	DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
	}
	return -1;
}

/**
 * Public functions
 **/
void ezcfg_ssi_delete(struct ezcfg_ssi *ssi)
{
	ASSERT(ssi != NULL);

	if (ssi->directive_data != NULL) {
		(ssi->directive_entry)->handler(ssi, NULL, 0);
	}
	if (ssi->directive_values != NULL) {
		delete_directive_values(ssi->directive_values);
	}
	if (ssi->remaining_data != NULL) {
		free(ssi->remaining_data);
	}
	if (ssi->fp != NULL) {
		fclose(ssi->fp);
	}
	if (ssi->path != NULL) {
		free(ssi->path);
	}
	if (ssi->document_root != NULL) {
		free(ssi->document_root);
	}

	free(ssi);
}

struct ezcfg_ssi *ezcfg_ssi_new(struct ezcfg *ezcfg, struct ezcfg_nvram *nvram)
{
	struct ezcfg_ssi *ssi;

	ASSERT(ezcfg != NULL);
	ASSERT(nvram != NULL);

	ssi = calloc(1, sizeof(struct ezcfg_ssi));
	if (ssi != NULL) {
		ssi->ezcfg = ezcfg;
		ssi->nvram = nvram;
		ssi->document_root = NULL;
		ssi->path = NULL;
		ssi->fp = NULL;
		ssi->remaining_data = NULL;
		ssi->directive_entry = NULL;
		ssi->directive_values = NULL;
		ssi->directive_data = NULL;
	}
	return ssi;
}

bool ezcfg_ssi_set_document_root(struct ezcfg_ssi *ssi, const char *root)
{
	char *p;

	ASSERT(ssi != NULL);
	ASSERT(root != NULL);

	p = strdup(root);
	if (p == NULL) {
		return false;
	}

	if (ssi->document_root != NULL) {
		free(ssi->document_root);
	}
	ssi->document_root = p;
	return true;
}

bool ezcfg_ssi_set_path(struct ezcfg_ssi *ssi, const char *path)
{
	char *p;

	ASSERT(ssi != NULL);
	ASSERT(path != NULL);

	p = strdup(path);
	if (p == NULL) {
		return false;
	}

	if (ssi->path != NULL) {
		free(ssi->path);
	}
	ssi->path = p;
	return true;
}

FILE *ezcfg_ssi_open_file(struct ezcfg_ssi *ssi, const char *mode)
{
	FILE *fp;
	char buf[256];

	ASSERT(ssi != NULL);

	if (ssi->document_root == NULL || ssi->path == NULL)
		return NULL;

	snprintf(buf, sizeof(buf), "%s%s", ssi->document_root, ssi->path);
	fp = fopen(buf, mode);
	if (fp == NULL)
		return NULL;

	if (ssi->fp != NULL) {
		fclose(ssi->fp);
	}
	ssi->fp = fp;
	return fp;
}

int ezcfg_ssi_file_get_line(struct ezcfg_ssi *ssi, char *buf, size_t size)
{
	struct ssi_directive_entry *entry;
	char *s, *e, *p;
	int len;
	int ret;

	entry = ssi->directive_entry;
	if (entry == NULL) {
		/* check if there is some remaining data should be handled */
		if (ssi->remaining_data == NULL) {
			len = 0;
		}
		else {
			len = strlen(ssi->remaining_data);
			if (len >= size) {
				/* Oops, buffer is too small */
				return -1;
			}
			strcpy(buf, ssi->remaining_data);
			free(ssi->remaining_data);
			ssi->remaining_data = NULL;
		}
		/* OK, it should read new line from SSI file */
		/* try to fill buffer full,
		 * so that the max SSI entry length is the buffer size
		 */
		if (fgets(buf+len, size-len, ssi->fp) == NULL) {
			return -1;
		}

		DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);
		/* search SSI starting sequence */
		s = strstr(buf, SSI_STARTING_SEQUENCE);
		if (s == NULL) {
			DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);
			return strlen(buf);
		}

		/* search SSI ending sequence */
		e = strstr(s+SSI_STARTING_SEQUENCE_LEN, SSI_ENDING_SEQUENCE);
		if (e == NULL) {
			/* no matching SSI ending sequence */
			if (len == 0) {
				/* It's first time to handle this SSI entry */
				ssi->remaining_data = strdup(s);
				if (ssi->remaining_data == NULL) {
					DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
					return -1;
				}
				/* cut off the buf and return first part */
				*s = '\0';
				DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);
				return strlen(buf);
			}
			else {
				/* Sorry, we can only handle buffer size length SSI entry */
				DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
				return -1;
			}
		}

		/* find an SSI diretive */
		/* first store the data after SSI directive entry */
		ssi->remaining_data = strdup(e+SSI_ENDING_SEQUENCE_LEN);
		if (ssi->remaining_data == NULL) {
			DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			return -1;
		}

		/* then construct SSI directive entry */
		p = s + SSI_STARTING_SEQUENCE_LEN;
		*e = '\0';
		DBG("mydebug: %s-%s(%d) p=[%s]\n", __FILE__, __func__, __LINE__, p);
		if (ssi_parse_directive_entry(ssi, p) == false) {
			DBG("mydebug: %s-%s(%d)\n", __FILE__, __func__, __LINE__);
			return -1;
		}

		/* cut off the buf and return first part */
		*s = '\0';
		DBG("mydebug: %s-%s(%d) buf=[%s]\n", __FILE__, __func__, __LINE__, buf);
		return strlen(buf);
	}
	else {
		ret = entry->handler(ssi, buf, size);
		DBG("mydebug: %s-%s(%d) ret=[%d]\n", __FILE__, __func__, __LINE__, ret);
		if (ret == 0) {
		DBG("mydebug: %s-%s(%d) ret=[%d]\n", __FILE__, __func__, __LINE__, ret);
			/* clean directive data buffer */
			entry->handler(ssi, NULL, 0);
			/* clean directive values buffer */
			if (ssi->directive_values != NULL) {
				delete_directive_values(ssi->directive_values);
				ssi->directive_values = NULL;
			}
			ssi->directive_entry = NULL;
		}
		return ret;
	}
}
