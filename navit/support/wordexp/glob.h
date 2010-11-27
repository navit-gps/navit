#ifndef _GLOB_H_
#define _GLOB_H_

#ifndef HAVE_GLOB
#if defined __MINGW32__ || defined _MSC_VER

typedef struct {
  size_t   gl_pathc;  /* count of file names */
  char   **gl_pathv;  /* list of file names */
  size_t   gl_offs;   /* slots to reserve in gl_pathv */
} glob_t;

int  glob(const char *pattern, int flags, int (*errfunc)(const char *epath, int eerrno), glob_t *pglob);
void globfree(glob_t *pglob);

#endif /* defined __MINGW32__ || defined _MSC_VER */
#endif

#endif /* _GLOB_H_ */
