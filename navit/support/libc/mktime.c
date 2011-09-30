/*
 * mktime.c: mktime implementation for WinCE.
 *
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Written by Pedro Alves <pedro_alves@portugalmail.pt> Feb 2007
 *
 */

#include "timeutil.h"

time_t
mktime (struct tm *pt)
{
  SYSTEMTIME ss, ls, s;
  FILETIME sf, lf, f;
  long long diff;

  GetSystemTime (&ss);
  GetLocalTime (&ls);
  SystemTimeToFileTime (&ss, &sf);
  SystemTimeToFileTime (&ls, &lf);

  diff = __FILETIME_to_ll (&lf) - __FILETIME_to_ll (&sf);
  diff /= _onesec_in100ns;

  __tm_to_SYSTEMTIME (pt, &s);
  SystemTimeToFileTime (&s, &f);
  return __FILETIME_to_time_t (&f) - (time_t)diff;
}
