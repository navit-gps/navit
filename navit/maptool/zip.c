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
#ifdef HAVE_LZMA
#    include <lzma.h>
#endif

struct zip_info {
    int zipnum;
    int dir_size;
    long long offset;
    int compression_level;
    int compression_method;
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

#ifdef HAVE_LZMA
void *arena_alloc(void *opaque, size_t nmemb, size_t size) {
    struct lzma_arena *a = (struct lzma_arena *)opaque;
    if (nmemb && size > SIZE_MAX / nmemb)
        return NULL;
    size_t total = nmemb * size;
    size_t aligned = (total + 15) & ~(size_t)15;
    if (a->pos + aligned > a->size)
        return NULL;
    char *ptr = a->buf + a->pos;
    a->pos += aligned;
    return ptr;
}

void arena_free(void *opaque, void *ptr) {
    (void)opaque;
    (void)ptr;
}

static int compress_lzma_int(uint8_t *dest, size_t *destLen, const uint8_t *source, size_t sourceLen, int level,
                             const lzma_allocator *allocator) {
    size_t out_pos = 0;
    lzma_ret ret =
        lzma_easy_buffer_encode(level, LZMA_CHECK_CRC64, allocator, source, sourceLen, dest, &out_pos, *destLen);
    if (ret == LZMA_OK) {
        *destLen = out_pos;
        return 0;
    }
    return -1;
}
#endif

#ifdef HAVE_ZLIB
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

char *compress_for_zip(char *input, int input_size, int level, int method, int *out_size, int *out_method,
                       char **reuse_buf, size_t *reuse_size, void *lzma_alloc) {
    size_t compbuflen = input_size + input_size / 3 + 200;

    if (*reuse_size < compbuflen) {
        *reuse_buf = g_realloc(*reuse_buf, compbuflen);
        *reuse_size = compbuflen;
    }

    *out_method = ZIP_COMPRESSION_STORED;
    if (level) {
        int tried_lzma = 0;
#if defined(HAVE_LZMA)
        if (method == ZIP_COMPRESSION_LZMA) {
            tried_lzma = 1;
            size_t out_len = compbuflen;
            if (compress_lzma_int((uint8_t *)*reuse_buf, &out_len, (uint8_t *)input, input_size, level,
                                  (const lzma_allocator *)lzma_alloc)
                == 0) {
                if (out_len < (size_t)input_size) {
                    char *result = g_malloc(out_len);
                    memcpy(result, *reuse_buf, out_len);
                    *out_size = out_len;
                    *out_method = ZIP_COMPRESSION_LZMA;
                    return result;
                }
            }
        }
#endif
#if defined(HAVE_ZLIB)
        if (!*out_method) {
            uLongf destlen = compbuflen;
            if (compress2_int((Byte *)*reuse_buf, &destlen, (Bytef *)input, input_size, level) == Z_OK) {
                if (destlen < (uLongf)input_size) {
                    char *result = g_malloc(destlen);
                    memcpy(result, *reuse_buf, destlen);
                    *out_size = destlen;
                    *out_method = ZIP_COMPRESSION_DEFLATE;
                    return result;
                }
            }
        }
#endif
#if defined(HAVE_LZMA)
        if (!*out_method && !tried_lzma) {
            size_t out_len = compbuflen;
            if (compress_lzma_int((uint8_t *)*reuse_buf, &out_len, (uint8_t *)input, input_size, level,
                                  (const lzma_allocator *)lzma_alloc)
                == 0) {
                if (out_len < (size_t)input_size) {
                    char *result = g_malloc(out_len);
                    memcpy(result, *reuse_buf, out_len);
                    *out_size = out_len;
                    *out_method = ZIP_COMPRESSION_LZMA;
                    return result;
                }
            }
        }
#endif
    }
    *out_size = input_size;
    return NULL;
}

void write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size) {
    int comp_size, zipmthd;
    unsigned long crc;
    size_t reuse_size = 0;
    char *reuse_buf = NULL;
    char *comp_data;

    crc = crc32(0, NULL, 0);
    crc = crc32(crc, (unsigned char *)data, data_size);

    comp_data = compress_for_zip(data, data_size, zip_info->compression_level, zip_info->compression_method, &comp_size,
                                 &zipmthd, &reuse_buf, &reuse_size, NULL);
    if (!comp_data) {
        comp_data = data;
        comp_size = data_size;
        zipmthd = ZIP_COMPRESSION_STORED;
    }

    write_zipmember_raw(zip_info, name, filelen, comp_data, comp_size, data_size, crc, zipmthd);

    if (comp_data != data)
        g_free(comp_data);
    g_free(reuse_buf);
}

void write_zipmember_raw(struct zip_info *zip_info, char *name, int filelen, char *compressed_data, int compressed_size,
                         int uncompressed_size, unsigned long crc, int zipmthd) {
    struct zip_lfh lfh = {
        zip_lfh_sig,
        0x0a,
        0x0,
        zipmthd,
        zip_info->time,
        zip_info->date,
        (int)crc,
        compressed_size,
        (unsigned int)uncompressed_size,
        filelen,
        0x0,
    };
    struct zip_cd cd = {
        zip_cd_sig,
        0x17,
        0x00,
        0x0a,
        0x00,
        0x0000,
        zipmthd,
        zip_info->time,
        zip_info->date,
        (int)crc,
        compressed_size,
        (unsigned int)uncompressed_size,
        filelen,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0,
        zip_info->offset,
    };
    struct zip_cd_ext cd_ext = {
        0x1,
        0x8,
        zip_info->offset,
    };
    char *filename;
    int len;

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
    zip_write(zip_info, compressed_data, compressed_size);
    zip_info->offset += compressed_size;
    dbg_assert(fwrite(&cd, sizeof(cd), 1, zip_info->dir) == 1);
    dbg_assert(fwrite(filename, filelen, 1, zip_info->dir) == 1);
    zip_info->dir_size += sizeof(cd) + filelen;
    if (zip_info->zip64) {
        dbg_assert(fwrite(&cd_ext, sizeof(cd_ext), 1, zip_info->dir) == 1);
        zip_info->dir_size += sizeof(cd_ext);
    }
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
    struct zip_info *info = g_new0(struct zip_info, 1);
    info->compression_method = ZIP_COMPRESSION_DEFLATE;
    return info;
}

void zip_set_zip64(struct zip_info *info, int on) {
    info->zip64 = on;
}

void zip_set_compression_level(struct zip_info *info, int level) {
    info->compression_level = level;
}

void zip_set_compression_method(struct zip_info *info, int method) {
    info->compression_method = method;
}

int zip_get_compression_method(struct zip_info *info) {
    return info->compression_method;
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

int zip_get_compression_level(struct zip_info *info) {
    return info->compression_level;
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
