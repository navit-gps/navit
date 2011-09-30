/*
 * stat.c: _stat, stat and fstat implementations for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> 10 Feb 2007
 *
 */

#include <windows.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <stdio.h>
#include "debug.h"

#include "timeutil.h"

struct stat_file_info_t
{
  DWORD dwFileAttributes;
  FILETIME ftLastWriteTime;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  DWORD nFileSizeLow;
};

#define COPY_MEMBER(DEST, SRC, MEMBER)		\
  do {						\
    (DEST)->MEMBER = (SRC)->MEMBER;		\
  } while (0)

#define TO_STAT_FILE_INFO(DEST, SRC)		\
  do {						\
    COPY_MEMBER (DEST, SRC, dwFileAttributes);	\
    COPY_MEMBER (DEST, SRC, ftLastWriteTime);	\
    COPY_MEMBER (DEST, SRC, ftCreationTime);	\
    COPY_MEMBER (DEST, SRC, ftLastAccessTime);	\
    COPY_MEMBER (DEST, SRC, nFileSizeLow);	\
  } while (0)

static int
__stat_by_file_info (struct stat_file_info_t *fi, struct _stat *st, int exec)
{
  int permission = _S_IREAD;

  memset (st, 0, sizeof (*st));

  st->st_size = fi->nFileSizeLow;
  st->st_mode = _S_IFREG;

  if((fi->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    st->st_mode = _S_IFDIR | _S_IEXEC;
  else if (exec)
    permission |= _S_IEXEC;

  if((fi->dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0)
    permission |= _S_IWRITE;

  st->st_mode |= permission | (permission >> 3) | (permission >> 6);

  st->st_nlink = 1; /* always 1? */
  st->st_rdev = 1; /* Where to get drive info?  */
  st->st_ino = 0; /* always 0 on Windows */

  st->st_mtime = __FILETIME_to_time_t (&fi->ftLastWriteTime);
  st->st_ctime = __FILETIME_to_time_t (&fi->ftCreationTime);
  st->st_atime = __FILETIME_to_time_t (&fi->ftLastAccessTime);

  /* Looks like the code below is never triggered.
     Windows CE always only keeps the LastWriteTime, and
     copies it to the CreationTime and LastAccessTime fields.  */
  if (st->st_ctime == 0)
    st->st_ctime = st->st_mtime;
  if (st->st_atime == 0)
    {
      /* On XP, at least, the st_atime is always set to the same as
	 the st_mtime, except the hour/min/sec == 00:00:00.  */
      SYSTEMTIME s;
      FILETIME f = fi->ftLastWriteTime;
      FileTimeToSystemTime (&f, &s);
      s.wHour = 0; s.wMinute = 0;
      s.wSecond = 0; s.wMilliseconds = 0;
      SystemTimeToFileTime (&s, &f);
      st->st_atime = __FILETIME_to_time_t (&f);
      /* st->st_atime = st->st_mtime; */
    }
  return 0;
}

int
_fstat (int fd, struct _stat *st)
{
  BY_HANDLE_FILE_INFORMATION fi;
  struct stat_file_info_t sfi;

  if (!GetFileInformationByHandle ((HANDLE) fd, &fi))
    return -1;
  TO_STAT_FILE_INFO (&sfi, &fi);
  return __stat_by_file_info (&sfi, st, 0);
}

int
fstat (int fd, struct stat *st)
{
  return _fstat (fd, (struct _stat *)st);
}

int
_stat (const char *path, struct _stat *st)
{
  WIN32_FIND_DATAW fd;
  HANDLE h;
  int ret;
  struct stat_file_info_t sfi;
  wchar_t pathw[MAX_PATH + 1];
  size_t len;
  int exec;

	dbg(0,"path=%s\n",path);
  mbstowcs (pathw, path, MAX_PATH);
	dbg(0,"wide path=%S\n",pathw);
  if((h = FindFirstFileW (pathw, &fd)) == INVALID_HANDLE_VALUE)
    {
      DWORD dwError = GetLastError ();
	dbg(0,"no file\n");
      if(dwError == ERROR_NO_MORE_FILES)
	/* Convert error to something more sensible.  */
	SetLastError (ERROR_FILE_NOT_FOUND);
      return -1;
    }

  TO_STAT_FILE_INFO (&sfi, &fd);

  len = strlen (path);
  exec = (len >= 4
	  && strcasecmp (path + len - 4, ".exe") == 0);
  ret = __stat_by_file_info (&sfi, st, exec);
	dbg(0,"ret=%d\n",ret);
  FindClose (h);
  return ret;
}

int
stat (const char *path, struct stat *st)
{
  return _stat (path, (struct _stat *)st);
}
