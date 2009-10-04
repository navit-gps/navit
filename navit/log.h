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
#define LOG_BUFFER_SIZE 256
/**
 * printf-style writing to the log file. A buffer of LOG_BUFFER_SIZE
 * bytes is preallocated for the complete format message, longer
 * messages will be truncated.
 */

enum log_flags {
	log_flag_replace_buffer=1,
	log_flag_force_flush=2,
	log_flag_keep_pointer=4,
	log_flag_keep_buffer=8,
	log_flag_truncate=16,
};
/* prototypes */
enum attr_type;
enum log_flags;
struct attr;
struct attr_iter;
struct log;
int log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct log *log_new(struct attr *parent, struct attr **attrs);
void log_set_header(struct log *this_, char *data, int len);
void log_set_trailer(struct log *this_, char *data, int len);
void log_write(struct log *this_, char *data, int len, enum log_flags flags);
void *log_get_buffer(struct log *this_, int *len);
void log_printf(struct log *this_, char *fmt, ...);
void log_destroy(struct log *this_);
/* end of prototypes */
#endif
