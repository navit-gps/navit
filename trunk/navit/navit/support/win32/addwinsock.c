#include "addwinsock.h"

int inet_aton(const char *cp, struct in_addr *inp)
{
	unsigned long addr = inet_addr(cp);
	inp->S_un.S_addr = addr;
	return addr != -1;
}
