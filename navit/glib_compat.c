#include <glib.h>

void g_list_free_full(GList *list, GDestroyNotify free_func) {
    g_list_foreach(list, (GFunc)free_func, NULL);
    g_list_free(list);
}
