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

#ifndef BINDING_WIN32_H
#define BINDING_WIN32_H

#define NAVIT_BINDING_W32_DWDATA 1
#define NAVIT_BINDING_W32_MAGIC "NavIt"
#define NAVIT_BINDING_W32_VERSION 1

struct navit_binding_w32_msg {
	/* Structure version number, should be equal to NAVIT_BINDING_W32_VERSION */
	int version;
	/* Magic code to filter out packets directed to other applications and [mistakely] sent to us or broadcasted.
	 * should be equal to NAVIT_BINDING_W32_MAGIC  */
	char magic[6];
	/* Command to be executed by Navit */
	char text[1];
};

#endif
