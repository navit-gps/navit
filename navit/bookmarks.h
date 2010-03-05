/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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

 #ifndef NAVIT_BOOKMARKS_H
 #define NAVIT_BOOKMARKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
struct bookmarks;
struct bookmarks *bookmarks_new(struct attr *parent,/* struct attr **attrs,*/ struct transformation *trans);
void bookmarks_destroy(struct bookmarks *this_);
void bookmarks_add_callback(struct bookmarks *this_, struct callback *cb);
int bookmarks_add_bookmark(struct bookmarks *this_, struct pcoord *c, const char *description);
int bookmarks_del_bookmark(struct bookmarks *this_, const char *description);
struct map* bookmarks_get_map(struct bookmarks *this_);

char* bookmarks_get_user_data_directory(gboolean create);
char* bookmarks_get_destination_file(gboolean create);
void bookmarks_set_center_from_file(struct bookmarks *this_, char *file);
char* bookmarks_get_center_file(gboolean create);
void bookmarks_write_center_to_file(struct bookmarks *this_, char *file);
void bookmarks_append_coord(struct bookmarks *this_, char *file, struct pcoord *c, const char *type, const char *description, GHashTable *h, int limit);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

 #endif /* NAVIT_BOOKMARKS_H */
