#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "glib.h"
#include <stdio.h>
#include <libintl.h>

#include "navit.h"
#include "config.h"
#include "gui.h"
#include "debug.h"
#include "GL/glc.h"


#include "projection.h"

#include "item.h"
#include "navit.h"
#include "vehicle.h"	
#include "profile.h"
#include "transform.h"
#include "coord.h"
#include "callback.h"
#include "point.h"
#include "graphics.h"
#include "navigation.h"
#include "attr.h"
#include "track.h"
#include "menu.h"
#include "map.h"


#define MENU_BOOKMARK 2
#define MENU_FORMER_DEST 3

struct bookmark{
	char * name;
	struct callback *cb;
	struct bookmark *next;
} *bookmarks;

struct former_dest{
	char * name;
	struct callback *cb;
	struct former_dest *next;
} *former_dests;


static struct menu_priv * 
 add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb);

static struct menu_methods menu_methods = {
	add_menu,
};

struct menu_priv {
	char *path;	
// 	GtkAction *action;
	struct gui_priv *gui;
	enum menu_type type;
	struct callback *cb;
	struct menu_priv *child;
	struct menu_priv *sibling;
	gulong handler_id;
	guint merge_id;
};

struct menu_priv *
gui_clutter_menubar_new(struct gui_priv *this_, struct menu_methods *meth);
