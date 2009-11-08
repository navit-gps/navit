#include <unistd.h>
#include "maptool.h"
#include "debug.h"

char *
tempfile_name(char *suffix, char *name)
{
	return g_strdup_printf("%s_%s.tmp",name, suffix);
}
FILE *
tempfile(char *suffix, char *name, int mode)
{
	char *buffer=tempfile_name(suffix, name);
	FILE *ret=NULL;
	switch (mode) {
	case 0:
		ret=fopen(buffer, "rb");
		break;
	case 1:
		ret=fopen(buffer, "wb+");
		break;
	case 2:
		ret=fopen(buffer, "ab");
		break;
	}
	g_free(buffer);
	return ret;
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
