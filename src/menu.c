#include <glib.h>
#include "menu.h"
#include "debug.h"

struct menu *
menu_add(struct menu *menu, char *name, enum menu_type type, struct callback *cb)
{
	struct menu *this;
        this=g_new0(struct menu, 1);
        this->priv=(*menu->meth.add)(menu->priv, &this->meth, name, type, cb);
	if (! this->priv) {
		g_free(this);
		return NULL;
	}

	return this;	
}

void
menu_popup(struct menu *menu)
{
	if (! menu || ! menu->meth.popup)
		return;
	(*menu->meth.popup)(menu->priv);

}
