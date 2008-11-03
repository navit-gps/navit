#include "locale.h"
int errno;

char *
getenv(char *name)
{
	return 0;
}

void
setenv(void)
{
}

char *
getcwd(void)
{
	return "dummy";
}

char *
getwd(void)
{
	return "dummy";
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
	return strtok(str, delim);
}

void
perror(char *x)
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

void
alarm(void)
{
}
