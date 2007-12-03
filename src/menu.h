#ifndef NAVIT_MENU_H
#define NAVIT_MENU_H

enum menu_type {
	menu_type_submenu,
	menu_type_menu,
	menu_type_toggle,
};

struct menu;

struct menu_methods {
	struct menu_priv *(*add)(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb);
	void (*set_toggle)(struct menu_priv *menu, int active);
	int (*get_toggle)(struct menu_priv *menu);
};

struct menu {
	struct menu_priv *priv;
	struct menu_methods meth;
};

/* prototypes */
enum menu_type;
struct callback;
struct container;
struct menu;
void menu_route_do_update(struct container *co);
void menu_route_update(struct container *co);
struct menu *menu_add(struct menu *menu, char *name, enum menu_type type, struct callback *cb);
void menu_set_toggle(struct menu *menu, int active);
int menu_get_toggle(struct menu *menu);
/* end of prototypes */
#endif
