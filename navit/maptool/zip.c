/*
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

#include "config.h"
#include "debug.h"
#include "maptool.h"
#include "zipfile.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zconf.h>
#include <zlib.h>
#ifdef HAVE_LIBDEFLATE
#    include <libdeflate.h>
#endif

struct zip_info {
    int zipnum;
    int dir_size;
    long long offset;
    int compression_level;
    int maxnamelen;
    int zip64;
    short date;
    short time;
    FILE *res2;
    FILE *index;
    FILE *dir;
};

static int zip_write(struct zip_info *info, void *data, int len) {
    if (fwrite(data, len, 1, info->res2) != 1)
        return 0;
    return 1;
}

#if defined(HAVE_ZLIB) && !defined(HAVE_LIBDEFLATE)
static int compress2_int(Byte *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level) {
    z_stream stream;
    int err;

    stream.next_in = (Bytef *)source;
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen)
        return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    err = deflateInit2(&stream, level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
        return err;

    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&stream);
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;

    err = deflateEnd(&stream);
    return err;
}
#endif

/**
 * @brief Compresses the data of one zip member.
 *
 * This function reads zip_info but does not change it. Several threads can
 * therefore compress different members at the same time. The caller must then
 * give the results to write_zipmember_compressed() in the order of the members,
 * because the position of a member in the file depends on the members before it.
 *
 * @param zip_info the zip file that receives the member
 * @param data the data of the member
 * @param data_size the size of data in bytes
 * @param m receives the result. Give it to write_zipmember_compressed().
 */
void zip_compress_member(struct zip_info *zip_info, char *data, int data_size, struct zip_member *m) {
    m->data = data;
    m->data_size = data_size;
    m->uncomp_size = data_size;
    m->buffer = NULL;
    m->crc = crc32(crc32(0, NULL, 0), (unsigned char *)data, data_size);
    m->method = zip_info->compression_level ? 8 : 0;
#ifdef HAVE_LIBDEFLATE
    if (zip_info->compression_level) {
        /* libdeflate produces the same raw deflate format as the zlib call
         * below, only faster. Level 6 of libdeflate compresses tile data
         * better than level 9 of zlib. A compressor is not safe for use by
         * several threads, so every call allocates its own. */
        struct libdeflate_compressor *comp = libdeflate_alloc_compressor(zip_info->compression_level);
        size_t destlen = data_size + data_size / 500 + 12;
        size_t outlen;
        m->buffer = g_malloc(destlen);
        outlen = libdeflate_deflate_compress(comp, data, data_size, m->buffer, destlen);
        libdeflate_free_compressor(comp);
        if (outlen && outlen < (size_t)data_size) {
            m->data = m->buffer;
            m->data_size = outlen;
        } else
            m->method = 0;
    }
#elif defined(HAVE_ZLIB)
    if (zip_info->compression_level) {
        uLongf destlen = data_size + data_size / 500 + 12;
        int error;
        m->buffer = g_malloc(destlen);
        error = compress2_int((Byte *)m->buffer, &destlen, (Bytef *)data, data_size, zip_info->compression_level);
        if (error == Z_OK) {
            if (destlen < data_size) {
                m->data = m->buffer;
                m->data_size = destlen;
            } else
                m->method = 0;
        } else {
            /* Note that this keeps method 8 for data that is not compressed.
             * The behavior is the same as before the members were compressed in
             * parallel. deflate needs more room than the buffer only for data
             * that grows, and tile data always becomes smaller. */
            fprintf(stderr, "compress2 returned %d\n", error);
        }
    }
#endif
}

/**
 * @brief Writes one compressed zip member and frees its buffer.
 *
 * Call this function for the members of a slice in the order of the members.
 *
 * @param zip_info the zip file that receives the member
 * @param name the name of the member
 * @param filelen the length that every name in this file uses
 * @param m the result of zip_compress_member(). This function frees its buffer.
 */
void write_zipmember_compressed(struct zip_info *zip_info, char *name, int filelen, struct zip_member *m) {
    struct zip_lfh lfh = {
        0x04034b50, 0x0a, 0x0, 0x0, zip_info->time, zip_info->date, 0x0, 0x0, 0x0, filelen, 0x0,
    };
    struct zip_cd cd = {
        0x02014b50, 0x17,    0x00,   0x0a,   0x00,   0x0000, 0x0, zip_info->time,   zip_info->date, 0x0, 0x0,
        0x0,        filelen, 0x0000, 0x0000, 0x0000, 0x0000, 0x0, zip_info->offset,
    };
    struct zip_cd_ext cd_ext = {
        0x1,
        0x8,
        zip_info->offset,
    };
    char *filename;
    int len;

    lfh.zipmthd = m->method;
    lfh.zipcrc = m->crc;
    lfh.zipsize = m->data_size;
    lfh.zipuncmp = m->uncomp_size;
    cd.zipccrc = m->crc;
    cd.zipcsiz = m->data_size;
    cd.zipcunc = m->uncomp_size;
    cd.zipcmthd = m->method;
    if (zip_info->zip64) {
        cd.zipofst = 0xffffffff;
        cd.zipcxtl += sizeof(cd_ext);
    }
    filename = g_alloca(filelen + 1);
    strcpy(filename, name);
    len = strlen(filename);
    while (len < filelen) {
        filename[len++] = '_';
    }
    filename[filelen] = '\0';
    zip_write(zip_info, &lfh, sizeof(lfh));
    zip_write(zip_info, filename, filelen);
    zip_info->offset += sizeof(lfh) + filelen;
    zip_write(zip_info, m->data, m->data_size);
    zip_info->offset += m->data_size;
    dbg_assert(fwrite(&cd, sizeof(cd), 1, zip_info->dir) == 1);
    dbg_assert(fwrite(filename, filelen, 1, zip_info->dir) == 1);
    zip_info->dir_size += sizeof(cd) + filelen;
    if (zip_info->zip64) {
        dbg_assert(fwrite(&cd_ext, sizeof(cd_ext), 1, zip_info->dir) == 1);
        zip_info->dir_size += sizeof(cd_ext);
    }

    g_free(m->buffer);
    m->buffer = NULL;
}

void write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size) {
    struct zip_member m;
    zip_compress_member(zip_info, data, data_size, &m);
    write_zipmember_compressed(zip_info, name, filelen, &m);
}

int zip_write_index(struct zip_info *info) {
    int size = ftell(info->index);
    char *buffer;

    buffer = g_alloca(size);
    fseek(info->index, 0, SEEK_SET);

    if (fread(buffer, size, 1, info->index) == 0) {
        dbg(lvl_warning, "fread failed");
        return 1;
    } else {
        write_zipmember(info, "index", strlen("index"), buffer, size);
    }
    info->zipnum++;
    return 0;
}

static void zip_write_file_data(struct zip_info *info, FILE *in) {
    size_t size;
    char buffer[4096];
    while ((size = fread(buffer, 1, 4096, in)))
        zip_write(info, buffer, size);
}

int zip_write_directory(struct zip_info *info) {
    struct zip_eoc eoc = {
        0x06054b50, 0x0000, 0x0000, 0x0000, 0x0000, 0x0, 0x0, 0x0,
    };
    struct zip64_eoc eoc64 = {
        0x06064b50, 0x0, 0x0, 0x0403, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    };
    struct zip64_eocl eocl = {
        0x07064b50,
        0x0,
        0x0,
        0x0,
    };

    fseek(info->dir, 0, SEEK_SET);
    zip_write_file_data(info, info->dir);
    if (info->zip64) {
        eoc64.zip64esize = sizeof(eoc64) - 12;
        eoc64.zip64ever = 0x031e;  /* UNIX, spec 3.0 */
        eoc64.zip64eneed = 0x002d; /* version 4.5 for zip64*/
        eoc64.zip64enum = info->zipnum;
        eoc64.zip64ecenn = info->zipnum;
        eoc64.zip64ecsz = info->dir_size;
        eoc64.zip64eofst = info->offset;
        zip_write(info, &eoc64, sizeof(eoc64));
        eocl.zip64lofst = info->offset + info->dir_size;
        eocl.zip74lnum = 1; /* we only have single disk archives. */
        zip_write(info, &eocl, sizeof(eocl));

        /* force to use the 64 bit values */
        eoc.zipenum = 0xFFFF;
        eoc.zipecenn = 0xFFFF;
        eoc.zipecsz = 0xFFFFFFFF;
        eoc.zipeofst = 0xFFFFFFFF;
    } else {
        eoc.zipenum = info->zipnum;
        eoc.zipecenn = info->zipnum;
        eoc.zipecsz = info->dir_size;
        eoc.zipeofst = info->offset;
    }
    zip_write(info, &eoc, sizeof(eoc));
    sig_alrm(0);
#ifndef _WIN32
    alarm(0);
#endif
    return 0;
}

struct zip_info *zip_new(void) {
    return g_new0(struct zip_info, 1);
}

void zip_set_zip64(struct zip_info *info, int on) {
    info->zip64 = on;
}

void zip_set_compression_level(struct zip_info *info, int level) {
    info->compression_level = level;
}

void zip_set_maxnamelen(struct zip_info *info, int max) {
    info->maxnamelen = max;
}

int zip_get_maxnamelen(struct zip_info *info) {
    return info->maxnamelen;
}

int zip_add_member(struct zip_info *info) {
    return info->zipnum++;
}

int zip_set_timestamp(struct zip_info *info, char *timestamp) {
    int year, month, day, hour, min, sec;

    if (sscanf(timestamp, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
        info->date = day | (month << 5) | ((year - 1980) << 9);
        info->time = (sec >> 1) | (min << 5) | (hour << 11);
        return 1;
    }
    return 0;
}

int zip_open(struct zip_info *info, char *out, char *dir, char *index) {
    info->res2 = fopen(out, "wb+");
    if (!info->res2) {
        fprintf(stderr, "Could not open output zip file %s\n", out);
        return 0;
    }
    info->dir = fopen(dir, "wb+");
    if (!info->dir) {
        fprintf(stderr, "Could not open zip directory %s\n", dir);
        return 0;
    }
    info->index = fopen(index, "wb+");
    if (!info->index) {
        fprintf(stderr, "Could not open index %s\n", index);
        return 0;
    }
    return 1;
}

FILE *zip_get_index(struct zip_info *info) {
    return info->index;
}

int zip_get_zipnum(struct zip_info *info) {
    return info->zipnum;
}

void zip_set_zipnum(struct zip_info *info, int num) {
    info->zipnum = num;
}

void zip_close(struct zip_info *info) {
    fclose(info->index);
    fclose(info->dir);
    fclose(info->res2);
}

void zip_destroy(struct zip_info *info) {
    g_free(info);
}
