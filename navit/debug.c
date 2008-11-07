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
#include <glib.h>
#include "file.h"
#include "item.h"
#include "debug.h"

#ifdef DEBUG_WIN32_CE_MESSAGEBOX
#include <windows.h>
#include <windowsx.h>
#endif

int debug_level=0,segv_level=0;
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
	debug_level=0;
	if (strcmp(name,"segv")) {
		g_hash_table_insert(debug_hash, g_strdup(name), (gpointer) level);
		g_hash_table_foreach(debug_hash, debug_update_level, NULL);	
	} else {
		segv_level=level;
		if (segv_level)
			signal(SIGSEGV, sigsegv);
		else
			signal(SIGSEGV, NULL);
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
#ifndef DEBUG_WIN32_CE_MESSAGEBOX
		if (! fp)
			fp = stderr;
		if (prefix)
			fprintf(fp,"%s:",buffer);
		vfprintf(fp,fmt, ap);
		fflush(fp);
#else
	wchar_t muni[4096];
	char xbuffer[4096];
	int len=0;
	if (prefix) {
		strcpy(xbuffer,buffer);
		len=strlen(buffer);
		xbuffer[len++]=':';
	}
	vsprintf(xbuffer+len,fmt,ap);
	mbstowcs(muni, xbuffer, strlen(xbuffer)+1);
	MessageBoxW(NULL, muni, TEXT("Navit - Error"), MB_APPLMODAL|MB_OK|MB_ICONERROR);
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
