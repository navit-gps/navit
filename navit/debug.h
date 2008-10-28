/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_DEBUG_H
#define NAVIT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
extern int debug_level;
#define dbg_str2(x) #x
#define dbg_str1(x) dbg_str2(x)
#define dbg_module dbg_str1(MODULE)
#define dbg(level,fmt...) ({ if (debug_level >= level) debug_printf(level,dbg_module,strlen(dbg_module),__PRETTY_FUNCTION__, strlen(__PRETTY_FUNCTION__),1,fmt); })
#define dbg_assert(expr) ((expr) ? (void) 0 : debug_assert_fail(dbg_module,strlen(dbg_module),__PRETTY_FUNCTION__, strlen(__PRETTY_FUNCTION__),__FILE__,__LINE__,dbg_str1(expr)))

/* prototypes */
struct attr;
void debug_init(const char *program_name);
void debug_destroy(void);
void debug_set_logfile(const char *path);
void debug_level_set(const char *name, int level);
int debug_level_get(const char *name);
struct debug *debug_new(struct attr *parent, struct attr **attrs);
void debug_vprintf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap);
void debug_printf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...);
void debug_assert_fail(char *module, const int mlen,const char *function, const int flen, char *file, int line, char *expr);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif

