/*
 * gmtime.c: gmtime implementation for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> Feb 2007
 *
 */

#include "timeutil.h"

struct tm *
gmtime(const time_t *t)
{
  FILETIME f;
  SYSTEMTIME s;
  static struct tm tms;
	
  __time_t_to_FILETIME (*t, &f);
  FileTimeToSystemTime (&f, &s);
  __SYSTEMTIME_to_tm (&s, &tms);
  return &tms;
}
