/*
 * mkdir.c: mkdir implementation for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public
 * Domain.  This file is a part of the mingw32ce package.  No
 * warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> 24 Jun 2007
 *
 */

#include <windows.h>
#include <io.h>

int
_mkdir (const char *dirname)
{
  wchar_t dirnamew[MAX_PATH];
  mbstowcs (dirnamew, dirname, MAX_PATH);
  if (!CreateDirectoryW (dirnamew, NULL))
    return -1;
  return 0;
}

int
mkdir (const char *dirname)
{
  return _mkdir (dirname);
}
