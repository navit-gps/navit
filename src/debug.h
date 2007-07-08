int debug_level;
#define dbg(level,fmt...) if (debug_level >= level) debug_print(level,MODULE,__PRETTY_FUNCTION__,fmt)

/* prototypes */
void debug_init(void);
void debug_level_set(const char *name, int level);
int debug_level_get(const char *name);
void debug_print(int level, const char *module, const char *function, const char *fmt, ...);
/* end of prototypes */
