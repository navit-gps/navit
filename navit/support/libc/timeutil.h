#ifndef __TIMEUTIL_H
#define __TIMEUTIL_H

#include <windows.h>
#include <sys/time.h>
#include <_mingw.h>

#define _onesec_in100ns 10000000LL

extern long long __FILETIME_to_ll (const FILETIME *f);
extern void __ll_to_FILETIME (long long t, FILETIME* f);
extern void __tm_to_SYSTEMTIME (struct tm *, SYSTEMTIME *);
extern void __SYSTEMTIME_to_tm (SYSTEMTIME *, struct tm *);
extern time_t __FILETIME_to_time_t (const FILETIME *);
extern void __time_t_to_FILETIME (time_t, FILETIME *);

#endif
