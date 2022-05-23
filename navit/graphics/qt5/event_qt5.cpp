/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2017 Navit Team
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
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "config.h"

#include "navit/callback.h"
#include "navit/color.h"
#include "navit/debug.h"
#include "navit/event.h"

#include "navit/point.h" /* needs to be before graphics.h */

#include "navit/graphics.h"
#include "navit/item.h"
#include "navit/keys.h"
#include "navit/navit.h"
#include "navit/plugin.h"
#include "navit/window.h"
}

#if defined(WINDOWS) || defined(WIN32) || defined(HAVE_API_WIN32_CE)
#include <windows.h>
#endif

#include "event_qt5.h"
//#include "event_qt5.moc"
#include "graphics_qt5.h"
#include <QSocketNotifier>

struct event_watch {
    QSocketNotifier* sn;
    struct callback* cb;
    int fd;
};

static void event_qt5_remove_timeout(struct event_timeout* to);

qt5_navit_timer::qt5_navit_timer(QObject* parent)
    : QObject(parent) {
    timer_type = g_hash_table_new(NULL, NULL);
    timer_callback = g_hash_table_new(NULL, NULL);
    watches = g_hash_table_new(NULL, NULL);
    dbg(lvl_debug, "qt5_navit_timer object created");
}

void qt5_navit_timer::watchEvent(int id) {
    struct event_watch* ret = g_new0(struct event_watch, 1);
    ret = (struct event_watch*)g_hash_table_lookup(watches, (void*)(long)id);
    if (ret) {
        dbg(lvl_debug, "callback found, calling it");
        callback_call_0(ret->cb);
    }
}

void qt5_navit_timer::timerEvent(QTimerEvent* event) {
    int id = event->timerId();
    void* multi = NULL;
    //        dbg(lvl_debug, "TimerEvent (%d)", id);
    struct callback* cb = (struct callback*)g_hash_table_lookup(timer_callback, (void*)(long)id);
    if (cb)
        callback_call_0(cb);
    /* remove timer if it was oneshot timer */
    if (g_hash_table_lookup_extended(timer_type, (void*)(long)id, NULL, &multi)) {
        /* it's still in the list */
        if (((int)(long)multi) == 0)
            event_qt5_remove_timeout((struct event_timeout*)(long)id);
    }
    //        dbg(lvl_debug, "TimerEvent (%d) leave", id);
}

qt5_navit_timer* qt5_timer = NULL;

static void event_qt5_main_loop_run(void) {
    dbg(lvl_debug, "enter");
    if (navit_app != NULL)
        navit_app->exec();
}

static void event_qt5_main_loop_quit(void) {
    dbg(lvl_debug, "enter");
    exit(0);
}

static struct event_watch* event_qt5_add_watch(int fd, enum event_watch_cond cond, struct callback* cb) {
    dbg(lvl_debug, "enter fd=%d", (int)(long)fd);
    struct event_watch* ret = g_new0(struct event_watch, 1);
    ret->fd = fd;
    ret->cb = cb;
    g_hash_table_insert(qt5_timer->watches, GINT_TO_POINTER(fd), ret);
    ret->sn = new QSocketNotifier(fd, QSocketNotifier::Read, qt5_timer);
    QObject::connect(ret->sn, SIGNAL(activated(int)), qt5_timer, SLOT(watchEvent(int)));
    return ret;
}

static void event_qt5_remove_watch(struct event_watch* ev) {
    dbg(lvl_debug, "enter");
    g_hash_table_remove(qt5_timer->watches, GINT_TO_POINTER(ev->fd));
    delete (ev->sn);
    g_free(ev);
}

static struct event_timeout* event_qt5_add_timeout(int timeout, int multi, struct callback* cb) {
    int id;
    dbg(lvl_debug, "add timeout %d, mul %d, %p ==", timeout, multi, cb);
    id = qt5_timer->startTimer(timeout);
    dbg(lvl_debug, "%d", id);
    g_hash_table_insert(qt5_timer->timer_callback, (void*)(long)id, cb);
    g_hash_table_insert(qt5_timer->timer_type, (void*)(long)id, (void*)(long)!!multi);
    return (struct event_timeout*)(long)id;
}

static void event_qt5_remove_timeout(struct event_timeout* to) {
    dbg(lvl_debug, "remove timeout (%d)", (int)(long)to);
    qt5_timer->killTimer((int)(long)to);
    g_hash_table_remove(qt5_timer->timer_callback, to);
    g_hash_table_remove(qt5_timer->timer_type, to);
}

static struct event_idle* event_qt5_add_idle(int priority, struct callback* cb) {
    dbg(lvl_debug, "add idle event");
    return (struct event_idle*)event_qt5_add_timeout(0, 1, cb);
}

static void event_qt5_remove_idle(struct event_idle* ev) {
    dbg(lvl_debug, "Remove idle timeout");
    event_qt5_remove_timeout((struct event_timeout*)ev);
}

static void event_qt5_call_callback(struct callback_list* cb) {
    dbg(lvl_debug, "enter");
}

static struct event_methods event_qt5_methods = {
    event_qt5_main_loop_run,
    event_qt5_main_loop_quit,
    event_qt5_add_watch,
    event_qt5_remove_watch,
    event_qt5_add_timeout,
    event_qt5_remove_timeout,
    event_qt5_add_idle,
    event_qt5_remove_idle,
    event_qt5_call_callback,
};

static struct event_priv* event_qt5_new(struct event_methods* meth) {
    *meth = event_qt5_methods;
    qt5_timer = new qt5_navit_timer(NULL);
    return NULL;
}

void qt5_event_init(void) {
    plugin_register_category_event("qt5", event_qt5_new);
}
