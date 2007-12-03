#include <glib.h>
#include "menu.h"

void
menu_route_do_update(struct container *co)
{
#if 0
	if (co->cursor) {
#if 0 /* FIXME */
		route_set_position(co->route, cursor_pos_get(co->cursor));
#endif
		graphics_redraw(co);
		if (co->statusbar && co->statusbar->statusbar_route_update)
			co->statusbar->statusbar_route_update(co->statusbar, co->route);
	}
#endif
}

void
menu_route_update(struct container *co)
{
#if 0
	menu_route_do_update(co);
	graphics_redraw(co);
#endif
}

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
	(*menu->meth.set_toggle)(menu->priv, active);
}

int
menu_get_toggle(struct menu *menu)
{
	return (*menu->meth.get_toggle)(menu->priv);
}
