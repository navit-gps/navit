#include "timeutil.h"

/* Originally based on code found in the Ruby WinCE support, which
   had the following notice: */

/* start original header */

/***************************************************************
  time.c

  author : uema2
  date   : Nov 30, 2002

  You can freely use, copy, modify, and redistribute
  the whole contents.
***************************************************************/

/* end original header */

/* Note: The original filename was: wince/time_wce.c  */

/* Adapted to mingw32ce by Pedro Alves  <pedro_alves@portugalmail.pt>
   10 Feb 2007 - Initial revision.  */

#define DELTA_EPOCH 116444736000000000LL

#if 0
how_to_calc_delta_epoch ()
{
  FILETIME f1601, f1970;
  __FILETIME_from_Year (1601, &f1601);
  __FILETIME_from_Year (1970, &f1970);
  DELTA_EPOCH = __FILETIME_to_ll (&f1970) - __FILETIME_to_ll (&f1601);
}
#endif

long long
__FILETIME_to_ll (const FILETIME *f)
{
  long long t;
  t = (long long)f->dwHighDateTime << 32;
  t |= f->dwLowDateTime;
  return t;
}

void
__ll_to_FILETIME (long long t, FILETIME* f)
{
  f->dwHighDateTime = (DWORD)((t >> 32) & 0x00000000FFFFFFFF);
  f->dwLowDateTime = (DWORD)(t & 0x00000000FFFFFFFF);
}

void
__tm_to_SYSTEMTIME (struct tm *t, SYSTEMTIME *s)
{
  s->wYear = t->tm_year + 1900;
  s->wMonth = t->tm_mon  + 1;
  s->wDayOfWeek = t->tm_wday;
  s->wDay = t->tm_mday;
  s->wHour = t->tm_hour;
  s->wMinute = t->tm_min;
  s->wSecond = t->tm_sec;
  s->wMilliseconds = 0;
}

static void
__FILETIME_from_Year (WORD year, FILETIME *f)
{
  SYSTEMTIME s = {0};

  s.wYear = year;
  s.wMonth = 1;
  s.wDayOfWeek = 1;
  s.wDay = 1;

  SystemTimeToFileTime (&s, f);
}

static int
__Yday_from_SYSTEMTIME (const SYSTEMTIME *s)
{
  long long t;
  FILETIME f1, f2;

  __FILETIME_from_Year (s->wYear, &f1);
  SystemTimeToFileTime (s, &f2);

  t = __FILETIME_to_ll (&f2) - __FILETIME_to_ll (&f1);

  return ((t / _onesec_in100ns) / (60 * 60 * 24));
}

void
__SYSTEMTIME_to_tm (SYSTEMTIME *s, struct tm *tm)
{
  tm->tm_year = s->wYear - 1900;
  tm->tm_mon = s->wMonth- 1;
  tm->tm_wday = s->wDayOfWeek;
  tm->tm_mday = s->wDay;
  tm->tm_yday = __Yday_from_SYSTEMTIME (s);
  tm->tm_hour = s->wHour;
  tm->tm_min = s->wMinute;
  tm->tm_sec = s->wSecond;
  tm->tm_isdst = 0;
}

time_t
__FILETIME_to_time_t (const FILETIME* f)
{
  long long t;
  t = __FILETIME_to_ll (f);
  t -= DELTA_EPOCH;
  return (time_t)(t / _onesec_in100ns);
}

void
__time_t_to_FILETIME (time_t t, FILETIME *f)
{
  long long time;

  time = t;
  time *= _onesec_in100ns;
  time += DELTA_EPOCH;

  __ll_to_FILETIME (time, f);
}
