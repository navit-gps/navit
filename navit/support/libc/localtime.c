/*
 * localtime.c: localtime implementation for WinCE.
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
localtime(const time_t *timer)
{
  SYSTEMTIME ss, ls, s;
  FILETIME sf, lf, f;
  long long t, diff;

  static struct tm tms;

  GetSystemTime (&ss);
  GetLocalTime (&ls);

  SystemTimeToFileTime (&ss, &sf);
  SystemTimeToFileTime (&ls, &lf);

  diff = __FILETIME_to_ll (&sf) - __FILETIME_to_ll (&lf);

  __time_t_to_FILETIME (*timer, &f);
  t = __FILETIME_to_ll (&f) - diff;
  __ll_to_FILETIME (t, &f);
  FileTimeToSystemTime (&f, &s);
  __SYSTEMTIME_to_tm (&s, &tms);

  return &tms;
}
