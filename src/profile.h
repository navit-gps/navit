#ifndef NAVIT_PROFILE_H
#define NAVIT_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif
#define profile_str2(x) #x
#define profile_str1(x) profile_str2(x)
#define profile_module profile_str1(MODULE)
#define profile(level,fmt...) profile_timer(level,profile_module,__PRETTY_FUNCTION__,fmt)
void profile_timer(int level, const char *module, const char *function, const char *fmt, ...);
#ifdef __cplusplus
}
#endif

#endif

