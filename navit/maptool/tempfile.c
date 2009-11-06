#include <unistd.h>
#include "maptool.h"
#include "debug.h"

FILE *
tempfile(char *suffix, char *name, int mode)
{
	char buffer[4096];
	sprintf(buffer,"%s_%s.tmp",name, suffix);
	switch (mode) {
	case 0:
		return fopen(buffer, "rb");
	case 1:
		return fopen(buffer, "wb+");
	case 2:
		return fopen(buffer, "ab");
	default:
		return NULL;
	}
}

void
tempfile_unlink(char *suffix, char *name)
{
	char buffer[4096];
	sprintf(buffer,"%s_%s.tmp",name, suffix);
	unlink(buffer);
}

void
tempfile_rename(char *suffix, char *from, char *to)
{
	char buffer_from[4096],buffer_to[4096];
	sprintf(buffer_from,"%s_%s.tmp",from,suffix);
	sprintf(buffer_to,"%s_%s.tmp",to,suffix);
	dbg_assert(rename(buffer_from, buffer_to) == 0);
	
}
