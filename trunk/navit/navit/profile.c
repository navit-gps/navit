/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif /* _MSC_VER */
#include "profile.h"
#include "debug.h"

void
profile_timer(int level, const char *module, const char *function, const char *fmt, ...)
{
	#ifndef _MSC_VER
	va_list ap;
	static struct timeval last[10];
	struct timeval curr;
	int msec,usec;
	char buffer[strlen(module)+20];

	va_start(ap, fmt);
	if (level < 0)
		level=0;
	if (level > 9)
		level=9;
	if (fmt) {
		gettimeofday(&curr, NULL);
		msec=(curr.tv_usec-last[level].tv_usec)/1000+
		     (curr.tv_sec-last[level].tv_sec)*1000;
	
		sprintf(buffer, "profile:%s", module);
		debug_vprintf(1, buffer, strlen(buffer), function, strlen(function), 1, fmt, ap); 
		if (msec >= 100) 
			debug_printf(1, buffer, strlen(buffer), function, strlen(function), 0, " %d msec\n", msec);
		else {
			usec=(curr.tv_usec-last[level].tv_usec)+(curr.tv_sec-last[level].tv_sec)*1000*1000;
			debug_printf(1, buffer, strlen(buffer), function, strlen(function), 0, " %d.%d msec\n", usec/1000, usec%1000);
		}
		gettimeofday(&last[level], NULL);
	} else {
		gettimeofday(&curr, NULL);
		while (level < 10) 
			last[level++]=curr;
	}
	va_end(ap);
	#endif _MSC_VER
}
