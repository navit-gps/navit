/*
 * unlink.c: unlink and _unlink implementations for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> Feb 2007
 *
 */

#include <stdio.h>
#include <windows.h>

int
_unlink (const char *file)
{
  wchar_t wfile[MAX_PATH];
  size_t conv = mbstowcs (wfile, file, MAX_PATH);
  if (conv > 0 && conv < MAX_PATH && DeleteFileW (wfile))
    return 0;
  return -1;
}

int
unlink (const char *file)
{
  return _unlink (file);
}
