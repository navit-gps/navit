void popup(struct container *co, int x, int y, int button);
struct popup_item *popup_item_new_text(struct popup_item **last, char *text, int priority);

struct popup {
	struct container *co;
	struct point pnt;
	struct coord c;
	void *gui_data;
	struct popup_item *items;
	struct popup_item *active;
};

struct popup_item {
	char *text;
	int priority;
	void (*func)(struct popup_item *, void *);
	void *param;
	struct popup_item *submenu;
	struct popup_item *next;
	void (*destroy)(struct popup_item *);
	int active;
};
