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
void cursor_draw(struct cursor *this_, struct point *pnt, int dir, int draw_dir, int force);
struct cursor *cursor_new(struct graphics *gra, struct color *c, struct color *c2, int animate);
void cursor_destroy(struct cursor *this_);
/* end of prototypes */

#endif
