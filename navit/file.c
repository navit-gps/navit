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
#include "cache.h"
#include "file.h"
#include "atom.h"
#include "config.h"

#ifdef HAVE_LIBCRYPTO
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif


#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef CACHE_SIZE
static GHashTable *file_name_hash;
#endif

static struct cache *file_cache;

struct file_cache_id {
	long long offset;
	int size;
	int file_name_id;
	int method;
} __attribute__ ((packed));

struct file *
file_create(char *name, enum file_flags flags)
{
	struct stat stat;
	struct file *file= g_new0(struct file,1);

	file->fd=open(name, O_RDONLY|O_LARGEFILE | O_BINARY);
	if (file->fd == -1) {
		g_free(file);
		return NULL;
	}
	dbg(1,"fd=%d\n", file->fd);
	fstat(file->fd, &stat);
	file->size=stat.st_size;
	dbg(1,"size=%Ld\n", file->size);
	file->name = g_strdup(name);
	file->name_id = (int)atom(name);
	if (file_cache && !(flags & file_flag_nocache)) 
		file->cache=1;
	dbg_assert(file != NULL);
	return file;
}

#if 0
struct file *
file_create_url(char *url)
{
	struct file *file= g_new0(struct file,1);
	char *cmd=g_strdup_printf("curl %s",url);
	file->name = g_strdup(url);
	file->stdfile=popen(cmd,"r");
	file->fd=fileno(file->stdfile);
	file->special=1;
	g_free(cmd);
	return file;
}
#endif

int file_is_dir(char *name)
{
	struct stat buf;
	if (! stat(name, &buf)) {
		return S_ISDIR(buf.st_mode);
	}
	return 0;

}

long long
file_size(struct file *file)
{
	return file->size;
}

int file_mkdir(char *name, int pflag)
{
	char buffer[strlen(name)+1];
	int ret;
	char *next;
	dbg(1,"enter %s %d\n",name,pflag);
	if (!pflag) {
		if (file_is_dir(name))
			return 0;
#ifdef HAVE_API_WIN32_BASE
		return mkdir(name);
#else
		return mkdir(name, 0777);
#endif
	}
	strcpy(buffer, name);
	next=buffer;
	while ((next=strchr(next, '/'))) {
		*next='\0';
		if (*buffer) {
			ret=file_mkdir(buffer, 0);
			if (ret)
				return ret;
		}
		*next++='/';
	}
	if (pflag == 2)
		return 0;
	return file_mkdir(buffer, 0);
}

int
file_mmap(struct file *file)
{
#if 0
	int mmap_size=file->size+1024*1024;
#else
	int mmap_size=file->size;
#endif
#ifdef HAVE_API_WIN32_BASE
	file->begin = (char*)mmap_readonly_win32( file->name, &file->map_handle, &file->map_file );
#else
	file->begin=mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, file->fd, 0);
	dbg_assert(file->begin != NULL);
	if (file->begin == (void *)0xffffffff) {
		perror("mmap");
		return 0;
	}
#endif
	dbg_assert(file->begin != (void *)0xffffffff);
	file->mmap_end=file->begin+mmap_size;
	file->end=file->begin+file->size;

	return 1;
}

unsigned char *
file_data_read(struct file *file, long long offset, int size)
{
	void *ret;
	if (file->special)
		return NULL;
	if (file->begin)
		return file->begin+offset;
	if (file_cache) {
		struct file_cache_id id={offset,size,file->name_id,0};
		ret=cache_lookup(file_cache,&id); 
		if (ret)
			return ret;
		ret=cache_insert_new(file_cache,&id,size);
	} else
		ret=g_malloc(size);
	lseek(file->fd, offset, SEEK_SET);
	if (read(file->fd, ret, size) != size) {
		file_data_free(file, ret);
		ret=NULL;
	}
	return ret;

}

unsigned char *
file_data_read_special(struct file *file, int size, int *size_ret)
{
	void *ret;
	if (!file->special)
		return NULL;
	ret=g_malloc(size);
	*size_ret=read(file->fd, ret, size);
	return ret;
}

unsigned char *
file_data_read_all(struct file *file)
{
	return file_data_read(file, 0, file->size);
}

int
file_data_write(struct file *file, long long offset, int size, unsigned char *data)
{
	if (file_cache) {
		struct file_cache_id id={offset,size,file->name_id,0};
		cache_flush(file_cache,&id);
	}
	lseek(file->fd, offset, SEEK_SET);
	if (write(file->fd, data, size) != size)
		return 0;
	if (file->size < offset+size)
		file->size=offset+size;
	return 1;
}

int
file_get_contents(char *name, unsigned char **buffer, int *size)
{
	struct file *file;
	file=file_create(name, 0);
	if (!file)
		return 0;
	*size=file_size(file);
	*buffer=file_data_read_all(file);
	file_destroy(file);
	return 1;	
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
	char *buffer = 0;
	uLongf destLen=size_uncomp;

	if (file_cache) {
		struct file_cache_id id={offset,size,file->name_id,1};
		ret=cache_lookup(file_cache,&id); 
		if (ret)
			return ret;
		ret=cache_insert_new(file_cache,&id,size_uncomp);
	} else 
		ret=g_malloc(size_uncomp);
	lseek(file->fd, offset, SEEK_SET);

	buffer = (char *)g_malloc(size);
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
	g_free(buffer);

	return ret;
}

unsigned char *
file_data_read_encrypted(struct file *file, long long offset, int size, int size_uncomp, int compressed, char *passwd)
{
#ifdef HAVE_LIBCRYPTO
	void *ret;
	unsigned char *buffer = 0;
	uLongf destLen=size_uncomp;

	if (file_cache) {
		struct file_cache_id id={offset,size,file->name_id,1};
		ret=cache_lookup(file_cache,&id); 
		if (ret)
			return ret;
		ret=cache_insert_new(file_cache,&id,size_uncomp);
	} else 
		ret=g_malloc(size_uncomp);
	lseek(file->fd, offset, SEEK_SET);

	buffer = (unsigned char *)g_malloc(size);
	if (read(file->fd, buffer, size) != size) {
		g_free(ret);
		ret=NULL;
	} else {
		unsigned char key[34], salt[8], verify[2], counter[16], xor[16], mac[10], mactmp[20], *datap;
		int overhead=sizeof(salt)+sizeof(verify)+sizeof(mac);
		int esize=size-overhead;
		PKCS5_PBKDF2_HMAC_SHA1(passwd, strlen(passwd), (unsigned char *)buffer, 8, 1000, 34, key);
		if (key[32] == buffer[8] && key[33] == buffer[9] && esize >= 0) {
			AES_KEY aeskey;
			AES_set_encrypt_key(key, 128, &aeskey);
			datap=buffer+sizeof(salt)+sizeof(verify);
			memset(counter, 0, sizeof(counter));
			while (esize > 0) {
				int i,curr_size,idx=0;
				do {
					counter[idx]++;
				} while (!counter[idx++]);
				AES_encrypt(counter, xor, &aeskey);
				curr_size=esize;
				if (curr_size > sizeof(xor))
					curr_size=sizeof(xor);
				for (i = 0 ; i < curr_size ; i++) 
					*datap++^=xor[i];
				esize-=curr_size;
			}
			size-=overhead;
			datap=buffer+sizeof(salt)+sizeof(verify);
			if (compressed) {
				if (uncompress_int(ret, &destLen, (Bytef *)datap, size) != Z_OK) {
					dbg(0,"uncompress failed\n");
					g_free(ret);
					ret=NULL;
				}
			} else {
				if (size == destLen) 
					memcpy(ret, destLen, buffer);
				else {
					dbg(0,"memcpy failed\n");
					g_free(ret);
					ret=NULL;
				}
			}
		} else {
			g_free(ret);
			ret=NULL;
		}
	}
	g_free(buffer);

	return ret;
#else
	return NULL;
#endif
}

void
file_data_free(struct file *file, unsigned char *data)
{
	if (file->begin) {
		if (data == file->begin)
			return;
		if (data >= file->begin && data < file->end)
			return;
	}	
	if (file->cache && data) {
		cache_entry_destroy(file_cache, data);
	} else
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
#if defined(_WIN32) || defined(__CEGCC__)
#else
	void *begin;
	munmap(f->begin, f->size);
	begin=mmap(f->begin, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
	if (f->begin != begin)
		printf("remap failed\n");
#endif
}

void
file_unmap(struct file *f)
{
#if defined(_WIN32) || defined(__CEGCC__)
    mmap_unmap_win32( f->begin, f->map_handle , f->map_file );
#else
	munmap(f->begin, f->size);
#endif
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
file_create_caseinsensitive(char *name, enum file_flags flags)
{
	char dirname[strlen(name)+1];
	char *filename;
	char *p;
	void *d;
	struct file *ret;

	ret=file_create(name, flags);
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
				ret=file_create(dirname, flags);
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
	switch (f->special) {
	case 0:
		close(f->fd);
		break;
#if 0
	case 1:
		pclose(f->stdfile);
		break;
#endif
	}

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

int
file_version(struct file *file, int mode)
{
#ifndef HAVE_API_WIN32_BASE
	struct stat st;
	int error;
	if (mode == 3) {
		long long size=lseek(file->fd, 0, SEEK_END);
		if (file->begin && file->begin+size > file->mmap_end) {
			file->version++;
		} else {
			file->size=size;
			if (file->begin)
				file->end=file->begin+file->size;
		}
	} else {
		if (mode == 2)
			error=stat(file->name, &st);
		else
			error=fstat(file->fd, &st);
		if (error || !file->version || file->mtime != st.st_mtime || file->ctime != st.st_ctime) {
			file->mtime=st.st_mtime;
			file->ctime=st.st_ctime;
			file->version++;
			dbg(1,"%s now version %d\n", file->name, file->version);
		}
	}
	return file->version;
#else
	return 0;
#endif
}

void *
file_get_os_handle(struct file *file)
{
	return GINT_TO_POINTER(file->fd);
}

void
file_init(void)
{
#ifdef CACHE_SIZE
	file_name_hash=g_hash_table_new(g_str_hash, g_str_equal);
	file_cache=cache_new(sizeof(struct file_cache_id), CACHE_SIZE);
#endif
}

