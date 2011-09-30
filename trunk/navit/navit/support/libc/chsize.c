/*
 * chsize.c: _chsize and chsize implementations for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public
 * Domain.  This file is a part of the mingw32ce package.  No
 * warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> 24 Jun 2007
 *
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int
_chsize (int fd, long size)
{
  DWORD cur;
  DWORD new;
  HANDLE h;
  BOOL ret;

  h = (HANDLE) fd;
  if (h == INVALID_HANDLE_VALUE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return -1;
    }

  cur = SetFilePointer (h, 0, NULL, FILE_CURRENT);
  if (cur == 0xffffffff)
    return -1;

  /* Move to where we want it.  */
  new = SetFilePointer (h, size, NULL, FILE_BEGIN);
  if (new == 0xffffffff)
    return -1;

  /* And commit it as eof, effectivelly growing or shrinking.  */
  ret = SetEndOfFile (h);

  SetFilePointer (h, cur, NULL, FILE_BEGIN);

  return ret ? 0 : -1;
}

int
chsize (int fd, long size)
{
  return _chsize (fd, size);
}
