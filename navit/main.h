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

#ifndef NAVIT_MAIN_H
#define NAVIT_MAIN_H

/* prototypes */
struct navit;
struct iter;
struct iter * main_iter_new(void);
void main_iter_destroy(struct iter *iter);
struct navit * main_get_navit(struct iter *iter);
void main_add_navit(struct navit *nav);
void main_remove_navit(struct navit *nav);
void main_init(char *program);
void main_init_nls(void);
int main(int argc, char **argv);
/* end of prototypes */

#endif

