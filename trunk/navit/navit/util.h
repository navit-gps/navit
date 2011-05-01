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

#ifndef NAVIT_types_H
#define NAVIT_types_H

#include <ctype.h>
#include "config.h"

void strtoupper(char *dest, const char *src);
void strtolower(char *dest, const char *src);
GList * g_hash_to_list(GHashTable *h);
GList * g_hash_to_list_keys(GHashTable *h);
gchar * g_strconcat_printf(gchar *buffer, gchar *fmt, ...);
#if defined(_WIN32) || defined(__CEGCC__) || defined (__APPLE__) || defined(HAVE_API_ANDROID)
#if defined(_UNICODE)
wchar_t* newSysString(const char *toconvert);
#else
char * newSysString(const char *toconvert);
#endif
#endif
unsigned int iso8601_to_secs(char *iso8601);
char * current_to_iso8601(void);

#if defined(_MSC_VER) || (!defined(HAVE_GETTIMEOFDAY) && defined(HAVE_API_WIN32_BASE))

#include <winsock.h>

int gettimeofday(struct timeval *time, void *);

#endif

struct spawn_process_info;
char * shell_escape(char *arg);
struct spawn_process_info* spawn_process(char **argv);
int spawn_process_check_status(struct spawn_process_info *pi,int block);

void spawn_process_info_free(struct spawn_process_info *pi);
void spawn_process_init(void);

#endif

