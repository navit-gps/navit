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

#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include "maptool.h"
#include "zipfile.h"

#ifdef HAVE_LIBCRYPTO
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
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
	char *passwd;
	FILE *res2;
	FILE *index;
	FILE *dir;
#ifdef HAVE_LIBCRYPTO
	MD5_CTX md5_ctx;
#endif
	int md5;
};

static int
zip_write(struct zip_info *info, void *data, int len)
{
	if (fwrite(data, len, 1, info->res2) != 1)
		return 0;
#ifdef HAVE_LIBCRYPTO
	if (info->md5) 
		MD5_Update(&info->md5_ctx, data, len);
#endif
	return 1;
}

#ifdef HAVE_ZLIB
static int
compress2_int(Byte *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	err = deflateInit2(&stream, level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) return err;

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

void
write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size)
{
	struct zip_lfh lfh = {
		0x04034b50,
		0x0a,
		0x0,
		0x0,
		zip_info->time,
		zip_info->date,
		0x0,
		0x0,
		0x0,
		filelen,
		0x0,
	};
	struct zip_cd cd = {
		0x02014b50,
		0x17,
		0x00,
		0x0a,
		0x00,
		0x0000,
		0x0,
		zip_info->time,
		zip_info->date,
		0x0,
		0x0,
		0x0,
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
#ifdef HAVE_LIBCRYPTO
	struct zip_enc enc = {
		0x9901,
		0x7,
		0x2,
		'A','E',
		0x1,
		0x0,
	};
	unsigned char salt[8], key[34], verify[2], mac[10];
#endif
	char filename[filelen+1];
	int error,crc=0,len,comp_size=data_size;
	uLongf destlen=data_size+data_size/500+12;
	char *compbuffer;

	compbuffer = malloc(destlen);
	if (!compbuffer) {
	  fprintf(stderr, "No more memory.\n");
	  exit (1);
	}
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {	
		RAND_bytes(salt, sizeof(salt));
		PKCS5_PBKDF2_HMAC_SHA1(zip_info->passwd, strlen(zip_info->passwd), salt, sizeof(salt), 1000, sizeof(key), key);
		verify[0]=key[32];
		verify[1]=key[33];
	} else {
#endif
		crc=crc32(0, NULL, 0);
		crc=crc32(crc, (unsigned char *)data, data_size);
#ifdef HAVE_LIBCRYPTO
	}
#endif
	lfh.zipmthd=zip_info->compression_level ? 8:0;
#ifdef HAVE_ZLIB
	if (zip_info->compression_level) {
		error=compress2_int((Byte *)compbuffer, &destlen, (Bytef *)data, data_size, zip_info->compression_level);
		if (error == Z_OK) {
			if (destlen < data_size) {
				data=compbuffer;
				comp_size=destlen;
			} else
				lfh.zipmthd=0;
		} else {
			fprintf(stderr,"compress2 returned %d\n", error);
		}
	}
#endif
	lfh.zipcrc=crc;
	lfh.zipsize=comp_size;
	lfh.zipuncmp=data_size;
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {
		enc.compress_method=lfh.zipmthd;
		lfh.zipmthd=99;
		lfh.zipxtraln+=sizeof(enc);
		lfh.zipgenfld|=1;
		lfh.zipsize+=sizeof(salt)+sizeof(verify)+sizeof(mac);
	}
#endif
	cd.zipccrc=crc;
	cd.zipcsiz=lfh.zipsize;
	cd.zipcunc=data_size;
	cd.zipcmthd=lfh.zipmthd;
	if (zip_info->zip64) {
		cd.zipofst=0xffffffff;
		cd.zipcxtl+=sizeof(cd_ext);
	}
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {
		cd.zipcmthd=99;
		cd.zipcxtl+=sizeof(enc);
		cd.zipcflg|=1;
	}
#endif
	strcpy(filename, name);
	len=strlen(filename);
	while (len < filelen) {
		filename[len++]='_';
	}
	filename[filelen]='\0';
	zip_write(zip_info, &lfh, sizeof(lfh));
	zip_write(zip_info, filename, filelen);
	zip_info->offset+=sizeof(lfh)+filelen;
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {
		unsigned char counter[16], xor[16], *datap=(unsigned char *)data;
		int size=comp_size;
		AES_KEY aeskey;
		zip_write(zip_info, &enc, sizeof(enc));
		zip_write(zip_info, salt, sizeof(salt));
		zip_write(zip_info, verify, sizeof(verify));
		zip_info->offset+=sizeof(enc)+sizeof(salt)+sizeof(verify);
		AES_set_encrypt_key(key, 128, &aeskey);
		memset(counter, 0, sizeof(counter));
		while (size > 0) {
			int i,curr_size,idx=0;
			do {
				counter[idx]++;
			} while (!counter[idx++]);
			AES_encrypt(counter, xor, &aeskey);
			curr_size=size;
			if (curr_size > sizeof(xor))
				curr_size=sizeof(xor);
			for (i = 0 ; i < curr_size ; i++) 
				*datap++^=xor[i];
			size-=curr_size;
		}
	}
#endif
	zip_write(zip_info, data, comp_size);
	zip_info->offset+=comp_size;
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {
		unsigned int maclen=sizeof(mac);
		unsigned char mactmp[maclen*2];
		HMAC(EVP_sha1(), key+16, 16, (unsigned char *)data, comp_size, mactmp, &maclen);
		zip_write(zip_info, mactmp, sizeof(mac));
		zip_info->offset+=sizeof(mac);
	}
#endif
	fwrite(&cd, sizeof(cd), 1, zip_info->dir);
	fwrite(filename, filelen, 1, zip_info->dir);
	zip_info->dir_size+=sizeof(cd)+filelen;
	if (zip_info->zip64) {
		fwrite(&cd_ext, sizeof(cd_ext), 1, zip_info->dir);
		zip_info->dir_size+=sizeof(cd_ext);
	}
#ifdef HAVE_LIBCRYPTO
	if (zip_info->passwd) {
		fwrite(&enc, sizeof(enc), 1, zip_info->dir);
		zip_info->dir_size+=sizeof(enc);
	}
#endif
	
	free(compbuffer);
}

void
zip_write_index(struct zip_info *info)
{
	int size=ftell(info->index);
	char buffer[size];

	fseek(info->index, 0, SEEK_SET);
	fread(buffer, size, 1, info->index);
	write_zipmember(info, "index", strlen("index"), buffer, size);
	info->zipnum++;
}

static void
zip_write_file_data(struct zip_info *info, FILE *in)
{
	size_t size;
	char buffer[4096];
	while ((size=fread(buffer, 1, 4096, in)))
		zip_write(info, buffer, size);
}

int
zip_write_directory(struct zip_info *info)
{
	struct zip_eoc eoc = {
		0x06054b50,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0,
		0x0,
		0x0,
	};
	struct zip64_eoc eoc64 = {
		0x06064b50,
		0x0,
		0x0,
		0x0403,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,		
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
		eoc64.zip64esize=sizeof(eoc64)-12;
		eoc64.zip64enum=info->zipnum;
		eoc64.zip64ecenn=info->zipnum;
		eoc64.zip64ecsz=info->dir_size;
		eoc64.zip64eofst=info->offset;
		zip_write(info, &eoc64, sizeof(eoc64));
		eocl.zip64lofst=info->offset+info->dir_size;
		zip_write(info, &eocl, sizeof(eocl));
	}
	eoc.zipenum=info->zipnum;
	eoc.zipecenn=info->zipnum;
	eoc.zipecsz=info->dir_size;
	eoc.zipeofst=info->offset;
	zip_write(info, &eoc, sizeof(eoc));
	sig_alrm(0);
#ifndef _WIN32
	alarm(0);
#endif
	return 0;
}

struct zip_info *
zip_new(void)
{
	return g_new0(struct zip_info, 1);
}

void
zip_set_md5(struct zip_info *info, int on)
{
#ifdef HAVE_LIBCRYPTO
	info->md5=on;
	if (on) 
		MD5_Init(&info->md5_ctx);
#endif
}

int
zip_get_md5(struct zip_info *info, unsigned char *out)
{
	if (!info->md5)
		return 0;
#ifdef HAVE_LIBCRYPTO
	MD5_Final(out, &info->md5_ctx);
	return 1;
#endif
	return 0;
}

void
zip_set_zip64(struct zip_info *info, int on)
{
	info->zip64=on;
}

void
zip_set_compression_level(struct zip_info *info, int level)
{
	info->compression_level=level;
}

void
zip_set_maxnamelen(struct zip_info *info, int max)
{
	info->maxnamelen=max;
}

int
zip_get_maxnamelen(struct zip_info *info)
{
	return info->maxnamelen;
}

int
zip_add_member(struct zip_info *info)
{
	return info->zipnum++;
}


int
zip_set_timestamp(struct zip_info *info, char *timestamp)
{
	int year,month,day,hour,min,sec;

	if (sscanf(timestamp,"%d-%d-%dT%d:%d:%d",&year,&month,&day,&hour,&min,&sec) == 6) {
		info->date=day | (month << 5) | ((year-1980) << 9);
		info->time=(sec >> 1) | (min << 5) | (hour << 11);
		return 1;
	}
	return 0;
}

void
zip_open(struct zip_info *info, char *out, char *dir, char *index)
{
	info->res2=fopen(out,"wb+");
	info->dir=fopen(dir,"wb+");
	info->index=fopen(index,"wb+");
}

FILE *
zip_get_index(struct zip_info *info)
{
	return info->index;
}

int
zip_get_zipnum(struct zip_info *info)
{
	return info->zipnum;
}

void
zip_set_zipnum(struct zip_info *info, int num)
{
	info->zipnum=num;
}

void
zip_close(struct zip_info *info)
{
	fclose(info->index);
	fclose(info->dir);
	fclose(info->res2);
}

void
zip_destroy(struct zip_info *info)
{
	g_free(info);
}
