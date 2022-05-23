/*
 * open.c: open, _open and_wopen implementations for WinCE.
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
#include <fcntl.h>
#include <sys/stat.h>

static int
vwopen (const wchar_t *wpath, int oflag, va_list ap)
{
  DWORD fileaccess;
  DWORD fileshare;
  DWORD filecreate;
  DWORD fileattrib;
  HANDLE hnd;

  switch (oflag & (O_RDONLY | O_WRONLY | O_RDWR))
    {
    case O_RDONLY:
      fileaccess = GENERIC_READ;
      break;
    case O_WRONLY:
      fileaccess = GENERIC_WRITE;
      break;
    case O_RDWR:
      fileaccess = GENERIC_READ | GENERIC_WRITE;
      break;
    default:
      return -1;
    }

  switch (oflag & (O_CREAT | O_EXCL | O_TRUNC))
    {
    case 0:
    case O_EXCL:		/* ignore EXCL w/o CREAT */
      filecreate = OPEN_EXISTING;
      break;
    case O_CREAT:
      filecreate = OPEN_ALWAYS;
      break;
    case O_CREAT | O_EXCL:
    case O_CREAT | O_TRUNC | O_EXCL:
      filecreate = CREATE_NEW;
      break;

    case O_TRUNC:
    case O_TRUNC | O_EXCL:	/* ignore EXCL w/o CREAT */
      filecreate = TRUNCATE_EXISTING;
      break;
    case O_CREAT | O_TRUNC:
      filecreate = CREATE_ALWAYS;
      break;
    default:
      /* this can't happen ... all cases are covered */
      return -1;
    }

  fileshare = 0;
  if (oflag & O_CREAT)
    {
      int pmode = va_arg (ap, int);
      if ((pmode & S_IREAD) == S_IREAD)
	fileshare |= FILE_SHARE_READ;
      if ((pmode & S_IWRITE) == S_IWRITE)
	fileshare |= FILE_SHARE_WRITE;
    }

  fileattrib = FILE_ATTRIBUTE_NORMAL;

  hnd = CreateFileW (wpath, fileaccess, fileshare, NULL, filecreate,
		     fileattrib, NULL);
  if (hnd == INVALID_HANDLE_VALUE)
    return -1;

  if (oflag & O_APPEND)
    SetFilePointer (hnd, 0, NULL, FILE_END);

  return (int) hnd;
}

static int
vopen (const char *path, int oflag, va_list ap)
{
  wchar_t wpath[MAX_PATH];

  size_t path_len = strlen (path);
  if (path_len >= MAX_PATH)
    return -1;

  mbstowcs (wpath, path, path_len + 1);

  return vwopen (wpath, oflag, ap);
}

int
_open (const char *path, int oflag, ...)
{
  va_list ap;
  int ret;
  va_start (ap, oflag);
  ret = vopen (path, oflag, ap);
  va_end (ap);
  return ret;
}

int
open (const char *path, int oflag, ...)
{
  va_list ap;
  int ret;
  va_start (ap, oflag);
  ret = vopen (path, oflag, ap);
  va_end (ap);
  return ret;
}

int
_wopen (const wchar_t *path, int oflag, ...)
{
  va_list ap;
  int ret;
  va_start (ap, oflag);
  ret = vwopen (path, oflag, ap);
  va_end (ap);
  return ret;
}
