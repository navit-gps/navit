#include "locale.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
int errno;

#define MAXENV 32
static char *envnames[MAXENV];
static char *envvars[MAXENV];

static void cleanup_libc(void) 
#ifndef _MSC_VER
__attribute__((destructor))
#endif
;
static void cleanup_libc(void)
{
	int i;
	for (i=0; i <MAXENV; i++) {
		if (envnames[i])
			free(envnames[i]);
		if (envvars[i])
			free(envvars[i]);
	}
}

char *
getenv(const char *name)
{
	int i;
	for (i=0; i < MAXENV; i++) {
		if (envnames[i] && !strcmp(envnames[i], name))
			return envvars[i];
	}
	return NULL;
}

int
setenv(const char *name, const char *value, int overwrite)
{
	int i;
	char *val;
	for (i=0; i < MAXENV; i++) {
		if (envnames[i] && !strcmp(envnames[i], name)) {
			if (overwrite) {
				val = g_strdup(value);
				if (!val)
					return -1;
				if (envvars[i])
					free(envvars[i]);
				envvars[i] = val;
			}
			return 0;
		}
	}
	for (i=0; i < MAXENV; i++) {
		if (!envnames[i]) {
			envnames[i] = g_strdup(name);
			envvars[i] = g_strdup(value);
			if (!envnames[i] || !envvars[i]) {
				if (envnames[i])
					free(envnames[i]);
				if (envvars[i])
					free(envvars[i]);
				envnames[i] = NULL;
				envvars[i] = NULL;
				return -1;
			}
			return 0;
		}
	}
	return -1;
}

int unsetenv(const char *name)
{
	int i;
	for (i=0; i < MAXENV; i++) {
		if (envnames[i] && !strcmp(envnames[i], name)) {
			free(envnames[i]);
			envnames[i] = NULL;
			if (envvars[i])
				free(envvars[i]);
			envvars[i] = NULL;
		}
	}
	return 0;
}

char *
getcwd(char *buf, int size)
{
	return "dummy";
}

char *
getwd(char *buf)
{
	return "dummy";
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
	return strtok(str, delim);
}

void
perror(const char *x)
{
}

void
raise(int signal)
{
}

void *
popen(void)
{
	return 0;
}

int pclose(FILE *stream)
{
	return 0;
}

void
rewind(void)
{
}

int
GetThreadLocale(void)
{
	return 0;
}

int signal(int signum, int handler)
{
	return 0;
}

char * setlocale ( int category, const char * locale )
{
	return 0;
}

static struct lconv localedata={"."};

struct lconv *
localeconv(void)
{
	return &localedata;
}

unsigned int
alarm(unsigned int seconds)
{
	return 0;
}

#ifdef _MSC_VER

size_t strftime (char *s, size_t maxsize, const char *format, const struct tm *tp)
{
    return 0;
}

__int64 _lseeki64(int FileHandle, __int64 Offset, int Origin)
{
    return 0;
}

intptr_t _get_osfhandle(int FileHandle)
{
    return 0;
}

#endif
