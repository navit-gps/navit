#include "config.h"
#ifndef HAVE_API_WIN32_BASE
#define USE_POSIX_THREADS 1
#endif
#if USE_POSIX_THREADS
#include <pthread.h>
#endif
#include "debug.h"
#include "gtypes.h"

#define g_return_if_fail


#if USE_POSIX_THREADS
# define GMutex pthread_mutex_t
# define g_mutex_new g_mutex_new_navit
# define g_mutex_lock(lock) ((lock == NULL) ? 0 : pthread_mutex_lock(lock))
# define g_mutex_unlock(lock) ((lock == NULL) ? 0 : pthread_mutex_unlock(lock))
# define g_mutex_trylock(lock) (((lock == NULL) ? 0 : pthread_mutex_trylock(lock)) == 0)
#  define GPrivate pthread_key_t
#  define g_private_new(xd) g_private_new_navit()
#  define g_private_get(xd) pthread_getspecific(xd)
#  define g_private_set(a,b) pthread_setspecific(a, b)

pthread_mutex_t* g_mutex_new_navit(void);
void g_get_current_time (GTimeVal *result);
GPrivate g_private_new_navit ();

#else
# if HAVE_API_WIN32_BASE
#  define GMutex CRITICAL_SECTION
#  define g_mutex_new g_mutex_new_navit
#  define g_mutex_lock(lock) (EnterCriticalSection(lock))
#  define g_mutex_unlock(lock) (LeaveCriticalSection(lock))
#  define g_mutex_trylock(lock) (TryEnterCriticalSection(lock))
#  define GPrivate int
#  define g_private_new(xd) g_private_new_navit()
#  define g_private_get(xd) TlsGetValue(xd)
#  define g_private_set(a,b) TlsSetValue(a, b)
# endif
#endif

char* g_convert               (const char  *str,
				int        len,            
				const char  *to_codeset,
				const char  *from_codeset,
				int        *bytes_read,     
				int        *bytes_written,  
				void      **error);
#define G_LOCK_DEFINE_STATIC(name)    //void
#define G_LOCK(name) //void //g_mutex_lock       (&G_LOCK_NAME (name))
#define G_UNLOCK(name) //void //g_mutex_unlock   (&G_LOCK_NAME (name))

#define g_thread_supported() TRUE

#define g_assert(expr) dbg_assert (expr)
