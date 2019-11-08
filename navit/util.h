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
#include <time.h>
#include "config.h"

#define MAX_MISMATCH 100

/**
 * @brief Escape modes for function str_escape()
 */
enum escape_mode {
    escape_mode_none=0,
    escape_mode_string=1,	/*!< Surround string by double quotes */
    escape_mode_quote=2,	/*!< Escape double quotes and backslashes */
    escape_mode_html_amp=4,	/*!< Use HTML-style escape sequences for ampersands */
    escape_mode_html_quote=8,	/*!< Use HTML-style escape sequences for double quotes */
    escape_mode_html_apos=16,	/*!< Use HTML-style escape sequences for single quotes (apostrophes) */
    escape_mode_html_lt=32,	/*!< Use HTML-style escape sequences for lower than sign ('<') */
    escape_mode_html_gt=64,	/*!< Use HTML-style escape sequences for greater than sign ('>') */
    escape_mode_html=escape_mode_html_amp|escape_mode_html_quote|escape_mode_html_apos|escape_mode_html_lt|escape_mode_html_gt,	/*!< Use all known HTML-style escape sequences */
};

void strtoupper(char *dest, const char *src);
void strtolower(char *dest, const char *src);
unsigned int uint_sqrt(unsigned int n);
int navit_utf8_strcasecmp(const char *s1, const char *s2);
char *str_escape(enum escape_mode mode, const char *in);
char *strncpy_unescape(char *dest, const char *src, size_t n);
int compare_name_systematic(const char *s1, const char *s2);
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

void square_shape_str(char *s);

unsigned int iso8601_to_secs(char *iso8601);
time_t mkgmtime(struct tm * pt);
time_t iso8601_to_time(char * iso8601);
char * time_to_iso8601(time_t time);
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

void get_compass_direction(char *buffer, int angle, int mode);
