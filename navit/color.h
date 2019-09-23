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

#ifndef NAVIT_COLOR_H
#define NAVIT_COLOR_H

struct color {
    int r,g,b,a;
};

#define COLOR_BITDEPTH 16
#define COLOR_WHITE_ 0xffff,0xffff,0xffff,0xffff
#define COLOR_BLACK_ 0x0000,0x0000,0x0000,0xffff
#define COLOR_BACKGROUND_ 0xffff, 0xefef, 0xb7b7, 0xffff
#define COLOR_TRANSPARENT__ 0x0000,0x0000,0x0000,0xffff
#define COLOR_WHITE ((struct color) {COLOR_WHITE_})
#define COLOR_BLACK ((struct color) {COLOR_BLACK_})
#define COLOR_TRANSPARENT ((struct color) {COLOR_TRANSPARENT_})
#define COLOR_FMT "0x%x,0x%x,0x%x,0x%x"
#define COLOR_ARGS(c) (c).r,(c).g,(c).b,(c).a
/*default alpha value to apply for all things flagged AF_UNDERGROUND
 *use solid color to not change default behaviour*/
#define UNDERGROUND_ALPHA_ 0xFFFF

#define COLOR_IS_SAME(c1,c2) ((c1).r==(c2).r && (c1).g==(c2).g && (c1).b==(c2).b && (c1).a==(c2).a)
#define COLOR_IS_WHITE(c) COLOR_IS_SAME(c, COLOR_WHITE)
#define COLOR_IS_BLACK(c) COLOR_IS_SAME(c, COLOR_BLACK)

#endif
