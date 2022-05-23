#include <windows.h>

/* Only needed on Windows CE. */

void abort(void)
{
  TerminateProcess (GetCurrentProcess(), 1);
  while (1); /* Kill 'noreturn function does return' warning.  */
}
