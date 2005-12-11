#include <ctype.h>
#include "util.h"

void
strtoupper(char *dest, const char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(char *dest, const char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}

