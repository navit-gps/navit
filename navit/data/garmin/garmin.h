#define dlog(x,y ...) logfn(__FILE__,__LINE__,x, ##y)
#ifdef HARDDEBUG
#define ddlog(x,y ...) logfn(__FILE__,__LINE__,x, ##y)
#else
#define ddlog(x,y ...)
#endif

extern int garmin_debug;
void logfn(char *file, int line, int level, char *fmt, ...);

