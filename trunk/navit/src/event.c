#include <glib.h>
#include "event.h"

static GMainLoop *loop;

void event_main_loop_run(void)
{
	loop = g_main_loop_new (NULL, TRUE);
        if (g_main_loop_is_running (loop))
        {
		g_main_loop_run (loop);
	}
}

void event_main_loop_quit(void)
{
	if (loop)
		g_main_loop_quit(loop);
} 
