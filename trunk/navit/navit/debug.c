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

#ifdef HAVE_API_WIN32_CE
#include <windows.h>
#include <windowsx.h>
#endif

int debug_level=0;
int segv_level=0;
int timestamp_prefix=0;

static int dummy;
static GHashTable *debug_hash;
static const gchar *gdb_program;

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
	gdb_program=g_strdup(program_name);
	signal(SIGSEGV, sigsegv);
	debug_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	debug_fp = stderr;
}


static void
debug_update_level(gpointer key, gpointer value, gpointer user_data)
{
	if (debug_level < GPOINTER_TO_INT(value))
		debug_level = GPOINTER_TO_INT(value);
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
		g_hash_table_insert(debug_hash, g_strdup(name), GINT_TO_POINTER(level));
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
	return GPOINTER_TO_INT(g_hash_table_lookup(debug_hash, name));
}

static void debug_timestamp(FILE *fp)
{
#ifdef HAVE_API_WIN32_CE
	LARGE_INTEGER counter, frequency;
	double val;
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&frequency);
	val=counter.HighPart * 4294967296.0 + counter.LowPart;
	val/=frequency.HighPart * 4294967296.0 + frequency.LowPart;
	fprintf(fp,"%.6f|",val);

#else
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
#endif
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
debug_assert_fail(const char *module, const int mlen,const char *function, const int flen, const char *file, int line, const char *expr)
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

struct malloc_head {
	int magic;
	int size;
	char *where;
	void *return_address[8];
	struct malloc_head *prev;
	struct malloc_head *next;
} *malloc_heads;

struct malloc_tail {
	int magic;
};

int mallocs,malloc_size,malloc_size_m;

void
debug_dump_mallocs(void)
{
	struct malloc_head *head=malloc_heads;
	int i;
	dbg(0,"mallocs %d\n",mallocs);
	while (head) {
		fprintf(stderr,"unfreed malloc from %s of size %d\n",head->where,head->size);
		for (i = 0 ; i < 8 ; i++)
			fprintf(stderr,"\tlist *%p\n",head->return_address[i]);
#if 0
		fprintf(stderr,"%s\n",head+1);
#endif
		head=head->next;
	}
}

void *
debug_malloc(const char *where, int line, const char *func, int size)
{
	struct malloc_head *head;
	struct malloc_tail *tail;
	if (!size)
		return NULL;
	mallocs++;
	malloc_size+=size;
	if (malloc_size/(1024*1024) != malloc_size_m) {
		malloc_size_m=malloc_size/(1024*1024);
		dbg(0,"malloced %d kb\n",malloc_size/1024);
	}
	head=malloc(size+sizeof(*head)+sizeof(*tail));
	head->magic=0xdeadbeef;
	head->size=size;
	head->prev=NULL;
	head->next=malloc_heads;
	malloc_heads=head;
	if (head->next) 
		head->next->prev=head;
	head->where=g_strdup_printf("%s:%d %s",where,line,func);
	head->return_address[0]=__builtin_return_address(0);
	head->return_address[1]=__builtin_return_address(1);
	head->return_address[2]=__builtin_return_address(2);
	head->return_address[3]=__builtin_return_address(3);
	head->return_address[4]=__builtin_return_address(4);
	head->return_address[5]=__builtin_return_address(5);
	head->return_address[6]=__builtin_return_address(6);
	head->return_address[7]=__builtin_return_address(7);
	head++;
	tail=(struct malloc_tail *)((unsigned char *)head+size);
	tail->magic=0xdeadbef0;
	return head;
}


void *
debug_malloc0(const char *where, int line, const char *func, int size)
{
	void *ret=debug_malloc(where, line, func, size);
	if (ret)
		memset(ret, 0, size);
	return ret;
}

void *
debug_realloc(const char *where, int line, const char *func, void *ptr, int size)
{
	void *ret=debug_malloc(where, line, func, size);
	if (ret && ptr)
		memcpy(ret, ptr, size);
	debug_free(where, line, func, ptr);
	return ret;
}

char *
debug_strdup(const char *where, int line, const char *func, const char *ptr)
{ 
	int size;
	char *ret;

	if (!ptr)
		return NULL;
	size=strlen(ptr)+1;
	ret=debug_malloc(where, line, func, size);
	memcpy(ret, ptr, size);
	return ret;
}

char *
debug_guard(const char *where, int line, const char *func, char *str)
{
	char *ret=debug_strdup(where, line, func, str);
	g_free(str);
	return ret;
}

void
debug_free(const char *where, int line, const char *func, void *ptr)
{
	struct malloc_head *head,*tail;
	if (!ptr)
		return;
	mallocs--;
	head=(struct malloc_head *)((unsigned char *)ptr-sizeof(*head));
	tail=(struct malloc_tail *)((unsigned char *)ptr+head->size);
	malloc_size-=head->size;
	if (head->magic != 0xdeadbeef || tail->magic != 0xdeadbef0) {
		fprintf(stderr,"Invalid free from %s:%d %s\n",where,line,func);
	}
	head->magic=0;
	tail->magic=0;
	if (head->prev) 
		head->prev->next=head->next;
	else
		malloc_heads=head->next;
	if (head->next)
		head->next->prev=head->prev;
	free(head->where);
	free(head);
}

void
debug_free_func(void *ptr)
{
	debug_free("unknown",0,"unknown",ptr);
}

void debug_finished(void) {
	debug_dump_mallocs();
	g_free(gdb_program);
	g_hash_table_destroy(debug_hash);
	debug_destroy();
}

