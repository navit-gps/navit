#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "point.h"
#include "window.h"
#include "graphics.h"
#include "event.h"
#include <glib.h>

struct graphics_priv {
	struct window win;
};

static void *
get_data(struct graphics_priv *this, const char *type)
{
	dbg(0,"enter\n");
	if (strcmp(type,"window"))
		return NULL;
	return &this->win;
}


static struct graphics_methods graphics_methods = {
	NULL, /* graphics_destroy, */
	NULL, /* draw_mode, */
	NULL, /* draw_lines, */
	NULL, /* draw_polygon, */
	NULL, /* draw_rectangle, */
	NULL, /* draw_circle, */
	NULL, /* draw_text, */
	NULL, /* draw_image, */
	NULL, /* draw_image_warp, */
	NULL, /* draw_restore, */
	NULL, /* draw_drag, */
	NULL, /* font_new, */
	NULL, /* gc_new, */
	NULL, /* background_gc, */
	NULL, /* overlay_new, */
	NULL, /* image_new, */
	get_data,
	NULL, /* image_free, */
	NULL, /* get_text_bbox, */
	NULL, /* overlay_disable, */
	NULL, /* overlay_resize, */
	NULL, /* set_attr, */
};



struct graphics_priv *
graphics_cocoa_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	*meth=graphics_methods;
	dbg(0,"enter\n");
	if(!event_request_system("cocoa","graphics_cocoa"))
                return NULL;
	return g_new0(struct graphics_priv, 1);
}

static void
event_cocoa_main_loop_run(void)
{
	dbg(0,"enter\n");
	cocoa_if_main();
}

static void *
event_cocoa_add_timeout(void)
{
	dbg(0,"enter\n");
	return NULL;
}

static struct event_methods event_cocoa_methods = {
	event_cocoa_main_loop_run,
	NULL, /* event_cocoa_main_loop_quit, */
	NULL, /* event_cocoa_add_watch, */
	NULL, /* event_cocoa_remove_watch, */
	event_cocoa_add_timeout, 
	NULL, /* event_cocoa_remove_timeout, */
	NULL, /* event_cocoa_add_idle, */
	NULL, /* event_cocoa_remove_idle, */
	NULL, /* event_cocoa_call_callback, */
};


static struct event_priv *
event_cocoa_new(struct event_methods *meth)
{
	dbg(0,"enter\n");
	*meth=event_cocoa_methods;
	return NULL;
}


void
plugin_init(void)
{
	dbg(0,"enter\n");
	plugin_register_graphics_type("cocoa", graphics_cocoa_new);
	plugin_register_event_type("cocoa", event_cocoa_new);
}
