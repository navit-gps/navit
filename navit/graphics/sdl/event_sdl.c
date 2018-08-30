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
#include <poll.h>
#include <SDL/SDL.h>
#include <pthread.h>
#include <signal.h>
#include "config.h"
#include "event.h"
#include "event_sdl.h"
#include "graphics_sdl.h"
#include "debug.h"
#include "callback.h"
#include "plugin.h"

static pthread_t sdl_watch_thread	= 0;
static GPtrArray *sdl_watch_list	= NULL;

static void event_sdl_watch_startthread(GPtrArray *);
static void event_sdl_watch_stopthread(void);
static struct event_watch *event_sdl_add_watch(int, enum event_watch_cond, struct callback *);
static void event_sdl_remove_watch(struct event_watch *);
static struct event_timeout *event_sdl_add_timeout(int, int, struct callback *);
static void event_sdl_remove_timeout(struct event_timeout *);
static struct event_idle *event_sdl_add_idle(int, struct callback *);
static void event_sdl_remove_idle(struct event_idle *);
static void event_sdl_call_callback(struct callback_list *);

static Uint32 sdl_timer_callback(Uint32 interval, void* param) {
    struct event_timeout *timeout=(struct event_timeout*)param;

    dbg(lvl_debug,"timer(%p) multi(%d) interval(%d) fired", param, timeout->multi, interval);

    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = SDL_USEREVENT_CODE_TIMER;
    userevent.data1 = timeout->cb;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent (&event);

    if (timeout->multi == 0) {
        timeout->id = 0;
        return 0; // cancel timer
    }
    return interval; // reactivate timer
}

/* SDL Mainloop */

static void event_sdl_main_loop_run(void) {
    graphics_sdl_idle(NULL);
    event_sdl_watch_stopthread();
}

static void event_sdl_main_loop_quit(void) {
    quit_event_loop = 1;
}

/* Watch */

void event_sdl_watch_thread (GPtrArray *watch_list) {
    struct pollfd *pfds = g_new0 (struct pollfd, watch_list->len);
    struct event_watch *ew;
    int ret;
    int idx;

    for (idx = 0; idx < watch_list->len; idx++ ) {
        ew = g_ptr_array_index (watch_list, idx);
        g_memmove (&pfds[idx], ew->pfd, sizeof(struct pollfd));
    }

    while ((ret = ppoll(pfds, watch_list->len, NULL, NULL)) > 0) {
        for (idx = 0; idx < watch_list->len; idx++ ) {
            if (pfds[idx].revents == pfds[idx].events) {	/* The requested event happened, notify mainloop! */
                ew = g_ptr_array_index (watch_list, idx);
                dbg(lvl_debug,"watch(%p) event(%d) encountered", ew, pfds[idx].revents);

                SDL_Event event;
                SDL_UserEvent userevent;

                userevent.type = SDL_USEREVENT;
                userevent.code = SDL_USEREVENT_CODE_WATCH;
                userevent.data1 = ew->cb;
                userevent.data2 = NULL;

                event.type = SDL_USEREVENT;
                event.user = userevent;

                SDL_PushEvent (&event);
            }
        }
    }

    g_free(pfds);

    pthread_exit(0);
}

static void event_sdl_watch_startthread(GPtrArray *watch_list) {
    dbg(lvl_debug,"enter");
    if (sdl_watch_thread)
        event_sdl_watch_stopthread();

    int ret;
    ret = pthread_create (&sdl_watch_thread, NULL, (void *)event_sdl_watch_thread, (void *)watch_list);

    dbg_assert (ret == 0);
}

static void event_sdl_watch_stopthread() {
    dbg(lvl_debug,"enter");
    if (sdl_watch_thread) {
        /* Notify the watch thread that the list of FDs will change */
        pthread_kill(sdl_watch_thread, SIGUSR1);
        pthread_join(sdl_watch_thread, NULL);
        sdl_watch_thread = 0;
    }
}

static struct event_watch *event_sdl_add_watch(int fd, enum event_watch_cond cond, struct callback *cb) {
    dbg(lvl_debug,"fd(%d) cond(%x) cb(%x)", fd, cond, cb);

    event_sdl_watch_stopthread();

    if (!sdl_watch_list)
        sdl_watch_list = g_ptr_array_new();

    struct event_watch *new_ew = g_new0 (struct event_watch, 1);
    struct pollfd *pfd = g_new0 (struct pollfd, 1);

    pfd->fd = fd;

    /* Modify watchlist here */
    switch (cond) {
    case event_watch_cond_read:
        pfd->events = POLLIN;
        break;
    case event_watch_cond_write:
        pfd->events = POLLOUT;
        break;
    case event_watch_cond_except:
        pfd->events = POLLERR|POLLHUP;
        break;
    }

    new_ew->pfd = (struct pollfd*) pfd;
    new_ew->cb = cb;

    g_ptr_array_add (sdl_watch_list, (gpointer)new_ew);

    event_sdl_watch_startthread(sdl_watch_list);

    return new_ew;
}

static void event_sdl_remove_watch(struct event_watch *ew) {
    dbg(lvl_debug,"enter %p",ew);

    event_sdl_watch_stopthread();

    g_ptr_array_remove (sdl_watch_list, ew);
    g_free (ew->pfd);
    g_free (ew);

    if (sdl_watch_list->len > 0)
        event_sdl_watch_startthread(sdl_watch_list);
}

/* Timeout */

static struct event_timeout *event_sdl_add_timeout(int timeout, int multi, struct callback *cb) {
    struct event_timeout * ret = g_new0(struct event_timeout, 1);
    if(!ret) {
        dbg(lvl_error,"g_new0 failed");
        return ret;
    }
    dbg(lvl_debug,"timer(%p) multi(%d) interval(%d) cb(%p) added",ret, multi, timeout, cb);
    ret->multi = multi;
    ret->cb = cb;
    ret->id = SDL_AddTimer(timeout, sdl_timer_callback, ret);

    return ret;
}

static void event_sdl_remove_timeout(struct event_timeout *to) {
    dbg(lvl_info,"enter %p", to);
    if(to) {
        /* do not SDL_RemoveTimer if oneshot timer has already fired */
        int ret = to->id == 0 ? SDL_TRUE : SDL_RemoveTimer(to->id);

        if (ret == SDL_FALSE)
            dbg(lvl_error,"SDL_RemoveTimer (%p) failed", to->id);

        g_free(to);
        dbg(lvl_debug,"timer(%p) removed", to);
    }
}

/* Idle */

/* sort ptr_array by priority, increasing order */
static gint sdl_sort_idle_tasks(gconstpointer parama, gconstpointer paramb) {
    struct idle_task *a = (struct idle_task *)parama;
    struct idle_task *b = (struct idle_task *)paramb;
    if (a->priority < b->priority)
        return -1;
    if (a->priority > b->priority)
        return 1;
    return 0;
}

static struct event_idle *event_sdl_add_idle(int priority, struct callback *cb) {
    dbg(lvl_debug,"add idle priority(%d) cb(%p)", priority, cb);

    struct idle_task *task = g_new0(struct idle_task, 1);
    task->priority = priority;
    task->cb = cb;

    g_ptr_array_add(idle_tasks, (gpointer)task);

    if (idle_tasks->len < 2) {
        SDL_Event event;
        SDL_UserEvent userevent;

        dbg(lvl_debug,"poking eventloop because of new idle_events");

        userevent.type = SDL_USEREVENT;
        userevent.code = SDL_USEREVENT_CODE_IDLE_EVENT;
        userevent.data1 = NULL;
        userevent.data2 = NULL;

        event.type = SDL_USEREVENT;
        event.user = userevent;

        SDL_PushEvent (&event);
    } else	// more than one entry => sort the list
        g_ptr_array_sort(idle_tasks, sdl_sort_idle_tasks);

    return (struct event_idle *)task;
}

static void event_sdl_remove_idle(struct event_idle *task) {
    dbg(lvl_debug,"remove task(%p)", task);
    g_ptr_array_remove(idle_tasks, (gpointer)task);
}

/* callback */

static void event_sdl_call_callback(struct callback_list *cbl) {
    dbg(lvl_debug,"call_callback cbl(%p)",cbl);
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = SDL_USEREVENT_CODE_CALL_CALLBACK;
    userevent.data1 = cbl;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent (&event);
}

static struct event_methods event_sdl_methods = {
    event_sdl_main_loop_run,
    event_sdl_main_loop_quit,
    event_sdl_add_watch,
    event_sdl_remove_watch,
    event_sdl_add_timeout,
    event_sdl_remove_timeout,
    event_sdl_add_idle,
    event_sdl_remove_idle,
    event_sdl_call_callback,
};

static struct event_priv *event_sdl_new(struct event_methods* methods) {
    idle_tasks = g_ptr_array_new();
    *methods = event_sdl_methods;
    return NULL;
}

void event_sdl_register(void) {
    plugin_register_category_event("sdl", event_sdl_new);
}
