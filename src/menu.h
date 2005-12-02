struct menu {
	struct menu_gui *gui;
};

struct container;

void menu_route_do_update(struct container *co);
void menu_route_update(struct container *co);
