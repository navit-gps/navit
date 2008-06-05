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

#ifndef NAVIT_COUNTRY_H
#define NAVIT_COUNTRY_H

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
struct attr;
struct country_search;
struct item;
struct country_search *country_search_new(struct attr *search, int partial);
struct item *country_search_get_item(struct country_search *this_);
struct attr *country_default(void);
void country_search_destroy(struct country_search *this_);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif
