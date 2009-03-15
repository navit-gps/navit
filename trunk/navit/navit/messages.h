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

#ifndef NAVIT_MESSAGES_H
#define NAVIT_MESSAGES_H

struct messagelist;

struct message {
	struct message *next;
	int id;
	time_t time;
	char *text;
};

/* Prototypes */
struct attr;

int message_new(struct messagelist *this_, char *message);
int message_delete(struct messagelist *this_, int mid);
struct messagelist *messagelist_new(struct attr **attrs);
void messagelist_init(struct messagelist *this_);
struct message *message_get(struct messagelist *this_);

#endif
