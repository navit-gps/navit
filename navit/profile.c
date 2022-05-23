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

#define PROFILE_LEVEL_MAX 9

/**
 * @brief Log timing information.
 *
 * @note Normally this function is called via the macro 'profile', which automatically fills in
 * parameters 'module' and 'function'.
 *
 * Successive calls to this function will print the elapsed time between calls, using an
 * internal timer. To start/reset the internal timer without printing a message, set parameter
 * 'fmt' to NULL.
 *
 * To run multiple timers in parallel, the parameter 'level' can be used. Each invocation will
 * print the elapsed time to the last invocation with the same level. As an exception, calling
 * with 'fmt'=NULL wll reset all timers with a level greater or equal to the level parameter.
 *
 * Typically, profiling is started by calling profile(0, NULL). Then calls with various levels
 * can be used to print different intermediate timings.
 *
 * @param level level of timer to use (0 to PROFILE_LEVEL_MAX).
 * @param module name of current module (for logging)
 * @param function name of current function (for logging)
 * @param fmt format string Log message to print (as a printf format string), followed by required
 * parameters as varargs. May be NULL; then no message is printed, and all timers with the
 * same or higher level are reset.
 */
void profile_timer(int level, const char *module, const char *function, const char *fmt, ...) {
#ifndef _MSC_VER
    va_list ap;
    static struct timeval last[PROFILE_LEVEL_MAX+1];
    struct timeval curr;
    double msec;
    char buffer[strlen(module)+20];

    if (level < 0)
        level=0;
    if (level > PROFILE_LEVEL_MAX)
        level=PROFILE_LEVEL_MAX;
    if (fmt) {
        gettimeofday(&curr, NULL);
        msec=(curr.tv_usec-last[level].tv_usec)/((double)1000)+
             (curr.tv_sec-last[level].tv_sec)*1000;

        sprintf(buffer, "profile:%s", module);
        va_start(ap, fmt);
        debug_vprintf(lvl_debug, buffer, strlen(buffer), function, strlen(function), 1, fmt, ap);
        va_end(ap);
        debug_printf(lvl_debug, buffer, strlen(buffer), function, strlen(function), 0, " %7.1f ms\n", msec);
        gettimeofday(&last[level], NULL);
    } else {
        gettimeofday(&curr, NULL);
        while (level <= PROFILE_LEVEL_MAX)
            last[level++]=curr;
    }
#endif /*_MSC_VER*/
}
