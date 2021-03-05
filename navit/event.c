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

#include "event.h"
#include "debug.h"
#include "plugin.h"
#include <stdlib.h>
#include <string.h>

static struct event_methods event_methods;
static const char *e_requestor;
static const char *e_system;

static int has_quit;

#define require_method_helper(m)                                                                                       \
    if (!event_methods.m) {                                                                                            \
        dbg(lvl_error, "Can't find event system method %s. Event system is %s%s", #m,                                  \
            e_system ? "set to " : "not set.", e_system ? e_system : "");

#define require_method(m)                                                                                              \
    require_method_helper(m) return;                                                                                   \
    }

#define require_method2(m, r)                                                                                          \
    require_method_helper(m) return r;                                                                                 \
    }

/**
 * @brief Starts the main event loop.
 */
void event_main_loop_run(void) {
    require_method(main_loop_run);
    event_methods.main_loop_run();
}

/**
 * @brief Stops the main event loop.
 */
void event_main_loop_quit(void) {
    if (event_methods.main_loop_quit)
        event_methods.main_loop_quit();
    has_quit = 1;
}

/**
 * @brief Returns whether the main event loop has quit.
 */
int event_main_loop_has_quit(void) {
    return has_quit;
}

/**
 * @brief Adds a watch event which will fire whenever a certain condition occurs on a file descriptor.
 *
 * @param fd The file descriptor to watch
 * @param cond The condition to watch for
 * @param cb The callback to call when the event fires
 *
 * @returns The watch event
 */
struct event_watch *event_add_watch(int fd, enum event_watch_cond cond, struct callback *cb) {
    require_method2(add_watch, NULL);
    return event_methods.add_watch(fd, cond, cb);
}

/**
 * @brief Removes a previously added watch event.
 *
 * @param ev The event, as returned by `event_add_watch()`
 */
void event_remove_watch(struct event_watch *ev) {
    require_method(remove_watch);
    event_methods.remove_watch(ev);
}

/**
 * @brief Adds an event which will fire when the specified timeout expires.
 *
 * After a one-shot event has fired, or in order to stop a multi-shot event from firing again, it must
 * be removed by calling `event_remove_timeout()`.
 *
 * @param timeout Timeout in msec
 * @param multi Whether the event should fire repeatedly (at the interval specified by `timeout`)
 * @param cb The callback to call when `timeout` expires
 *
 * @returns The timeout event
 */
struct event_timeout *event_add_timeout(int timeout, int multi, struct callback *cb) {
    require_method2(add_timeout, NULL);
    return event_methods.add_timeout(timeout, multi, cb);
}

/**
 * @brief Removes a previously added timeout event.
 *
 * @param ev The event, as returned by `event_add_timeout()`
 */
void event_remove_timeout(struct event_timeout *ev) {
    require_method(remove_timeout);
    event_methods.remove_timeout(ev);
}

/**
 * @brief Adds an event which will run repeatedly when the event system is idle.
 *
 * This is typically done to run long loops in the background without blocking the UI: the loop is
 * wrapped into a callback and exits after a certain time or number of runs. Upon its next invocation,
 * it takes up where it interrupted its work. Each invocation should complete in a period of time short
 * enough to not cause a significant delay to the UI. After the last run of the loop, the callback
 * removes its event.
 *
 * This can be seen as a lightweight alternative to multithreading. It is available even on platforms
 * without multithreading support, and synchronization is easier as the idle callback has full control
 * over when it yields to other tasks, and can ensure all data is in a consistent state before doing so.
 * However, idle events not suited to all tasks, and poorly written callbacks can cause the UI to become
 * unresponsive. As they un on the main thread, they cannot benefit from multi-processor systems.
 *
 * @param priority The priority for the event, if multiple idle events are present at the same time
 * @param cb The callback to call when the event fires
 *
 * @returns The idle event
 */
struct event_idle *event_add_idle(int priority, struct callback *cb) {
    require_method2(add_idle, NULL);
    return event_methods.add_idle(priority, cb);
}

/**
 * @brief Removes a previously added idle event.
 *
 * @param ev The event, as returned by `event_add_idle()`
 */
void event_remove_idle(struct event_idle *ev) {
    require_method(remove_idle);
    event_methods.remove_idle(ev);
}

/**
 * @brief Calls a list of callbacks.
 *
 * This is not implemented by all event systems.
 */
void event_call_callback(struct callback_list *cb) {
    require_method(call_callback);
    event_methods.call_callback(cb);
}

/**
 * @brief Returns the current event system.
 */
char const *event_system(void) {
    return e_system;
}

/**
 * @brief Initializes a new event system.
 *
 * This call will fail if an event system other than the one requested has already been initialized, or
 * if `system` does not match a supported event system.
 *
 * @param system The event system to initialize.
 * @param requestor A string identifying the requestor, which can be freely chosen by the caller.
 *
 * @return true on success, false on failure
 */
int event_request_system(const char *system, const char *requestor) {
    void (*event_type_new)(struct event_methods *meth);
    if (e_system) {
        if (strcmp(e_system, system)) {
            dbg(lvl_error, "system '%s' already requested by '%s', can't set to '%s' as requested from '%s'", e_system,
                e_requestor, system, requestor);
            return 0;
        }
        return 1;
    }
    event_type_new = plugin_get_category_event(system);
    if (!event_type_new) {
        dbg(lvl_error, "unsupported event system '%s' requested from '%s'", system, requestor);
        return 0;
    }
    event_type_new(&event_methods);
    e_system = system;
    e_requestor = requestor;

    return 1;
}
