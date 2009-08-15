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

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <glib.h>
#include "config.h"
#include "file.h"
#include "item.h"
#include "debug.h"

#ifdef HAVE_API_ANDROID
#include <android/log.h>
#endif

#ifdef DEBUG_WIN32_CE_MESSAGEBOX
#include <windows.h>
#include <windowsx.h>
#endif

int debug_level=0;
int segv_level=0;
int timestamp_prefix=0;

static int dummy;
static GHashTable *debug_hash;
static const char *gdb_program;

static FILE *debug_fp;

#if defined(_WIN32) || defined(__CEGCC__)

static void sigsegv(int sig)
{
}

#else
#include <unistd.h>
static void sigsegv(int sig)
{
	char buffer[256];
	if (segv_level > 1)
		sprintf(buffer, "gdb -ex bt %s %d", gdb_program, getpid());
	else
		sprintf(buffer, "gdb -ex bt -ex detach -ex quit %s %d", gdb_program, getpid());
	system(buffer);
	exit(1);
}
#endif

void
debug_init(const char *program_name)
{
	gdb_program=program_name;
	signal(SIGSEGV, sigsegv);
	debug_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	debug_fp = stderr;
}


static void
debug_update_level(gpointer key, gpointer value, gpointer user_data)
{
	if (debug_level < (int) value)
		debug_level=(int) value;
}

void
debug_level_set(const char *name, int level)
{
	if (!strcmp(name, "segv")) {
		segv_level=level;
		if (segv_level)
			signal(SIGSEGV, sigsegv);
		else
			signal(SIGSEGV, NULL);
	} else if (!strcmp(name, "timestamps")) {
		timestamp_prefix=level;
	} else {
		debug_level=0;
		g_hash_table_insert(debug_hash, g_strdup(name), (gpointer) level);
		g_hash_table_foreach(debug_hash, debug_update_level, NULL);
	}
}

struct debug *
debug_new(struct attr *parent, struct attr **attrs)
{
	struct attr *name,*level;
	name=attr_search(attrs, NULL, attr_name);
	level=attr_search(attrs, NULL, attr_level);
	if (!name || !level)
		return NULL;
	debug_level_set(name->u.str, level->u.num);
	return (struct debug *)&dummy;
}


int
debug_level_get(const char *name)
{
	if (!debug_hash)
		return 0;
	return (int)(g_hash_table_lookup(debug_hash, name));
}

static void debug_timestamp(FILE *fp)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1)
		return;
	/* Timestamps are UTC */
	fprintf(fp,
		"%02d:%02d:%02d.%03d|",
		(int)(tv.tv_sec/3600)%24,
		(int)(tv.tv_sec/60)%60,
		(int)tv.tv_sec % 60,
		(int)tv.tv_usec/1000);
}

void
debug_vprintf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap)
{
#ifdef HAVE_API_WIN32_CE
	char buffer[4096];
#else
	char buffer[mlen+flen+3];
#endif
	FILE *fp=debug_fp;

	sprintf(buffer, "%s:%s", module, function);
	if (debug_level_get(module) >= level || debug_level_get(buffer) >= level) {
#if defined(DEBUG_WIN32_CE_MESSAGEBOX) || defined(HAVE_API_ANDROID)
		char xbuffer[4096];
		wchar_t muni[4096];
		int len=0;
		if (prefix) {
			strcpy(xbuffer,buffer);
			len=strlen(buffer);
			xbuffer[len++]=':';
		}
		vsprintf(xbuffer+len,fmt,ap);
#endif
#ifdef DEBUG_WIN32_CE_MESSAGEBOX
		mbstowcs(muni, xbuffer, strlen(xbuffer)+1);
		MessageBoxW(NULL, muni, TEXT("Navit - Error"), MB_APPLMODAL|MB_OK|MB_ICONERROR);
#else
#ifdef HAVE_API_ANDROID
		__android_log_print(ANDROID_LOG_ERROR,"navit", "%s", xbuffer);
#else
		if (! fp)
			fp = stderr;
		if (timestamp_prefix)
			debug_timestamp(fp);
		if (prefix)
			fprintf(fp,"%s:",buffer);
		vfprintf(fp,fmt, ap);
		fflush(fp);
#endif
#endif
	}
}

void
debug_printf(int level, const char *module, const int mlen,const char *function, const int flen, int prefix, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	debug_vprintf(level, module, mlen, function, flen, prefix, fmt, ap);
	va_end(ap);
}

void
debug_assert_fail(char *module, const int mlen,const char *function, const int flen, char *file, int line, char *expr)
{
	debug_printf(0,module,mlen,function,flen,1,"%s:%d assertion failed:%s\n", file, line, expr);
	abort();
}

void
debug_destroy(void)
{
	if (!debug_fp)
		return;
	if (debug_fp == stderr || debug_fp == stdout)
		return;
	fclose(debug_fp);
	debug_fp = NULL;
}

void debug_set_logfile(const char *path)
{
	FILE *fp;
	fp = fopen(path, "a");
	if (fp) {
		debug_destroy();
		debug_fp = fp;
		fprintf(debug_fp, "Navit log started\n");
		fflush(debug_fp);
	}
}
