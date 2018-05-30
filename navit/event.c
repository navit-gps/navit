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
static const char *e_requestor;
static const char *e_system;

static int has_quit;

#define require_method_helper(m)\
	if(!event_methods.m) {\
		dbg(lvl_error, "Can't find event system method %s. Event system is %s%s",\
			#m ,e_system?"set to ":"not set.", e_system?e_system:"");\

#define require_method(m)\
		require_method_helper(m)\
		return;\
	}

#define require_method2(m,r)\
		require_method_helper(m)\
		return r;\
	}

void event_main_loop_run(void) {
    require_method(main_loop_run);
    event_methods.main_loop_run();
}

void event_main_loop_quit(void) {
    if (event_methods.main_loop_quit)
        event_methods.main_loop_quit();
    has_quit=1;
}

int event_main_loop_has_quit(void) {
    return has_quit;
}

struct event_watch *
event_add_watch(int fd, enum event_watch_cond cond, struct callback *cb) {
    require_method2(add_watch, NULL);
    return event_methods.add_watch(fd, cond, cb);
}

void event_remove_watch(struct event_watch *ev) {
    require_method(remove_watch);
    event_methods.remove_watch(ev);
}

/**
 * Add an event timeout
 *
 * @param the timeout itself in msec
 * @param multi 0 means that the timeout will fire only once, 1 means that it will repeat
 * @param the callback to call when the timeout expires
 *
 * @returns the result of the event_methods.add_timeout() call
 */
struct event_timeout *
event_add_timeout(int timeout, int multi, struct callback *cb) {
    require_method2(add_timeout, NULL);
    return event_methods.add_timeout(timeout, multi, cb);
}

void event_remove_timeout(struct event_timeout *ev) {
    require_method(remove_timeout);
    event_methods.remove_timeout(ev);
}

struct event_idle *
event_add_idle(int priority, struct callback *cb) {
    require_method2(add_idle, NULL);
    return event_methods.add_idle(priority,cb);
}

void event_remove_idle(struct event_idle *ev) {
    require_method(remove_idle);
    event_methods.remove_idle(ev);
}

void event_call_callback(struct callback_list *cb) {
    require_method(call_callback);
    event_methods.call_callback(cb);
}

char const *event_system(void) {
    return e_system;
}

int event_request_system(const char *system, const char *requestor) {
    void (*event_type_new)(struct event_methods *meth);
    if (e_system) {
        if (strcmp(e_system, system)) {
            dbg(lvl_error,"system '%s' already requested by '%s', can't set to '%s' as requested from '%s'", e_system, e_requestor,
                system, requestor);
            return 0;
        }
        return 1;
    }
    event_type_new=plugin_get_category_event(system);
    if (! event_type_new) {
        dbg(lvl_error,"unsupported event system '%s' requested from '%s'", system, requestor);
        return 0;
    }
    event_type_new(&event_methods);
    e_system=system;
    e_requestor=requestor;

    return 1;
}


