/*
 * rename.c: rename implementation for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> Feb 2007
 *
 */

#include <windows.h>

/* We return an error if the TO file already exists, 
   like the Windows NT/9x 'rename' does.  */

int
rename (const char* from, const char* to)
{
  wchar_t fromw[MAX_PATH + 1];
  wchar_t tow[MAX_PATH + 1];
  mbstowcs (fromw, from, MAX_PATH);
  mbstowcs (tow, to, MAX_PATH);
  if (MoveFileW (fromw, tow))
    return 0;
  return -1;
}
