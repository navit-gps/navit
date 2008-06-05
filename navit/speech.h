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

#ifndef NAVIT_SPEECH_H
#define NAVIT_SPEECH_H

struct speech_priv;

struct speech_methods {
	void (*destroy)(struct speech_priv *this_);
	int (*say)(struct speech_priv *this_, const char *text);
};

/* prototypes */
struct speech * speech_new(const char *type, const char *data);
int speech_say(struct speech *this_, const char *text);
int speech_sayf(struct speech *this_, const char *format, ...);
void speech_destroy(struct speech *this_);
/* end of prototypes */

#endif

