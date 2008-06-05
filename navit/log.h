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

#ifndef NAVIT_LOG_H
#define NAVIT_LOG_H
/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct log;
int log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct log *log_new(struct attr **attrs);
void log_set_header(struct log *this_, char *data, int len);
void log_set_trailer(struct log *this_, char *data, int len);
void log_write(struct log *this_, char *data, int len);
void log_destroy(struct log *this_);
/* end of prototypes */
#endif
