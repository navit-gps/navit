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

#ifndef NAVIT_CURSOR_H
#define NAVIT_CURSOR_H

/* prototypes */
struct color;
struct cursor;
struct graphics;
struct point;
void cursor_draw(struct cursor *this_, struct graphics *gra, struct point *pnt, int lazy, int dir, int speed);
int cursor_add_attr(struct cursor *this_, struct attr *attr);
struct cursor *cursor_new(struct attr *parent, struct attr **attrs);
void cursor_destroy(struct cursor *this_);
/* end of prototypes */

#endif
