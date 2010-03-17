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

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <glib.h>
#include "file.h"
#include "item.h"
#include "event.h"
#include "callback.h"
#include "debug.h"
#include "log.h"

struct log_data {
	int len;
	int max_len;
	char *data;
};

struct log {
	FILE *f;
	int overwrite;
	int empty;
	int lazy;
	int mkdir;
	int flush_size;
	int flush_time;
	struct event_timeout *timer;
	struct callback *timer_callback;
	struct timeval last_flush;
	char *filename;
	char *filename_ex1;
	char *filename_ex2;
	struct log_data header;
	struct log_data data;
	struct log_data trailer;
	struct attr **attrs;
};

static void
strftime_localtime(char *buffer, int size, char *fmt)
{
	time_t t;
	struct tm *tm;

	t=time(NULL);
	tm=localtime(&t);
	strftime(buffer, 4096, fmt, tm);
}

static void
expand_filenames(struct log *this_)
{
	char *pos,buffer[4096];
	int i;

	strftime_localtime(buffer, 4096, this_->filename);
	this_->filename_ex1=g_strdup(buffer);
	if ((pos=strstr(this_->filename_ex1,"%i"))) {
#ifdef HAVE_API_ANDROID
		pos[1]='d';
#endif
		i=0;
		do {
			g_free(this_->filename_ex2);
			this_->filename_ex2=g_strdup_printf(this_->filename_ex1,i++);
		} while (file_exists(this_->filename_ex2));
#ifdef HAVE_API_ANDROID
		pos[1]='i';
#endif
	} else 
		this_->filename_ex2=g_strdup(this_->filename_ex1);
}

static void
log_set_last_flush(struct log *this_)
{
	gettimeofday(&this_->last_flush, NULL);
}

static void
log_open(struct log *this_)
{
	char *mode;
	if (this_->overwrite)
		mode="w";
	else
		mode="r+";
	if (this_->mkdir)
		file_mkdir(this_->filename_ex2, 2);
	this_->f=fopen(this_->filename_ex2, mode);
	if (! this_->f)
		this_->f=fopen(this_->filename_ex2, "w");
	if (! this_->f)
		return;
	if (!this_->overwrite) 
		fseek(this_->f, 0, SEEK_END);
	this_->empty = !ftell(this_->f);
	log_set_last_flush(this_);
}

static void
log_close(struct log *this_)
{
	if (! this_->f)
		return;
	if (this_->trailer.len) 
		fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
	fflush(this_->f);
	fclose(this_->f);
	this_->f=NULL;
}

static void
log_flush(struct log *this_, enum log_flags flags)
{
	long pos;
	if (this_->lazy && !this_->f) {
		if (!this_->data.len)
			return;
		log_open(this_);
	}
	if (! this_->f)
		return;
	if (this_->empty) {
		if (this_->header.len) 
			fwrite(this_->header.data, 1, this_->header.len, this_->f);
		if (this_->header.len || this_->data.len)
			this_->empty=0;
	}
	fwrite(this_->data.data, 1, this_->data.len, this_->f);
#ifndef HAVE_API_WIN32_BASE
	if (flags & log_flag_truncate) {
		pos=ftell(this_->f);
		ftruncate(fileno(this_->f), pos);
	}
#endif
	if (this_->trailer.len) {
		pos=ftell(this_->f);
		if (pos > 0) {
			fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
			fseek(this_->f, pos, SEEK_SET);	
		}
	}
	if (flags & log_flag_keep_pointer)
		fseek(this_->f, -this_->data.len, SEEK_CUR);
	fflush(this_->f);
	if (!(flags & log_flag_keep_buffer)) {
		g_free(this_->data.data);
		this_->data.data=NULL;
		this_->data.max_len=this_->data.len=0;
	}
	log_set_last_flush(this_);
}

static int
log_flush_required(struct log *this_)
{
	return this_->data.len > this_->flush_size;
}


static void
log_change(struct log *this_)
{
	log_flush(this_,0);
	log_close(this_);
	expand_filenames(this_);
	if (! this_->lazy)
		log_open(this_);
}

static int
log_change_required(struct log *this_)
{
	char buffer[4096];

	strftime_localtime(buffer, 4096, this_->filename);
	return (strcmp(this_->filename_ex1, buffer) != 0);
}

static void
log_timer(struct log *this_)
{
	struct timeval tv;
	int delta;
	gettimeofday(&tv, NULL);
	delta=(tv.tv_sec-this_->last_flush.tv_sec)*1000+(tv.tv_usec-this_->last_flush.tv_usec)/1000;
	dbg(1,"delta=%d flush_time=%d\n", delta, this_->flush_time);
	if (this_->flush_time && delta >= this_->flush_time*1000)
		log_flush(this_,0);
}

int
log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
        return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}


struct log *
log_new(struct attr * parent,struct attr **attrs)
{
	struct log *ret=g_new0(struct log, 1);
	struct attr *data,*overwrite,*lazy,*mkdir,*flush_size,*flush_time;
	struct file_wordexp *wexp;
	char *filename, **wexp_data;

	dbg(1,"enter\n");
	data=attr_search(attrs, NULL, attr_data);
	if (! data)
		return NULL;
	filename=data->u.str;
	wexp=file_wordexp_new(filename);
	if (wexp && file_wordexp_get_count(wexp) > 0) {
		wexp_data=file_wordexp_get_array(wexp);
		filename=wexp_data[0];
	}
	if (filename)
		ret->filename=g_strdup(filename);
	if (wexp)
		file_wordexp_destroy(wexp);
	overwrite=attr_search(attrs, NULL, attr_overwrite);
	if (overwrite)
		ret->overwrite=overwrite->u.num;
	lazy=attr_search(attrs, NULL, attr_lazy);
	if (lazy)
		ret->lazy=lazy->u.num;
	mkdir=attr_search(attrs, NULL, attr_mkdir);
	if (mkdir)
		ret->mkdir=mkdir->u.num;
	flush_size=attr_search(attrs, NULL, attr_flush_size);
	if (flush_size)
		ret->flush_size=flush_size->u.num;
	flush_time=attr_search(attrs, NULL, attr_flush_time);
	if (flush_time)
		ret->flush_time=flush_time->u.num;
	if (ret->flush_time) {
		dbg(1,"interval %d\n", ret->flush_time*1000);
		ret->timer_callback=callback_new_1(callback_cast(log_timer), ret);
		ret->timer=event_add_timeout(ret->flush_time*1000, 1, ret->timer_callback);
	}
	expand_filenames(ret);
	if (ret->lazy)
		log_set_last_flush(ret);
	else
		log_open(ret);
	ret->attrs=attr_list_dup(attrs);
	return ret;
}

void
log_set_header(struct log *this_, char *data, int len)
{
	this_->header.data=g_malloc(len);
	this_->header.max_len=this_->header.len=len;
	memcpy(this_->header.data, data, len);
}

void
log_set_trailer(struct log *this_, char *data, int len)
{
	this_->trailer.data=g_malloc(len);
	this_->trailer.max_len=this_->trailer.len=len;
	memcpy(this_->trailer.data, data, len);
}

void
log_write(struct log *this_, char *data, int len, enum log_flags flags)
{
	dbg(1,"enter\n");
	if (log_change_required(this_)) {
		dbg(1,"log_change");
		log_change(this_);
	}
	if (flags & log_flag_replace_buffer)
		this_->data.len=0;
	if (this_->data.len + len > this_->data.max_len) {
		dbg(2,"overflow\n");
		this_->data.max_len+=16384;
		this_->data.data=g_realloc(this_->data.data,this_->data.max_len);
	}
	memcpy(this_->data.data+this_->data.len, data, len);
	this_->data.len+=len;
	if (log_flush_required(this_) || (flags & log_flag_force_flush))
		log_flush(this_, flags);
}

void *
log_get_buffer(struct log *this_, int *len)
{
	if (len)
		*len=this_->data.len;
	return this_->data.data;
}


void
log_printf(struct log *this_, char *fmt, ...)
{
	char buffer[LOG_BUFFER_SIZE];
	int size;
	va_list ap;

	va_start(ap, fmt);

	// Format the string and write it to the log
	size = vsnprintf(buffer, LOG_BUFFER_SIZE, fmt, ap);
	log_write(this_, buffer, size, 0);

	va_end(ap);
}

void
log_destroy(struct log *this_)
{
	attr_list_free(this_->attrs);
	callback_destroy(this_->timer_callback);
	event_remove_timeout(this_->timer);
	log_flush(this_,0);
	log_close(this_);
	g_free(this_);
}
