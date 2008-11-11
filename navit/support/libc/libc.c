#include "locale.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int errno;

#define MAXENV 32
static char *envnames[MAXENV];
static char *envvars[MAXENV];

static void cleanup_libc(void) __attribute__((destructor));
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
				val = strdup(value);
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
			envnames[i] = strdup(name);
			envvars[i] = strdup(value);
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
raise(void)
{
}

void *
popen(void)
{
	return 0;
}

void
pclose(void)
{
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

int
signal(void)
{
	return 0;
}

void
setlocale(void)
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
}
