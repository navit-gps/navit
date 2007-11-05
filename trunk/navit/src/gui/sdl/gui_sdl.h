#include "SDL/SDL.h"

#define XRES 800
#define YRES 600


struct statusbar_methods;
struct menu_methods;
struct navit;

extern struct navit *sdl_gui_navit;

struct gui_priv {
/*
        GtkWidget *win;
        GtkWidget *vbox;
	GtkActionGroup *base_group;
	GtkActionGroup *debug_group;
	GtkActionGroup *dyn_group;
	GtkUIManager *menu_manager;
        void *statusbar;*/
	struct navit *nav;
	int dyn_counter;
};

