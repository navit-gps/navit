#ifndef NAVIT_PROFILE_H
#define NAVIT_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif
#define profile(level,fmt...) profile_timer(level,MODULE,__PRETTY_FUNCTION__,fmt)
void profile_timer(int level, const char *module, const char *function, const char *fmt, ...);
#ifdef __cplusplus
}
#endif

#endif

