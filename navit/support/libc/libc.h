#ifndef _SUPPORT_LIBC_H
#define _SUPPORT_LIBC_H       1

#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

void *popen(const char *command, const char *type);

int pclose(void *stream);

char*	getenv	(const char*);

int setenv(const char *name, const char *value, int overwrite);

void raise(int signal);

char * setlocale ( int category, const char * locale );

struct lconv *localeconv(void);

int signal(int signum, void *handler);

#endif
