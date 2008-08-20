#include "gui_clutter_menuhack.h"

static struct menu_priv *
add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb)
{
	*meth=menu_methods;
	dbg(0,"callback : %s\n",name);

	if(menu==(struct menu_priv *)(MENU_BOOKMARK)){
		dbg(0,"Item %s is a bookmark\n",name);

		struct bookmark *newB = g_new0(struct bookmark, 1);
		newB->name=g_strdup(name);
		newB->cb=cb;
		if (newB) {
			newB->next = bookmarks;
			bookmarks = newB;
		}

	}

	if(menu==(struct menu_priv *)(MENU_FORMER_DEST)){
		dbg(0,"Item %s is a former destination\n",name);

		struct former_dest *newB = g_new0(struct former_dest, 1);
		newB->name=g_strdup(name);
		newB->cb=cb;
		if (newB) {
			newB->next = former_dests;
			former_dests = newB;
		}

	}

	if(!strcmp(name,"Bookmarks")){
		dbg(0,"Menu is the bookmark menu!\n");
		return (struct menu_priv *)MENU_BOOKMARK;
	} else if(!strcmp(name,"Former Destinations")){
		dbg(0,"Menu is the Former Destinations menu!\n");
		return (struct menu_priv *)MENU_FORMER_DEST;
	} else {
		return (struct menu_priv *)1;
	}
}

struct menu_priv *
gui_clutter_menubar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	dbg(0,"Creating the menus\n");
	*meth=menu_methods;
	return (struct menu_priv *) 1; //gui_gtk_ui_new(this_, meth, "/ui/MenuBar", nav, 0);
}
