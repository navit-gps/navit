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
#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "file.h"
#include "zipfile.h"
#include "types.h"
#include "endianess.h"

static void lfh_to_cpu(struct zip_lfh *lfh) {
	dbg_assert(lfh != NULL);
	if (lfh->ziplocsig != zip_lfh_sig) {
		lfh->ziplocsig = le32_to_cpu(lfh->ziplocsig);
		lfh->zipver    = le16_to_cpu(lfh->zipver);
		lfh->zipgenfld = le16_to_cpu(lfh->zipgenfld);
		lfh->zipmthd   = le16_to_cpu(lfh->zipmthd);
		lfh->ziptime   = le16_to_cpu(lfh->ziptime);
		lfh->zipdate   = le16_to_cpu(lfh->zipdate);
		lfh->zipcrc    = le32_to_cpu(lfh->zipcrc);
		lfh->zipsize   = le32_to_cpu(lfh->zipsize);
		lfh->zipuncmp  = le32_to_cpu(lfh->zipuncmp);
		lfh->zipfnln   = le16_to_cpu(lfh->zipfnln);
		lfh->zipxtraln = le16_to_cpu(lfh->zipxtraln);
	}
}

void cd_to_cpu(struct zip_cd *zcd) {
	dbg_assert(zcd != NULL);
	if (zcd->zipcensig != zip_cd_sig) {
		zcd->zipcensig = le32_to_cpu(zcd->zipcensig);
		zcd->zipccrc   = le32_to_cpu(zcd->zipccrc);
		zcd->zipcsiz   = le32_to_cpu(zcd->zipcsiz);
		zcd->zipcunc   = le32_to_cpu(zcd->zipcunc);
		zcd->zipcfnl   = le16_to_cpu(zcd->zipcfnl);
		zcd->zipcxtl   = le16_to_cpu(zcd->zipcxtl);
		zcd->zipccml   = le16_to_cpu(zcd->zipccml);
		zcd->zipdsk    = le16_to_cpu(zcd->zipdsk);
		zcd->zipint    = le16_to_cpu(zcd->zipint);
		zcd->zipext    = le32_to_cpu(zcd->zipext);
		zcd->zipofst   = le32_to_cpu(zcd->zipofst);
	}
}

static void eoc_to_cpu(struct zip_eoc *eoc) {
	dbg_assert(eoc != NULL);
	if (eoc->zipesig != zip_eoc_sig) {
		eoc->zipesig   = le32_to_cpu(eoc->zipesig);
		eoc->zipedsk   = le16_to_cpu(eoc->zipedsk);
		eoc->zipecen   = le16_to_cpu(eoc->zipecen);
		eoc->zipenum   = le16_to_cpu(eoc->zipenum);
		eoc->zipecenn  = le16_to_cpu(eoc->zipecenn);
		eoc->zipecsz   = le32_to_cpu(eoc->zipecsz);
		eoc->zipeofst  = le32_to_cpu(eoc->zipeofst);
		eoc->zipecoml  = le16_to_cpu(eoc->zipecoml);
	}
}

/**
 * @brief Read the "end of central directory" structure, verifying its signature
 * @param fi file to read from
 * @return structure read, or NULL if file cannot be read or eoc signature is wrong
 */
struct zip_eoc *
zipfile_read_eoc(struct file *fi)
{
	struct zip_eoc *eoc;
	eoc=(struct zip_eoc *)file_data_read(fi,fi->size-sizeof(struct zip_eoc), sizeof(struct zip_eoc));
	if (eoc) {
		eoc_to_cpu(eoc);
		dbg(lvl_debug,"sig 0x%x\n", eoc->zipesig);
		if (eoc->zipesig != zip_eoc_sig) {
			dbg(lvl_error,"map file %s: eoc signature check failed: 0x%x vs 0x%x\n", fi->name, eoc->zipesig,zip_eoc_sig);
			file_data_free(fi,(unsigned char *)eoc);
			eoc=NULL;
		}
	}
	return eoc;
}

/**
 * @brief Read the 64 bit "end of central directory" structure, verifying its signature
 * @param fi file to read from
 * @return structure read, or NULL if file cannot be read or eoc64 signature is wrong
 */
struct zip64_eoc *
zipfile_read_eoc64(struct file *fi)
{
	struct zip64_eocl *eocl;
	struct zip64_eoc *eoc;
	eocl=(struct zip64_eocl *)file_data_read(fi,fi->size-sizeof(struct zip_eoc)-sizeof(struct zip64_eocl), sizeof(struct zip64_eocl));
	if (!eocl)
		return NULL;
	dbg(lvl_debug,"sig 0x%x\n", eocl->zip64lsig);
	if (eocl->zip64lsig != zip64_eocl_sig) {
		file_data_free(fi,(unsigned char *)eocl);
		dbg(lvl_warning,"map file %s: eocl wrong\n", fi->name);
		return NULL;
	}
	eoc=(struct zip64_eoc *)file_data_read(fi,eocl->zip64lofst, sizeof(struct zip64_eoc));
	if (eoc) {
		if (eoc->zip64esig != zip64_eoc_sig) {
			file_data_free(fi,(unsigned char *)eoc);
			dbg(lvl_warning,"map file %s: eoc wrong\n", fi->name);
			eoc=NULL;
		}
		dbg(lvl_debug,"eoc64 ok 0x"LONGLONG_HEX_FMT " 0x"LONGLONG_HEX_FMT "\n",eoc->zip64eofst,eoc->zip64ecsz);
	}
	file_data_free(fi,(unsigned char *)eocl);
	return eoc;
}

/**
 * @brief Get size of data needed to read after a given central struct zip_cd to fetch
 *    both file name and "extra field".
 * @param cd zip_cd structure
 * @return count of bytes in filename and extra field
 */
int
zipfile_cd_name_and_extra_len(struct zip_cd *cd)
{
	return cd->zipcfnl+cd->zipcxtl;
}

/**
 * @brief Get the zip central directory header, change byte order if needed and verify signature.
 * @param fi zip file to read from
 * @param eoc zip end of central directory structure (pre-fetched)
 * @param eoc64 zip64 end of central directory structure (pre-fetched)
 * @param offset offset relative to beginnning of the central directory to read from
 * @param len amount of bytes to read, -1 to read struct zip_cd and following filename and extra field
 * @return central directory data read
 */
struct zip_cd *
zipfile_read_cd(struct file *fi, struct zip_eoc *eoc, struct zip64_eoc *eoc64, int offset, int len)
{
	struct zip_cd *cd;
	long long cdoffset=eoc64?eoc64->zip64eofst:eoc->zipeofst;
	if (len == -1) {
		cd=(struct zip_cd *)file_data_read(fi,cdoffset+offset, sizeof(*cd));
		if(!cd)
			return NULL;
		cd_to_cpu(cd);
		len=zipfile_cd_name_and_extra_len(cd);
		file_data_free(fi,(unsigned char *)cd);
	}
	cd=(struct zip_cd *)file_data_read(fi,cdoffset+offset, sizeof(*cd)+len);
	if (cd) {
		dbg(lvl_debug,"cd at "LONGLONG_FMT" %zu bytes\n",cdoffset+offset, sizeof(*cd)+len);
		cd_to_cpu(cd);
		dbg(lvl_debug,"sig 0x%x\n", cd->zipcensig);
		if (cd->zipcensig != zip_cd_sig) {
			file_data_free(fi,(unsigned char *)cd);
			cd=NULL;
		}
	}
	return cd;
}

/**
 * @brief Get the ZIP64 extra field data corresponding to a zip central
 * directory header.
 *
 * @param cd pointer to zip central directory structure
 * @return pointer to ZIP64 extra field, or NULL if not available
 */
struct zip_cd_ext *
zipfile_cd_ext(struct zip_cd *cd)
{
	struct zip_cd_ext *ext;
	if (cd->zipofst != zip_size_64bit_placeholder)
		return NULL;
	if (cd->zipcxtl != sizeof(*ext))
		return NULL;
	ext=(struct zip_cd_ext *)((unsigned char *)cd+sizeof(*cd)+cd->zipcfnl);
	if (ext->tag != zip_extra_header_id_zip64 || ext->size != 8)
		return NULL;
	return ext;
}

/**
 * @brief Get local file header offset for a given central directory element.
 * Will use ZIP64 data if present.
 * @param cd pointer to zip central directory structure
 * @return Offset of local file header in zip file.
 */
long long
zipfile_lfh_offset(struct zip_cd *cd)
{
	struct zip_cd_ext *ext=zipfile_cd_ext(cd);
	if (ext)
		return ext->zipofst;
	else
		return cd->zipofst;
}

/**
 * @brief Read local file header at a given offset, change byte order if needed and verify signature. 
 * @param fi file to read from
 * @param offset offset to read at
 * @return local file header structure read.
 */
struct zip_lfh *
zipfile_read_lfh(struct file *fi, long long offset)
{
	struct zip_lfh *lfh;

	lfh=(struct zip_lfh *)(file_data_read(fi,offset,sizeof(struct zip_lfh)));
	if (lfh) {
		lfh_to_cpu(lfh);
		if (lfh->ziplocsig != zip_lfh_sig) {
			file_data_free(fi,(unsigned char *)lfh);
			lfh=NULL;
		}
	}
	return lfh;
}

unsigned char *
zipfile_read_content(struct file *fi, long long offset, struct zip_lfh *lfh, char *passwd)
{
	struct zip_enc *enc;
	unsigned char *ret=NULL;

	offset+=sizeof(struct zip_lfh)+lfh->zipfnln;
	switch (lfh->zipmthd) {
	case 0:
		offset+=lfh->zipxtraln;
		ret=file_data_read(fi,offset, lfh->zipuncmp);
		break;
	case 8:
		offset+=lfh->zipxtraln;
		ret=file_data_read_compressed(fi,offset, lfh->zipsize, lfh->zipuncmp);
		break;
	case 99:
		if (!passwd)
			break;
		enc=(struct zip_enc *)file_data_read(fi, offset, sizeof(*enc));
		offset+=lfh->zipxtraln;
		switch (enc->compress_method) {
		case 0:
			ret=file_data_read_encrypted(fi, offset, lfh->zipsize, lfh->zipuncmp, 0, passwd);
			break;
		case 8:
			ret=file_data_read_encrypted(fi, offset, lfh->zipsize, lfh->zipuncmp, 1, passwd);
			break;
		default:
			dbg(lvl_error,"zip file %s: unknown encrypted compression method %d\n", fi->name, enc->compress_method);
		}
		file_data_free(fi, (unsigned char *)enc);
		break;
	default:
		dbg(lvl_error,"zip file %s: unknown compression method %d\n", fi->name, lfh->zipmthd);
	}
	return ret;
}

void 
zipfile_init(void)
{
}

