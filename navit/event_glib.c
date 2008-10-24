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
event_glib_add_watch(struct file *file, enum event_watch_cond cond, struct callback *cb)
{
	struct event_watch *ret=g_new0(struct event_watch, 1);
	int flags=0,fd=(int)file_get_os_handle(file);
	ret->iochan = g_io_channel_unix_new(fd);
	switch (cond) {
	case event_watch_cond_read:
		flags=G_IO_IN;
		break;
	case event_watch_cond_write:
		flags=G_IO_OUT;
		break;
	case event_watch_cond_except:
		flags=G_IO_ERR|G_IO_HUP;
		break;
	}	
	ret->source = g_io_add_watch(ret->iochan, cond, event_glib_call_watch, (gpointer)cb);
	return ret;
}

static void
event_glib_remove_watch(struct event_watch *ev)
{
	GError *error = NULL;
	if (! ev)
		return;
	g_source_remove(ev->source);
	g_io_channel_shutdown(ev->iochan, 0, &error);
	g_free(ev);
}

struct event_timeout {
	guint source;
	struct callback *cb;
};

static gboolean
event_glib_call_timeout_single(struct event_timeout *ev)
{
	callback_call_0(ev->cb);
	g_free(ev);
	return FALSE;
}

static gboolean
event_glib_call_timeout_multi(struct event_timeout *ev)
{
	callback_call_0(ev->cb);
	return TRUE;
}


static struct event_timeout *
event_glib_add_timeout(int timeout, int multi, struct callback *cb)
{
	struct event_timeout *ret=g_new0(struct event_timeout, 1);
	ret->cb=cb;
	ret->source = g_timeout_add(timeout, multi ? (GSourceFunc)event_glib_call_timeout_multi : (GSourceFunc)event_glib_call_timeout_single, (gpointer)ret);

	return ret;
}

static void
event_glib_remove_timeout(struct event_timeout *ev)
{
	if (! ev)
		return;
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
