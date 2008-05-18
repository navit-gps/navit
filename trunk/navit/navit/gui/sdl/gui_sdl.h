#include "SDL/SDL.h"

#define XRES 800
#define YRES 600


struct menu_methods;
struct navit;

extern struct navit *sdl_gui_navit;


bool BookmarkGo(const char * name);
bool FormerDestGo(const char * name);

struct gui_priv {
	struct navit *nav;
	int dyn_counter;
};

