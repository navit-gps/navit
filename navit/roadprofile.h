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


#ifdef __cplusplus
extern "C" {
#endif
struct roadprofile {
    NAVIT_OBJECT
    int speed;
    int route_weight;
    int maxspeed;
};

struct roadprofile * roadprofile_new(struct attr *parent, struct attr **attrs);
int roadprofile_get_attr(struct roadprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int roadprofile_set_attr(struct roadprofile *this_, struct attr *attr);
int roadprofile_add_attr(struct roadprofile *this_, struct attr *attr);
int roadprofile_remove_attr(struct roadprofile *this_, struct attr *attr);
struct attr_iter *roadprofile_attr_iter_new(void* unused);
void roadprofile_attr_iter_destroy(struct attr_iter *iter);
#ifdef __cplusplus
}
#endif
