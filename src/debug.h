#include <stdarg.h>
int debug_level;
#define dbg(level,fmt...) ({ if (debug_level >= level) debug_printf(level,MODULE,__PRETTY_FUNCTION__,1,fmt); })

/* prototypes */
void debug_init(void);
void debug_level_set(const char *name, int level);
int debug_level_get(const char *name);
void debug_vprintf(int level, const char *module, const char *function, int prefix, const char *fmt, va_list ap);
void debug_printf(int level, const char *module, const char *function, int prefix, const char *fmt, ...);
/* end of prototypes */
