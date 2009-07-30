#ifndef _GLOB_H_
#define _GLOB_H_

#ifndef HAVE_GLOB
#ifdef __MINGW32__

typedef struct {
  size_t gl_pathc;
  char **gl_pathv;
  size_t gl_offs;
} glob_t;

int  glob(const char *pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *pglob);
void globfree(glob_t *pglob);

#endif
#endif

#endif /* _GLOB_H_ */
