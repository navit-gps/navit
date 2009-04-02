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

struct vehicleprofile * vehicleprofile_new(struct attr *parent, struct attr **attrs);
int vehicleprofile_get_attr(struct vehicleprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int vehicleprofile_set_attr(struct vehicleprofile *this_, struct attr *attr);
int vehicleprofile_add_attr(struct vehicleprofile *this_, struct attr *attr);
int vehicleprofile_remove_attr(struct vehicleprofile *this_, struct attr *attr);
