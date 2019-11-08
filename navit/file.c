/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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

#include "config.h"
#include "navit_lfs.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#include <dirent.h>
#endif /* _MSC_VER */
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <wordexp.h>
#include <glib.h>
#include <zlib.h>
#include "debug.h"
#include "cache.h"
#include "file.h"
#include "atom.h"
#include "item.h"
#include "util.h"
#include "types.h"
#include "zipfile.h"
#ifdef HAVE_SOCKET
#include <sys/socket.h>
#include <netdb.h>
#endif

#ifdef CACHE_SIZE
static GHashTable *file_name_hash;
#endif

static struct cache *file_cache;

#ifdef HAVE_PRAGMA_PACK
#pragma pack(push)
#pragma pack(1)
#endif

struct file_cache_id {
    long long offset;
    int size;
    int file_name_id;
    int method;
} ATTRIBUTE_PACKED;

#ifdef HAVE_PRAGMA_PACK
#pragma pack(pop)
#endif

#ifdef HAVE_SOCKET
static int file_socket_connect(char *host, char *service) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd=-1,s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    s = getaddrinfo(host, service, &hints, &result);
    if (s != 0) {
        dbg(lvl_error,"getaddrinfo error %s",gai_strerror(s));
        return -1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd != -1) {
            if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
                break;
            close(fd);
            fd=-1;
        }
    }
    freeaddrinfo(result);
    return fd;
}

static void file_http_request(struct file *file, char *method, char *host, char *path, char *header, int persistent) {
    char *request=g_strdup_printf("%s %s HTTP/1.0\r\nUser-Agent: navit %s\r\nHost: %s\r\n%s%s%s\r\n",method,path,
                                  NAVIT_VERSION,
                                  host,persistent?"Connection: Keep-Alive\r\n":"",header?header:"",header?"\r\n":"");
    int retval = write(file->fd, request, strlen(request));
    dbg(lvl_debug,"%s returned %d",request, retval);
    file->requests++;
}

static int file_request_do(struct file *file, struct attr **options, int connect) {
    struct attr *attr;
    char *name;

    if (!options)
        return 0;
    attr=attr_search(options, NULL, attr_url);
    if (!attr)
        return 0;
    name=attr->u.str;
    if (!name)
        return 0;
    g_free(file->name);
    file->name = g_strdup(name);
    if (!strncmp(name,"http://",7)) {
        char *host=g_strdup(name+7);
        char *port=strchr(host,':');
        char *path=strchr(name+7,'/');
        char *method="GET";
        char *header=NULL;
        int persistent=0;
        if ((attr=attr_search(options, NULL, attr_http_method)) && attr->u.str)
            method=attr->u.str;
        if ((attr=attr_search(options, NULL, attr_http_header)) && attr->u.str)
            header=attr->u.str;
        if ((attr=attr_search(options, NULL, attr_persistent)))
            persistent=attr->u.num;
        if (path)
            host[path-name-7]='\0';
        if (port)
            *port++='\0';
        dbg(lvl_debug,"host=%s path=%s",host,path);
        if (connect)
            file->fd=file_socket_connect(host,port?port:"80");
        file_http_request(file,method,host,path,header,persistent);
        file->special=1;
        g_free(host);
    }
    return 1;
}
#endif

static unsigned char *file_http_header_end(unsigned char *str, int len) {
    int i;
    for (i=0; i+1<len; i+=2) {
        if (str[i+1]=='\n') {
            if (str[i]=='\n')
                return str+i+2;
            else if (str[i]=='\r' && i+3<len && str[i+2]=='\r' && str[i+3]=='\n')
                return str+i+4;
            --i;
        } else if (str[i+1]=='\r') {
            if (i+4<len && str[i+2]=='\n' && str[i+3]=='\r' && str[i+4]=='\n')
                return str+i+5;
            --i;
        }
    }
    return NULL;
}

int file_request(struct file *f, struct attr **options) {
#ifdef HAVE_SOCKET
    return file_request_do(f, options, 0);
#else
    return 0;
#endif
}

char *file_http_header(struct file *f, char *header) {
    if (!f->headers)
        return NULL;
    return g_hash_table_lookup(f->headers, header);
}

struct file *
file_create(char *name, struct attr **options) {
    struct file *file= g_new0(struct file,1);
    struct attr *attr;
    int open_flags=O_LARGEFILE|O_BINARY;

    if (options && (attr=attr_search(options, NULL, attr_url))) {
#ifdef HAVE_SOCKET
        file_request_do(file, options, 1);
#endif
    } else {
        if (options && (attr=attr_search(options, NULL, attr_readwrite)) && attr->u.num) {
            open_flags |= O_RDWR;
            if ((attr=attr_search(options, NULL, attr_create)) && attr->u.num)
                open_flags |= O_CREAT|O_TRUNC;
        } else
            open_flags |= O_RDONLY;
        file->name = g_strdup(name);
        file->fd=open(name, open_flags, 0666);
        if (file->fd == -1) {
            g_free(file->name);
            g_free(file);
            return NULL;
        }
        dbg(lvl_debug,"fd=%d", file->fd);
        file->size=lseek(file->fd, 0, SEEK_END);
        if (file->size < 0)
            file->size=0;
        dbg(lvl_debug,"size="LONGLONG_FMT"", file->size);
        file->name_id = (long)atom(name);
    }
#ifdef CACHE_SIZE
    if (!options || !(attr=attr_search(options, NULL, attr_cache)) || attr->u.num)
        file->cache=1;
#endif
    dbg_assert(file != NULL);
    return file;
}

#if 0
struct file *
file_create_url(char *url) {
}
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

/**
 * @brief Check if a given path corresponds to a directory
 *
 * @param name The path to the filesystem element
 * @return !=0 (true) if @p name is a directory
 */
int file_is_dir(const char *name) {
    struct stat buf;
    if (! stat(name, &buf)) {
        return S_ISDIR(buf.st_mode);
    }
    return 0;

}

/**
 * @brief Check if a given path corresponds to a regular file
 *
 * @param name The path to the filesystem element
 * @return !=0 (true) if @p name is a regular file
 */
int file_is_reg(const char *name) {
    struct stat buf;
    if (! stat(name, &buf)) {
        return S_ISREG(buf.st_mode);
    }
    return 0;
}

long long file_size(struct file *file) {
    return file->size;
}

int file_mkdir(char *name, int pflag) {
    char *buffer=g_alloca(sizeof(char)*(strlen(name)+1));
    int ret;
    char *next;
    dbg(lvl_debug,"enter %s %d",name,pflag);
    if (!pflag) {
        if (file_is_dir(name))
            return 0;
#if defined HAVE_API_WIN32_BASE || defined _MSC_VER
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

int file_mmap(struct file *file) {
#if 0
    int mmap_size=file->size+1024*1024;
#else
    int mmap_size=file->size;
#endif
#ifdef HAVE_API_WIN32_BASE
    file->begin = (unsigned char*)mmap_readonly_win32( file->name, &file->map_handle, &file->map_file );
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

unsigned char *file_data_read(struct file *file, long long offset, int size) {
    void *ret;
    if (file->special)
        return NULL;
    if (file->begin)
        return file->begin+offset;
    if (file->cache) {
        struct file_cache_id id= {offset,size,file->name_id,0};
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

static void file_process_headers(struct file *file, unsigned char *headers) {
    char *tok;
    char *cl;
    if (file->headers)
        g_hash_table_destroy(file->headers);
    file->headers=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    while ((tok=strtok((char*)headers, "\r\n"))) {
        char *sep;
        tok=g_strdup(tok);
        sep=strchr(tok,':');
        if (!sep)
            sep=strchr(tok,'/');
        if (!sep) {
            g_free(tok);
            continue;
        }
        *sep++='\0';
        if (*sep == ' ')
            sep++;
        strtolower(tok, tok);
        dbg(lvl_debug,"header '%s'='%s'",tok,sep);
        g_hash_table_insert(file->headers, tok, sep);
        headers=NULL;
    }
    cl=g_hash_table_lookup(file->headers, "content-length");
    if (cl)
#ifdef HAVE__ATOI64
        file->size=_atoi64(cl);
#else
        file->size=atoll(cl);
#endif
}

static void file_shift_buffer(struct file *file, int amount) {
    memmove(file->buffer, file->buffer+amount, file->buffer_len-amount);
    file->buffer_len-=amount;
}

unsigned char *file_data_read_special(struct file *file, int size, int *size_ret) {
    unsigned char *ret,*hdr;
    int rets=0,rd;
    int buffer_size=8192;
    int eof=0;
    if (!file->special)
        return NULL;
    if (!file->buffer)
        file->buffer=g_malloc(buffer_size);
    ret=g_malloc(size);
    while ((size > 0 || file->requests) && (!eof || file->buffer_len)) {
        int toread=buffer_size-file->buffer_len;
        if (toread >= 4096 && !eof) {
            if (!file->requests && toread > size)
                toread=size;
            rd=read(file->fd, file->buffer+file->buffer_len, toread);
            if (rd > 0) {
                file->buffer_len+=rd;
            } else
                eof=1;
        }
        if (file->requests) {
            dbg(lvl_debug,"checking header");
            if ((hdr=file_http_header_end(file->buffer, file->buffer_len))) {
                hdr[-1]='\0';
                dbg(lvl_debug,"found %s",file->buffer);
                file_process_headers(file, file->buffer);
                file_shift_buffer(file, hdr-file->buffer);
                file->requests--;
                if (file_http_header(file, "location"))
                    break;
            }
        }
        if (!file->requests) {
            rd=file->buffer_len;
            if (rd > size)
                rd=size;
            memcpy(ret+rets, file->buffer, rd);
            file_shift_buffer(file, rd);
            rets+=rd;
            size-=rd;
        }
    }
    *size_ret=rets;
    return ret;
}

unsigned char *file_data_read_all(struct file *file) {
    return file_data_read(file, 0, file->size);
}

void file_data_flush(struct file *file, long long offset, int size) {
    if (file->cache) {
        struct file_cache_id id= {offset,size,file->name_id,0};
        cache_flush(file_cache,&id);
        dbg(lvl_debug,"Flushing "LONGLONG_FMT" %d bytes",offset,size);
    }
}

int file_data_write(struct file *file, long long offset, int size, const void *data) {
    file_data_flush(file, offset, size);
    lseek(file->fd, offset, SEEK_SET);
    if (write(file->fd, data, size) != size)
        return 0;
    if (file->size < offset+size)
        file->size=offset+size;
    return 1;
}

int file_get_contents(char *name, unsigned char **buffer, int *size) {
    struct file *file;
    file=file_create(name, 0);
    if (!file)
        return 0;
    file->cache=0;
    *size=file_size(file);
    *buffer=file_data_read_all(file);
    file_destroy(file);
    return 1;
}


static int uncompress_int(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen) {
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

unsigned char *file_data_read_compressed(struct file *file, long long offset, int size, int size_uncomp) {
    void *ret;
    char *buffer = 0;
    uLongf destLen=size_uncomp;

    if (file->cache) {
        struct file_cache_id id= {offset,size,file->name_id,1};
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
            dbg(lvl_error,"uncompress failed");
            g_free(ret);
            ret=NULL;
        }
    }
    g_free(buffer);

    return ret;
}

void file_data_free(struct file *file, unsigned char *data) {
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

void file_data_remove(struct file *file, unsigned char *data) {
    if (file->begin) {
        if (data == file->begin)
            return;
        if (data >= file->begin && data < file->end)
            return;
    }
    if (file->cache && data) {
        cache_flush_data(file_cache, data);
    } else
        g_free(data);
}

int file_exists(char const *name) {
    struct stat buf;
    if (! stat(name, &buf))
        return 1;
    return 0;
}

void file_remap_readonly(struct file *f) {
#if defined(_WIN32) || defined(__CEGCC__)
#else
    void *begin;
    munmap(f->begin, f->size);
    begin=mmap(f->begin, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
    if (f->begin != begin)
        printf("remap failed\n");
#endif
}

void file_unmap(struct file *f) {
#if defined(_WIN32) || defined(__CEGCC__)
    mmap_unmap_win32( f->begin, f->map_handle, f->map_file );
#else
    munmap(f->begin, f->size);
#endif
}

#ifndef _MSC_VER
void *file_opendir(char *dir) {
    return opendir(dir);
}
#else
void *file_opendir(char *dir) {
    WIN32_FIND_DATAA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
#undef UNICODE         // we need FindFirstFileA() which takes an 8-bit c-string
    char* fname=g_alloca(sizeof(char)*(strlen(dir)+4));
    sprintf(fname,"%s\\*",dir);
    hFind = FindFirstFileA(fname, &FindFileData);
    return hFind;
}
#endif

#ifndef _MSC_VER
char *file_readdir(void *hnd) {
    struct dirent *ent;

    ent=readdir(hnd);
    if (! ent)
        return NULL;
    return ent->d_name;
}
#else
char *file_readdir(void *hnd) {
    WIN32_FIND_DATA FindFileData;

    if (FindNextFile(hnd, &FindFileData) ) {
        return FindFileData.cFileName;
    } else {
        return NULL;
    }
}
#endif /* _MSC_VER */

#ifndef _MSC_VER
void file_closedir(void *hnd) {
    closedir(hnd);
}
#else
void file_closedir(void *hnd) {
    FindClose(hnd);
}
#endif /* _MSC_VER */

struct file *
file_create_caseinsensitive(char *name, struct attr **options) {
    char *dirname=g_alloca(sizeof(char)*(strlen(name)+1));
    char *filename;
    char *p;
    void *d;
    struct file *ret;

    ret=file_create(name, options);
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
            if (!g_ascii_strcasecmp(filename, p)) {
                strcpy(p, filename);
                ret=file_create(dirname, options);
                if (ret)
                    break;
            }
        }
        file_closedir(d);
    }
    return ret;
}

void file_fsync(struct file *f) {
#ifdef HAVE_FSYNC
    fsync(f->fd);
#endif
}

void file_destroy(struct file *f) {
    if (f->headers)
        g_hash_table_destroy(f->headers);
    switch (f->special) {
    case 0:
    case 1:
        close(f->fd);
        break;
    }

    if ( f->begin != NULL ) {
        file_unmap( f );
    }

    g_free(f->buffer);
    g_free(f->name);
    g_free(f);
}

struct file_wordexp {
    int err;
    char *pattern;
    wordexp_t we;
};

struct file_wordexp *
file_wordexp_new(const char *pattern) {
    struct file_wordexp *ret=g_new0(struct file_wordexp, 1);

    ret->pattern=g_strdup(pattern);
    ret->err=wordexp(pattern, &ret->we, 0);
    if (ret->err)
        dbg(lvl_debug,"wordexp('%s') returned %d", pattern, ret->err);
    return ret;
}

int file_wordexp_get_count(struct file_wordexp *wexp) {
    if (wexp->err)
        return 1;
    return wexp->we.we_wordc;
}

char ** file_wordexp_get_array(struct file_wordexp *wexp) {
    if (wexp->err)
        return &wexp->pattern;
    return wexp->we.we_wordv;
}

void file_wordexp_destroy(struct file_wordexp *wexp) {
    if (! wexp->err)
        wordfree(&wexp->we);
    g_free(wexp->pattern);
    g_free(wexp);
}


int file_version(struct file *file, int mode) {
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
            dbg(lvl_debug,"%s now version %d", file->name, file->version);
        }
    }
    return file->version;
#else
    return 0;
#endif
}

void *file_get_os_handle(struct file *file) {
    return GINT_TO_POINTER(file->fd);
}

int file_set_cache_size(int cache_size) {
#ifdef CACHE_SIZE
    cache_resize(file_cache, cache_size);
    return 1;
#else
    return 0;
#endif
}

void file_init(void) {
#ifdef CACHE_SIZE
    file_name_hash=g_hash_table_new(g_str_hash, g_str_equal);
    file_cache=cache_new(sizeof(struct file_cache_id), CACHE_SIZE);
#endif
    if(sizeof(off_t)<8)
        dbg(lvl_error,"Maps larger than 2GB are not supported by this binary, sizeof(off_t)=%zu",sizeof(off_t));
}

