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

#include <string.h>
#include <stdlib.h>
#include "event.h"
#include "plugin.h"
#include "debug.h"

static struct event_methods event_methods;
static char *e_requestor;
static char *e_system;

void event_main_loop_run(void)
{
	if (! event_methods.main_loop_run) {
		dbg(0,"no event system set\n");
		exit(1);
	}
	event_methods.main_loop_run();
}

void event_main_loop_quit(void)
{
	if (event_methods.main_loop_quit)
		event_methods.main_loop_quit();
}

struct event_watch *
event_add_watch(void *fd, enum event_watch_cond cond, struct callback *cb)
{
	return event_methods.add_watch(fd, cond, cb);
}

void
event_remove_watch(struct event_watch *ev)
{
	event_methods.remove_watch(ev);
}

struct event_timeout *
event_add_timeout(int timeout, int multi, struct callback *cb)
{
	return event_methods.add_timeout(timeout, multi, cb);
}

void
event_remove_timeout(struct event_timeout *ev)
{
	event_methods.remove_timeout(ev);
}

struct event_idle *
event_add_idle(struct callback *cb)
{
	return event_methods.add_idle(cb);
}

void
event_remove_idle(struct event_idle *ev)
{
	event_methods.remove_idle(ev);
}

void
event_call_callback(struct callback_list *cb)
{
	event_methods.call_callback(cb);
}

int
event_request_system(char *system, char *requestor)
{
	void (*event_type_new)(struct event_methods *meth);
	if (e_system) {
		if (strcmp(e_system, system)) {
			dbg(0,"system '%s' already requested by '%s', can't set to '%s' as requested from '%s'\n", e_system, e_requestor, system, requestor);
			return 0;
		}
		return 1;
	}
	event_type_new=plugin_get_event_type(system);
        if (! event_type_new) {
		dbg(0,"unsupported event system '%s' requested from '%s'\n", system, requestor);
                return 0;
	}
	event_type_new(&event_methods);
	e_system=system;
	e_requestor=requestor;
	
	return 1;
}


