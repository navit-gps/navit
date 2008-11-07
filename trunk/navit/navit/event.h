/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct event_idle;
struct event_timeout;
struct event_watch;
struct callback;
struct file;

enum event_watch_cond {
	event_watch_cond_read=1,
	event_watch_cond_write,
	event_watch_cond_except,
};

struct event_methods {
	void (*main_loop_run)(void);
	void (*main_loop_quit)(void);
	struct event_watch *(*add_watch)(struct file *file, enum event_watch_cond cond, struct callback *cb);
	void (*remove_watch)(struct event_watch *ev);
	struct event_timeout *(*add_timeout)(int timeout, int multi, struct callback *cb);
	void (*remove_timeout)(struct event_timeout *ev);
	struct event_idle *(*add_idle)(struct callback *cb);
	void (*remove_idle)(struct event_idle *ev);
	void (*call_callback)(struct callback *cb);
};


/* prototypes */
void event_main_loop_run(void);
void event_main_loop_quit(void);
struct event_watch *event_add_watch(struct file *file, enum event_watch_cond cond, struct callback *cb);
void event_remove_watch(struct event_watch *ev);
struct event_timeout *event_add_timeout(int timeout, int multi, struct callback *cb);
void event_remove_timeout(struct event_timeout *ev);
struct event_idle *event_add_idle(struct callback *cb);
void event_remove_idle(struct event_idle *ev);
int event_request_system(char *system, char *requestor);
void event_call_callback(struct callback *cb);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
