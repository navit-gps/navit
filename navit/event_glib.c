/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include "event.h"
#include "event_glib.h"
#include "debug.h"
#include "callback.h"

static GMainLoop *loop;

static void event_glib_main_loop_run(void)
{
	loop = g_main_loop_new (NULL, TRUE);
        if (g_main_loop_is_running (loop))
        {
		g_main_loop_run (loop);
	}
}

static void event_glib_main_loop_quit(void)
{
	if (loop)
		g_main_loop_quit(loop);
}

struct event_watch {
	GIOChannel *iochan;
	guint source;
};

static gboolean
event_glib_call_watch(GIOChannel * iochan, GIOCondition condition, gpointer t)
{
	struct callback *cb=t;
	callback_call_0(cb);
	return TRUE;
}

static struct event_watch *
event_glib_add_watch(int fd, int w, struct callback *cb)
{
	struct event_watch *ret=g_new0(struct event_watch, 1);
	ret->iochan = g_io_channel_unix_new(fd);
	ret->source = g_io_add_watch(ret->iochan, G_IO_IN | G_IO_ERR | G_IO_HUP, event_glib_call_watch, (gpointer)cb);
	return ret;
}

static void
event_glib_remove_watch(struct event_watch *ev)
{
	GError *error = NULL;
	g_source_remove(ev->source);
	g_io_channel_shutdown(ev->iochan, 0, &error);
	g_free(ev);
}

struct event_timeout {
	guint source;
};

static gboolean
event_glib_call_timeout_single(gpointer t)
{
	struct callback *cb=t;
	callback_call_0(cb);
	return FALSE;
}

static gboolean
event_glib_call_timeout_multi(gpointer t)
{
	struct callback *cb=t;
	callback_call_0(cb);
	return TRUE;
}


static struct event_timeout *
event_glib_add_timeout(int timeout, int multi, struct callback *cb)
{
	struct event_timeout *ret=g_new0(struct event_timeout, 1);
	ret->source = g_timeout_add(timeout, multi ? (GSourceFunc)event_glib_call_timeout_multi : (GSourceFunc)event_glib_call_timeout_single, (gpointer)cb);

	return ret;
}

static void
event_glib_remove_timeout(struct event_timeout *ev)
{
	g_source_remove(ev->source);
	g_free(ev);
}

static struct event_idle *
event_glib_add_idle(struct callback *cb)
{
	return NULL;
}

static void
event_glib_remove_idle(struct event_idle *ev)
{
}

static struct event_methods event_glib_methods = {
	event_glib_main_loop_run,
	event_glib_main_loop_quit,
	event_glib_add_watch,
	event_glib_remove_watch,
	event_glib_add_timeout,
	event_glib_remove_timeout,
	event_glib_add_idle,
	event_glib_remove_idle,
};

static void
event_glib_new(struct event_methods *meth)
{
	*meth=event_glib_methods;
}

void
event_glib_init(void)
{
	plugin_register_event_type("glib", event_glib_new);
}
