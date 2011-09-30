/*
 * lseek.c: lseek implementation for WinCE.
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

long
_lseek (int fildes, long offset, int whence)
{
  DWORD mode;
  switch (whence)
    {
    case SEEK_SET:
      mode = FILE_BEGIN;
      break;
    case SEEK_CUR:
      mode = FILE_CURRENT;
      break;
    case SEEK_END:
      mode = FILE_END;
      break;
    default:
      /* Specify an invalid mode so SetFilePointer catches it.  */
      mode = (DWORD)-1;
    }
  return (long) SetFilePointer ((HANDLE) fildes, offset, NULL, mode);
}

long
lseek (int fildes, long offset, int whence)
{
  return _lseek (fildes, offset, whence);
}
