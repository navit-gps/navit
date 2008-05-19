#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdio.h>
#include <wordexp.h>
#include <glib.h>
#include <zlib.h>
#include "debug.h"
#include "file.h"
#include "config.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static struct file *file_list;

struct file *
file_create(char *name)
{
	struct stat stat;
	struct file *file= g_new0(struct file,1);

	if (! file)
		return file;
	file->fd=open(name, O_RDONLY|O_LARGEFILE | O_BINARY);
	if (file->fd < 0) {
		g_free(file);
		return NULL;
	}
	fstat(file->fd, &stat);
	file->size=stat.st_size;
	file->name = g_strdup(name);
	g_assert(file != NULL);
	file->next=file_list;
	file_list=file;
	return file;
}

int
file_mmap(struct file *file)
{
#ifdef _WIN32
    file->begin = (char*)mmap_readonly_win32( file->name, &file->map_handle, &file->map_file );
#else
	file->begin=mmap(NULL, file->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, file->fd, 0);
	g_assert(file->begin != NULL);
	if (file->begin == (void *)0xffffffff) {
		perror("mmap");
		return 0;
	}
#endif
	g_assert(file->begin != (void *)0xffffffff);
	file->end=file->begin+file->size;

	return 1;
}

unsigned char *
file_data_read(struct file *file, long long offset, int size)
{
	void *ret;
	if (file->begin)
		return file->begin+offset;
	ret=g_malloc(size);
	lseek(file->fd, offset, SEEK_SET);
	if (read(file->fd, ret, size) != size) {
		g_free(ret);
		ret=NULL;
	}
	return ret;

}

static int
uncompress_int(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	err = inflateInit2(&stream, -MAX_WBITS);
	if (err != Z_OK) return err;

	err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
	inflateEnd(&stream);
	if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
		return Z_DATA_ERROR;
		return err;
	}
	*destLen = stream.total_out;

	err = inflateEnd(&stream);
	return err;
}

unsigned char *
file_data_read_compressed(struct file *file, long long offset, int size, int size_uncomp)
{
	void *ret;
	char buffer[size];
	uLongf destLen=size_uncomp;

	ret=g_malloc(size_uncomp);
	lseek(file->fd, offset, SEEK_SET);
	if (read(file->fd, buffer, size) != size) {
		g_free(ret);
		ret=NULL;
	} else {
		if (uncompress_int(ret, &destLen, (Bytef *)buffer, size) != Z_OK) {
			dbg(0,"uncompress failed\n");
			g_free(ret);
			ret=NULL;
		}
	}
	return ret;
}

void
file_data_free(struct file *file, unsigned char *data)
{
	if (file->begin && data >= file->begin && data < file->end)
		return;
	g_free(data);
}

int
file_exists(char *name)
{
	struct stat buf;
	if (! stat(name, &buf))
		return 1;
	return 0;
}

void
file_remap_readonly(struct file *f)
{
#ifdef _WIN32
#else
	void *begin;
	munmap(f->begin, f->size);
	begin=mmap(f->begin, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
	if (f->begin != begin)
		printf("remap failed\n");
#endif
}

void
file_remap_readonly_all(void)
{
	struct file *f=file_list;
	int limit=1000;

	while (f && limit-- > 0) {
		file_remap_readonly(f);
		f=f->next;
	}
}

void
file_unmap(struct file *f)
{
#ifdef _WIN32
    mmap_unmap_win32( f->begin, f->map_handle , f->map_file );
#else
	munmap(f->begin, f->size);
#endif
}

void
file_unmap_all(void)
{
	struct file *f=file_list;
	int limit=1000;

	while (f && limit-- > 0) {
		file_unmap(f);
		f=f->next;
	}
}



void *
file_opendir(char *dir)
{
	return opendir(dir);
}

char *
file_readdir(void *hnd)
{
	struct dirent *ent;

	ent=readdir(hnd);
	if (! ent)
		return NULL;
	return ent->d_name;
}

void
file_closedir(void *hnd)
{
	closedir(hnd);
}

struct file *
file_create_caseinsensitive(char *name)
{
	char dirname[strlen(name)+1];
	char *filename;
	char *p;
	void *d;
	struct file *ret;

	ret=file_create(name);
	if (ret)
		return ret;

	strcpy(dirname, name);
	p=dirname+strlen(name);
	while (p > dirname) {
		if (*p == '/')
			break;
		p--;
	}
	*p=0;
	d=file_opendir(dirname);
	if (d) {
		*p++='/';
		while ((filename=file_readdir(d))) {
			if (!strcasecmp(filename, p)) {
				strcpy(p, filename);
				ret=file_create(dirname);
				if (ret)
					break;
			}
		}
		file_closedir(d);
	}
	return ret;
}

void
file_destroy(struct file *f)
{
	close(f->fd);

    if ( f->begin != NULL )
    {
        file_unmap( f );
    }

	g_free(f->name);
	g_free(f);
}

struct file_wordexp {
	int err;
	wordexp_t we;
};

struct file_wordexp *
file_wordexp_new(const char *pattern)
{
	struct file_wordexp *ret=g_new0(struct file_wordexp, 1);

	ret->err=wordexp(pattern, &ret->we, 0);
	if (ret->err)
		dbg(0,"wordexp('%s') returned %d\n", pattern, ret->err);
	return ret;
}

int
file_wordexp_get_count(struct file_wordexp *wexp)
{
	return wexp->we.we_wordc;
}

char **
file_wordexp_get_array(struct file_wordexp *wexp)
{
	return wexp->we.we_wordv;
}

void
file_wordexp_destroy(struct file_wordexp *wexp)
{
	if (! wexp->err)
		wordfree(&wexp->we);
	g_free(wexp);
}


int
file_get_param(struct file *file, struct param_list *param, int count)
{
	int i=count;
	param_add_string("Filename", file->name, &param, &count);
	param_add_hex("Size", file->size, &param, &count);
	return i-count;
}
