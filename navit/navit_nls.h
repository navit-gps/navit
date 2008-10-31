#ifndef __NAVIT_NLS_H__
#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(STRING)    gettext(STRING)
#define gettext_noop(String) String
#define _n(STRING)    gettext_noop(STRING)
#else
#define _(STRING) STRING
#define _n(STRING) STRING
#define gettext(STRING) STRING
static inline const char *ngettext(const char *msgid, const char *msgid_plural, unsigned long int n)
{
	if (n == 1) {
		return msgid;
	} else {
		return msgid_plural;
	}
}
#endif
#define __NAVIT_NLS_H__
#endif
