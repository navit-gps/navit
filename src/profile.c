#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "profile.h"

void
profile_timer(char *where)
{
	static struct timeval last;
	struct timeval curr;
	int msec;

	if (where) {
		gettimeofday(&curr, NULL);
		msec=(curr.tv_usec-last.tv_usec)/1000+
		     (curr.tv_sec-last.tv_sec)*1000;
		printf("%s:%d msec\n", where, msec);
	}
	gettimeofday(&last, NULL);
}
