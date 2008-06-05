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
#include "event.h"

static GMainLoop *loop;

void event_main_loop_run(void)
{
	loop = g_main_loop_new (NULL, TRUE);
        if (g_main_loop_is_running (loop))
        {
		g_main_loop_run (loop);
	}
}

void event_main_loop_quit(void)
{
	if (loop)
		g_main_loop_quit(loop);
} 
