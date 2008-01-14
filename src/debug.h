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

/* prototypes */
void debug_init(void);
void debug_level_set(const char *name, int level);
int debug_level_get(const char *name);
void debug_vprintf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap);
void debug_printf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif

