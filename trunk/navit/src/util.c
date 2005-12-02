#include <ctype.h>

void
strtoupper(unsigned char *dest, unsigned char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(unsigned char *dest, unsigned char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}

