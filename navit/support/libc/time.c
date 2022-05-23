/*
 * time.c: time implementation for WinCE.
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
time (time_t *timer)
{
  SYSTEMTIME s;
  FILETIME f;
  time_t t;

  if (timer == NULL)
    timer = &t;

  GetSystemTime (&s);
  SystemTimeToFileTime (&s, &f);
  *timer = __FILETIME_to_time_t (&f);
  return *timer;
}
