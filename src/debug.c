#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "file.h"
#include "debug.h"



static void sigsegv(int sig)
{
	FILE *f;
	time_t t;
	printf("segmentation fault received\n");
	f=fopen("crash.txt","a");
	setvbuf(f, NULL, _IONBF, 0);
	fprintf(f,"segmentation fault received\n");
	t=time(NULL);
	fprintf(f,"Time: %s", ctime(&t));
	file_unmap_all();
	fprintf(f,"dumping core\n");
	fclose(f);	
	abort();
}

void
debug_init(void)
{
	signal(SIGSEGV, sigsegv);
}
