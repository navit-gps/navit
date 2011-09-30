/*
 * read.c: read and _read implementations for WinCE.
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
_read (int fildes, void *buf, unsigned int bufsize)
{
  DWORD NumberOfBytesRead;
  if (bufsize > 0x7fffffff)
    bufsize = 0x7fffffff;
  if (!ReadFile ((HANDLE) fildes, buf, bufsize, &NumberOfBytesRead, NULL))
    return -1;
  return (int) NumberOfBytesRead;
}

int
read (int fildes, void *buf, unsigned int bufsize)
{
  return _read (fildes, buf, bufsize);
}
