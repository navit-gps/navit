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
#include <string.h>

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCTION__

/* Uncomment the following define to enable MSVC's memory debugging support */
/*#define _CRTDBG_MAP_ALLOC*/
#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

/** Possible debug levels (inspired by SLF4J). */
typedef enum {
	/** Internal use only, do not use for logging. */
	lvl_unset=-1,
	/** Error: something did not work. */
	lvl_error,
	/** Warning: something may not have worked. */
	lvl_warning,
	/** Informational message. Should make sense to non-programmers. */
	lvl_info,
	/** Debug output: (almost) anything goes. */
	lvl_debug
} dbg_level;

extern dbg_level max_debug_level;
#define dbg_str2(x) #x
#define dbg_str1(x) dbg_str2(x)
#define dbg_module dbg_str1(MODULE)
#define dbg(level,...) { if (max_debug_level >= level) debug_printf(level,dbg_module,strlen(dbg_module),__PRETTY_FUNCTION__, strlen(__PRETTY_FUNCTION__),1,__VA_ARGS__); }
#define dbg_assert(expr) ((expr) ? (void) 0 : debug_assert_fail(dbg_module,strlen(dbg_module),__PRETTY_FUNCTION__, strlen(__PRETTY_FUNCTION__),__FILE__,__LINE__,dbg_str1(expr)))

#define DEBUG_MODULE_GLOBAL "global"

#ifdef DEBUG_MALLOC
#undef g_new
#undef g_new0
#define g_new(type, size) (type *)debug_malloc(__FILE__,__LINE__,__PRETTY_FUNCTION__,sizeof(type)*(size))
#define g_new0(type, size) (type *)debug_malloc0(__FILE__,__LINE__,__PRETTY_FUNCTION__,sizeof(type)*(size))
#define g_malloc(size) debug_malloc(__FILE__,__LINE__,__PRETTY_FUNCTION__,(size))
#define g_malloc0(size) debug_malloc0(__FILE__,__LINE__,__PRETTY_FUNCTION__,(size))
#define g_realloc(ptr,size) debug_realloc(__FILE__,__LINE__,__PRETTY_FUNCTION__,ptr,(size))
#define g_free(ptr) debug_free(__FILE__,__LINE__,__PRETTY_FUNCTION__,ptr)
#define g_strdup(ptr) debug_strdup(__FILE__,__LINE__,__PRETTY_FUNCTION__,ptr)
#define g_strdup_printf(fmt...) debug_guard(__FILE__,__LINE__,__PRETTY_FUNCTION__,g_strdup_printf(fmt))
#define graphics_icon_path(x) debug_guard(__FILE__,__LINE__,__PRETTY_FUNCTION__,graphics_icon_path(x))
#define dbg_guard(x) debug_guard(__FILE__,__LINE__,__PRETTY_FUNCTION__,x)
#define g_free_func debug_free_func
#else
#define g_free_func g_free
#define dbg_guard(x) x
#endif

/* prototypes */
struct attr;
struct debug;
void debug_init(const char *program_name);
void debug_level_set(const char *name, dbg_level level);
struct debug *debug_new(struct attr *parent, struct attr **attrs);
dbg_level debug_level_get(const char *name);
void debug_vprintf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap);
void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...)
#ifdef  __GNUC__
	__attribute__ ((format (printf, 7, 8)))
#endif
;
void debug_assert_fail(const char *module, const int mlen, const char *function, const int flen, const char *file, int line, const char *expr);
void debug_destroy(void);
void debug_set_logfile(const char *path);
void debug_dump_mallocs(void);
void *debug_malloc(const char *where, int line, const char *func, int size);
void *debug_malloc0(const char *where, int line, const char *func, int size);
char *debug_strdup(const char *where, int line, const char *func, const char *ptr);
char *debug_guard(const char *where, int line, const char *func, char *str);
void debug_free(const char *where, int line, const char *func, void *ptr);
void debug_free_func(void *ptr);
void debug_finished(void);
void *debug_realloc(const char *where, int line, const char *func, void *ptr, int size);
void debug_set_global_level(dbg_level level, int override_old_value);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif

