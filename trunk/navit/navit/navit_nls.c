#include "config.h"
#include "debug.h"
#include <glib.h>

#ifdef USE_LIBGNUINTL
#include <libgnuintl.h>
#else
#include <libintl.h>
#endif
static GList *textdomains;

char *
navit_nls_add_textdomain(const char *package, const char *dir)
{
#ifdef ENABLE_NLS
	char *ret=bindtextdomain(package, dir);
	bind_textdomain_codeset(package, "UTF-8");
	textdomains=g_list_append(textdomains, g_strdup(package));
	return ret;
#endif
}

void
navit_nls_remove_textdomain(const char *package)
{
#ifdef ENABLE_NLS
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

const char *
navit_nls_gettext(const char *msgid)
{
#ifdef ENABLE_NLS
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

const char *
navit_nls_ngettext(const char *msgid, const char *msgid_plural, unsigned long int n)
{
#ifdef ENABLE_NLS
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
