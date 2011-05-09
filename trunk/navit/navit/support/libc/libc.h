#ifndef _SUPPORT_LIBC_H
#define _SUPPORT_LIBC_H       1


void *popen(const char *command, const char *type);

int pclose(void *stream);

char*	getenv	(const char*);

int setenv(const char *name, const char *value, int overwrite);

void raise(int signal);

char * setlocale ( int category, const char * locale );

struct lconv *localeconv(void);

int signal(int signum, void *handler);

#ifdef _MSC_VER

#define GetWindowLongPtr        GetWindowLong
#define SetWindowLongPtr        SetWindowLong

#define DWLP_MSGRESULT  0
#define DWLP_DLGPROC    DWLP_MSGRESULT + sizeof(LRESULT)
#define DWLP_USER       DWLP_DLGPROC + sizeof(DLGPROC)

struct tm *localtime (const time_t *t);

size_t strftime (char *s, size_t maxsize, const char *format, const struct tm *tp);

#define mkdir       _mkdir
#define open        _open
#define close       _close
#define read        _read
#define write       _write
#define lseek       _lseek
#define vsnprintf   _vsnprintf

#else

#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

#endif


#endif
