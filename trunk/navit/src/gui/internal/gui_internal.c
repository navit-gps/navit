#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#include <libintl.h>
#include <gtk/gtk.h>
#include "config.h"
#include "item.h"
#include "navit.h"
#include "debug.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "transform.h"
#include "config.h"

struct gui_priv {
        struct navit *nav;
};

static int
gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	void *graphics;
	
	dbg(0,"enter\n");
	graphics=graphics_get_data(gra, "window");
        if (! graphics)
                return 1;
	return 0;
}


struct gui_methods gui_internal_methods = {
	NULL,
	NULL,
	NULL,
	NULL,
        gui_internal_set_graphics,
};

static struct gui_priv *
gui_internal_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	struct gui_priv *this;
	*meth=gui_internal_methods;
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;

	return this;
}

void
plugin_init(void)
{
	plugin_register_gui_type("internal", gui_internal_new);
}
