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
#include <time.h>
#include "messages.h"
#include "callback.h"
#include "event.h"
#include "attr.h"

struct messagelist {
    struct message *messages;	/**< All the messages that can currently be shown */
    int last_mid;							/**< Last Message ID */
    int maxage;								/**< Maximum age of messages */
    int maxnum;								/**< Maximum number of messages */
    struct callback *msg_cleanup_cb;			/**< Callback to clean up the messages */
    struct event_timeout *msg_cleanup_to;		/**< Idle event to clean up the messages */
};

int message_new(struct messagelist *this_, const char *message) {
    struct message *msg;

    msg = g_new0(struct message, 1);
    msg->text = g_strdup(message);
    msg->id = ++(this_->last_mid);
    msg->time = time(NULL);

    msg->next = this_->messages;
    this_->messages = msg;

    return msg->id;
}

int message_delete(struct messagelist *this_, int mid) {
    struct message *msg,*last;;

    msg = this_->messages;
    last = NULL;

    while (msg) {
        if (msg->id == mid) {
            break;
        }

        last = msg;
        msg = msg->next;
    }

    if (msg) {
        if (last) {
            last->next = msg->next;
        } else {
            this_->messages = msg->next;
        }

        g_free(msg->text);
        g_free(msg);

        return 1;
    } else {
        return 0;
    }
}

static void message_cleanup(struct messagelist *this_) {
    struct message *msg,*next,*prev=NULL;
    int i;
    time_t now;

    msg = this_->messages;

    now = time(NULL);

    i = 0;
    while (msg && (i < this_->maxnum)) {
        if ((this_->maxage > 0) && (now - msg->time) > this_->maxage) {
            break;
        }

        i++;
        prev = msg;
        msg = msg->next;
    }

    if (prev) {
        prev->next = NULL;
    } else {
        this_->messages = NULL;
    }

    while (msg) {
        next = msg->next;

        g_free(msg->text);
        g_free(msg);

        msg = next;
    }
}

struct messagelist
*messagelist_new(struct attr **attrs) {
    struct messagelist *this = g_new0(struct messagelist, 1);
    struct attr num_attr,age_attr;

    if (attr_generic_get_attr(attrs, NULL, attr_message_maxage, &age_attr, NULL)) {
        this->maxage = age_attr.u.num;
    } else {
        this->maxage = 10;
    }

    if (attr_generic_get_attr(attrs, NULL, attr_message_maxnum, &num_attr, NULL)) {
        this->maxnum = num_attr.u.num;
    } else {
        this->maxnum = 3;
    }

    return this;
}

void messagelist_init(struct messagelist *this_) {
    if (!event_system())
        return;
    this_->msg_cleanup_cb = callback_new_1(callback_cast(message_cleanup), this_);
    this_->msg_cleanup_to = event_add_timeout(1000, 1, this_->msg_cleanup_cb);
}

struct message
*message_get(struct messagelist *this_) {
    return this_->messages;
}
