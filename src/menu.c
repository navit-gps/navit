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
menu_set_toggle(struct menu *menu, int active)
{
	if(*menu->meth.set_toggle){
		(*menu->meth.set_toggle)(menu->priv, active);
	}
}

int
menu_get_toggle(struct menu *menu)
{
	return ((*menu->meth.get_toggle) && (*menu->meth.get_toggle)(menu->priv));
}

void
menu_popup(struct menu *menu)
{
	if (! menu || ! menu->meth.popup)
		return;
	(*menu->meth.popup)(menu->priv);

}
