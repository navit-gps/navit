#include "config.h"
#include "debug.h"

#ifdef USE_LIBGNUINTL
#include <libgnuintl.h>
#else
#include <libintl.h>
#endif


const char *
navit_nls_gettext(const char *msgid)
{
#ifdef ENABLE_NLS
	return gettext(msgid);
#else
	return msgid;
#endif
}

const char *
navit_nls_ngettext(const char *msgid, const char *msgid_plural, unsigned long int n)
{
#ifdef ENABLE_NLS
	return ngettext(msgid, msgid_plural, n);
#else
	if (n == 1) {
		return msgid;
	} else {
		return msgid_plural;
	}
#endif
}
