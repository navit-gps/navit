/*
 * close.c: close implementation for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> Feb 2007
 *
 */

#include <windows.h>
#include <unistd.h>

int
_close (int fildes)
{
  if (CloseHandle ((HANDLE) fildes))
    return 0;
  return -1;
}

int
close (int fildes)
{
  return _close (fildes);
}
