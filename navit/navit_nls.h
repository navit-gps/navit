#ifndef __NAVIT_NLS_H__

const char *navit_nls_gettext(const char *msgid);
const char *navit_nls_ngettext(const char *msgid, const char *msgid_plural, unsigned long int n);

#define _(STRING)    navit_nls_gettext(STRING)
#define gettext_noop(String) String
#define _n(STRING)    gettext_noop(STRING)

#define __NAVIT_NLS_H__
#endif
