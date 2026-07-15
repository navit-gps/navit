#include <glib.h>
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 10
#    define g_slice_alloc0 g_malloc0
#    define g_slice_new0(x) g_new0(x, 1)
#    define g_slice_free(x, y) g_free(y)
#    define g_slice_free1(x, y) g_free(y)
#endif
#if !GLIB_CHECK_VERSION(2, 68, 0)
#    define g_memdup2(mem, n) g_memdup((mem), (guint)(n))
#endif
