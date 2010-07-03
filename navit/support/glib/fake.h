#include "config.h"
#if USE_POSIX_THREADS
#include <pthread.h>
#endif
#include "debug.h"

#define g_return_if_fail

#if USE_POSIX_THREADS
# define GMutex pthread_mutex_t
# define g_mutex_new g_mutex_new_navit
# define g_mutex_lock(lock) (pthread_mutex_lock(lock))
# define g_mutex_unlock(lock) (pthread_mutex_unlock(lock))
# define g_mutex_trylock(lock) (pthread_mutex_trylock(lock))
#else
# if HAVE_API_WIN32_BASE
#  define GMutex CRITICAL_SECTION
#  define g_mutex_new g_mutex_new_navit
#  define g_mutex_lock(lock) (EnterCriticalSection(lock))
#  define g_mutex_unlock(lock) (LeaveCriticalSection(lock))
#  define g_mutex_trylock(lock) (TryEnterCriticalSection(lock))
# endif
#endif

#define GPrivate int
#define g_private_new(xd) g_private_new_navit()
#define g_private_get(xd) TlsGetValue(xd)
#define g_private_set(a,b) TlsSetValue(a, b)

#define G_LOCK_DEFINE_STATIC(name)    void
#define G_LOCK(name) void //g_mutex_lock       (&G_LOCK_NAME (name))
#define G_UNLOCK(name) void //g_mutex_unlock   (&G_LOCK_NAME (name))

#define g_thread_supported() TRUE

#define g_assert(expr) dbg_assert (expr)
