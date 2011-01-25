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

#ifndef NAVIT_PROFILE_H
#define NAVIT_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif
#define profile_str2(x) #x
#define profile_str1(x) profile_str2(x)
#define profile_module profile_str1(MODULE)
#define profile(level,...) profile_timer(level,profile_module,__PRETTY_FUNCTION__,__VA_ARGS__)
void profile_timer(int level, const char *module, const char *function, const char *fmt, ...);
#ifdef __cplusplus
}
#endif

#endif

