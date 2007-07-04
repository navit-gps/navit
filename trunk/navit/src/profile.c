#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "profile.h"

void
profile_timer(int level, const char *module, const char *function, const char *fmt, ...)
{
	va_list ap;
	static struct timeval last[3];
	struct timeval curr;
	int msec,usec;

	if (level < 0)
		level=0;
	if (level > 2)
		level=2;
	if (fmt) {
		gettimeofday(&curr, NULL);
		msec=(curr.tv_usec-last[level].tv_usec)/1000+
		     (curr.tv_sec-last[level].tv_sec)*1000;
		printf("%s:%s ", module, function);
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
		if (msec >= 100) 
			printf(" %d msec\n", msec);
		else {
			usec=(curr.tv_usec-last[level].tv_usec)+(curr.tv_sec-last[level].tv_sec)*1000*1000;
			printf(" %d.%d msec\n", usec/1000, usec%1000);
		}
		gettimeofday(&last[level], NULL);
	} else {
		gettimeofday(&curr, NULL);
		for (level = 0 ; level < 3 ; level++) 
			last[level]=curr;
	}
}
