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

#ifndef NAVIT_SEARCH_H
#define NAVIT_SEARCH_H

#ifdef __cplusplus
extern "C" {
#endif
struct search_list_common {
	struct item item;
	int selected;
	struct pcoord *c;
};

struct search_list_country {
	struct item item;
	int selected;
	struct pcoord *c;
	char *car;
	char *iso2;
	char *iso3;
	char *name;
	char *flag;
};

struct search_list_town {
	struct item item;
	int selected;
	struct pcoord *c;
	struct item itemt;
	char *postal;
	char *name;
	char *district;
};

struct search_list_street {
	struct item item;
	int selected;
	struct pcoord *c;
	char *name;
};

struct search_list_result {
	int id;
	struct pcoord *c;
	struct search_list_country *country;
	struct search_list_town *town;
	struct search_list_street *street;
};

/* prototypes */
struct attr;
struct mapset;
struct search_list;
struct search_list_result;
struct search_list *search_list_new(struct mapset *ms);
void search_list_search(struct search_list *this_, struct attr *search_attr, int partial);
int search_list_select(struct search_list *this_, enum attr_type attr_type, int id, int mode);
struct search_list_result *search_list_get_result(struct search_list *this_);
void search_list_destroy(struct search_list *this_);
void search_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

