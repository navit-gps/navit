#include "config.h"
#include "debug.h"
#include <glib.h>
#include <navit_nls.h>
#include <stdlib.h>
#ifdef HAVE_API_WIN32_CE
#include "libc.h"
#endif

#ifdef USE_LIBGNUINTL
#include <libgnuintl.h>
#else
#include <libintl.h>
#endif
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
static GList *textdomains;
#endif

char *navit_nls_add_textdomain(const char *package, const char *dir) {
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
    char *ret=bindtextdomain(package, dir);
    bind_textdomain_codeset(package, "UTF-8");
    textdomains=g_list_append(textdomains, g_strdup(package));
    return ret;
#else
    return NULL;
#endif
}

void navit_nls_remove_textdomain(const char *package) {
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
    GList *i=textdomains;
    while (i) {
        if (!strcmp(i->data, package)) {
            textdomains=g_list_remove_all(textdomains, i->data);
            g_free(i->data);
            return;
        }
        i=g_list_next(i);
    }
#endif
}

const char *navit_nls_gettext(const char *msgid) {
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
    GList *i=textdomains;
    while (i) {
        const char *ret=dgettext(i->data, msgid);
        if (ret != msgid)
            return ret;
        i=g_list_next(i);
    }
#endif
    return msgid;
}

const char *navit_nls_ngettext(const char *msgid, const char *msgid_plural, unsigned long int n) {
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
    GList *i=textdomains;
    while (i) {
        const char *ret=dngettext(i->data, msgid, msgid_plural, n);
        if (ret != msgid && ret != msgid_plural)
            return ret;
        i=g_list_next(i);
    }
#endif
    if (n == 1) {
        return msgid;
    } else {
        return msgid_plural;
    }
}

void navit_nls_main_init(void) {
#ifdef USE_NATIVE_LANGUAGE_SUPPORT
#ifdef FORCE_LOCALE
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
    setlocale(LC_MESSAGES,STRINGIFY(FORCE_LOCALE));
#endif
    navit_nls_add_textdomain(PACKAGE, getenv("NAVIT_LOCALEDIR"));
    textdomain(PACKAGE);
#endif
}
