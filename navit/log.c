#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <glib.h>
#include "file.h"
#include "item.h"
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
	int flush_size;
	int flush_time;
	guint timer;
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
	char buffer[4096];
	int i;

	strftime_localtime(buffer, 4096, this_->filename);
	this_->filename_ex1=g_strdup(buffer);
	if (strstr(this_->filename_ex1,"%i")) {
		i=0;
		do {
			g_free(this_->filename_ex2);
			this_->filename_ex2=g_strdup_printf(this_->filename_ex1,i++);
		} while (file_exists(this_->filename_ex2));
	} else 
		this_->filename_ex2=g_strdup(this_->filename_ex1);
}

static void
log_open(struct log *this_)
{
	char *mode;
	if (this_->overwrite)
		mode="w";
	else
		mode="r+";
	this_->f=fopen(this_->filename_ex2, mode);
	if (! this_->f)
		this_->f=fopen(this_->filename_ex2, "w");
	if (!this_->overwrite) 
		fseek(this_->f, 0, SEEK_END);
	this_->empty = !ftell(this_->f);
}

static void
log_close(struct log *this_)
{
	if (this_->trailer.len) 
		fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
	fflush(this_->f);
	fclose(this_->f);
	this_->f=NULL;
}

static void
log_flush(struct log *this_)
{
	long pos;
	if (this_->empty) {
		if (this_->header.len) 
			fwrite(this_->header.data, 1, this_->header.len, this_->f);
		if (this_->header.len || this_->data.len)
			this_->empty=0;
	}
	fwrite(this_->data.data, 1, this_->data.len, this_->f);
	if (this_->trailer.len) {
		pos=ftell(this_->f);
		if (pos > 0) {
			fwrite(this_->trailer.data, 1, this_->trailer.len, this_->f);
			fseek(this_->f, pos, SEEK_SET);	
		}
	}
	
	fflush(this_->f);
	g_free(this_->data.data);
	this_->data.data=NULL;
	this_->data.max_len=this_->data.len=0;
	gettimeofday(&this_->last_flush, NULL);
}

static int
log_flush_required(struct log *this_)
{
	return this_->data.len > this_->flush_size;
}


static void
log_change(struct log *this_)
{
	log_flush(this_);
	log_close(this_);
	log_open(this_);
}

static int
log_change_required(struct log *this_)
{
	char buffer[4096];

	strftime_localtime(buffer, 4096, this_->filename);
	return (strcmp(this_->filename_ex1, buffer) != 0);
}

static gboolean
log_timer(gpointer data)
{
	struct log *this_=data;
	struct timeval tv;
	int delta;
	gettimeofday(&tv, NULL);
	delta=(tv.tv_sec-this_->last_flush.tv_sec)*1000+(tv.tv_usec-this_->last_flush.tv_usec)/1000;
	dbg(0,"delta=%d flush_time=%d\n", delta, this_->flush_time);
	if (this_->flush_time && delta > this_->flush_time*1000)
		log_flush(this_);
	return TRUE;
}

int
log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
        return attr_generic_get_attr(this_->attrs, type, attr, iter);
}


struct log *
log_new(struct attr **attrs)
{
	struct log *ret=g_new0(struct log, 1);
	struct attr *data,*overwrite,*flush_size,*flush_time;

	dbg(1,"enter\n");
	data=attr_search(attrs, NULL, attr_data);
	if (! data)
		return NULL;
	ret->filename=g_strdup(data->u.str);
	overwrite=attr_search(attrs, NULL, attr_overwrite);
	if (overwrite)
		ret->overwrite=overwrite->u.num;
	flush_size=attr_search(attrs, NULL, attr_flush_size);
	if (flush_size)
		ret->flush_size=flush_size->u.num;
	flush_time=attr_search(attrs, NULL, attr_flush_time);
	if (flush_time)
		ret->flush_time=flush_time->u.num;
	if (ret->flush_time) {
		dbg(0,"interval %d\n", ret->flush_time*1000);
		ret->timer=g_timeout_add(ret->flush_time*1000, log_timer, ret);
	}
	expand_filenames(ret);
	log_open(ret);
	gettimeofday(&ret->last_flush, NULL);
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
log_write(struct log *this_, char *data, int len)
{
	dbg(1,"enter\n");
	if (log_change_required(this_)) 
		log_change(this_);
	if (this_->data.len + len > this_->data.max_len) {
		dbg(2,"overflow\n");
		this_->data.max_len+=16384;
		this_->data.data=g_realloc(this_->data.data,this_->data.max_len);
	}
	memcpy(this_->data.data+this_->data.len, data, len);
	this_->data.len+=len;
	if (log_flush_required(this_))
		log_flush(this_);
}

void
log_destroy(struct log *this_)
{
	if (this_->timer)
		g_source_remove(this_->timer);
	log_flush(this_);
	log_close(this_);
	g_free(this_);
}
