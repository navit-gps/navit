/*
 * write.c: write and _write implementations for WinCE.
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
_write (int fildes, const void *buf, unsigned int bufsize)
{
  DWORD NumberOfBytesWritten;
  if (bufsize > 0x7fffffff)
    bufsize = 0x7fffffff;
  if (!WriteFile ((HANDLE) fildes, buf, bufsize, &NumberOfBytesWritten, NULL))
    return -1;
  return (int) NumberOfBytesWritten;
}

int
write (int fildes, const void *buf, unsigned int bufsize)
{
  return _write (fildes, buf, bufsize);
}
