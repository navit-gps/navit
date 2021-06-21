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
#include "qt5graphicsworker.h"

static void event_qt5_remove_timeout(struct event_timeout* to);

Qt5GraphicsWorker* qt5_timer = nullptr;
QThread *qt5_event_thread = nullptr;

static void event_qt5_main_loop_run(void) {
    qDebug() << "Starting mainloop";
    if (navit_app != nullptr)
        navit_app->exec();
}

static void event_qt5_main_loop_quit(void) {
    qDebug() << "Quitting mainloop";

    exit(0);
}

static struct event_watch* event_qt5_add_watch(int fd, enum event_watch_cond cond, struct callback* cb) {
    qDebug() << "Adding watch : " << fd;
    struct Qt5GraphicsWorker::eventWatch *ew = (Qt5GraphicsWorker::eventWatch*) malloc( sizeof(struct Qt5GraphicsWorker::eventWatch));
    ew->fd = fd;
    ew->cb = cb;

    qt5_timer->m_watches.insert(fd, ew);
    ew->sn = new QSocketNotifier(fd, QSocketNotifier::Read, qt5_timer);
    QObject::connect(ew->sn, SIGNAL(activated(int)), qt5_timer, SLOT(watchEvent(int)));
    return (event_watch*) ew;
}

static void event_qt5_remove_watch(struct event_watch* ew) {
    Qt5GraphicsWorker::eventWatch *watch = (Qt5GraphicsWorker::eventWatch*)ew;
    qDebug() << "Removing watch : " << watch->fd;
    qt5_timer->m_watches.remove(watch->fd);
    delete (watch->sn);
    g_free(watch);
}

static struct event_timeout* event_qt5_add_timeout(int timeout, int multi, struct callback* cb) {
    struct Qt5GraphicsWorker::eventTimeout *et = (Qt5GraphicsWorker::eventTimeout*) malloc( sizeof(struct Qt5GraphicsWorker::eventTimeout));
    et->type = !!multi;
    et->cb = cb;

    emit qt5_timer->addTimeout(et, timeout);
    return (event_timeout*) et;
}

static void event_qt5_remove_timeout(struct event_timeout* et) {
    Qt5GraphicsWorker::eventTimeout *timeout = (Qt5GraphicsWorker::eventTimeout*)et;
    qDebug() << "Removing timeout : " << timeout->id;
    emit qt5_timer->removeTimeout(timeout);

}

static struct event_idle* event_qt5_add_idle(int priority, struct callback* cb) {
    struct Qt5GraphicsWorker::eventIdle *et = (Qt5GraphicsWorker::eventIdle*) malloc( sizeof(struct Qt5GraphicsWorker::eventIdle));
    et->cb = cb;

    emit qt5_timer->addIdle(et);
    return (struct event_idle*) et;
}

static void event_qt5_remove_idle(struct event_idle* ev) {
    Qt5GraphicsWorker::eventIdle *idle = (Qt5GraphicsWorker::eventIdle*)ev;
    qDebug() << "Removing idle : " << idle->id;
    emit qt5_timer->removeIdle(idle);
}

static void event_qt5_call_callback(struct callback_list* cb) {
    qDebug() << "event_qt5_call_callback";
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
    QThread::currentThread()->setObjectName("main");
    *meth = event_qt5_methods;

    qDebug() << "Starting event thread : " << QThread::currentThread();

    if(!qt5_timer){
        qt5_timer = new Qt5GraphicsWorker();
    }

    if(!qt5_event_thread){
        qt5_event_thread = new QThread;
        qt5_event_thread->setObjectName("event thread");
        qt5_event_thread->start();
    }
    qt5_timer->moveToThread(qt5_event_thread);

    return nullptr;
}

void qt5_event_init(void) {
    plugin_register_category_event("qt5", event_qt5_new);
}
