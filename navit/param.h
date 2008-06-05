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

#ifndef NAVIT_PARAM_H
#define NAVIT_PARAM_H

struct param_list {
	char *name;
	char *value;	
};

void param_add_string(char *name, char *value, struct param_list **param, int *count);
void param_add_dec(char *name, unsigned long value, struct param_list **param, int *count);
void param_add_hex(char *name, unsigned long value, struct param_list **param, int *count);
void param_add_hex_sig(char *name, long value, struct param_list **param, int *count);

#endif

