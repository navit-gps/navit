#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib.h>

int tst(void)
{
	unsigned short t1=20;
	unsigned short t2=10;
	return t2-t1;
}

int main(int argc, char **argv)
{
	printf("res=%d\n", tst());
}
