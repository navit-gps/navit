/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __ZIPFILE_H__
#define __ZIPFILE_H__

#ifdef HAVE_PRAGMA_PACK
#pragma pack(push)
#pragma pack(1)
#endif

#ifdef  __GNUC__
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#else
#define ATTRIBUTE_PACKED
#endif

#define zip_split_sig 0x08074b50
#define zip_split_sig_rev 0x504b0708

struct zip_split {
	int zipsplitsig;
};

#define zip_lfh_sig 0x04034b50
#define zip_lfh_sig_rev 0x504b0304


//! ZIP local file header structure.

//! See the documentation of the ZIP format for the meaning
//! of these fields.
struct zip_lfh {
	int ziplocsig;             //!< local file header signature
	short zipver; 	           //!< minimum zip spec version needed to extract
	short zipgenfld;	   //!< general purpose flags
	short zipmthd;		   //!< compression method
	short ziptime;	      	   //!< file modification time
	short zipdate;		   //!< file modification date
	int zipcrc;		   //!< CRC-32 checksum
	unsigned int zipsize;      //!< file size (after compression)
	unsigned int zipuncmp;     //!< file size (uncompressed)
	unsigned short zipfnln;    //!< file name length
	unsigned short zipxtraln;  //!< extra filed length (unused?)
	char zipname[0];           //!< file name (length as given above)
} ATTRIBUTE_PACKED;

#define zip_cd_sig 0x02014b50
#define zip_cd_sig_rev 0x504b0102

//! ZIP central directory structure.

//! See the documentation of the ZIP format for the meaning
//! of these fields.
struct zip_cd {
	int zipcensig;   //!< central directory signature
	char zipcver;    //!< zip spec version of creating software
	char zipcos;     //!< os compatibility of the file attribute information
	char zipcvxt;    //!< minimum zip spec version needed to extract
	char zipcexos;   //!< unused (?)
	short zipcflg;   //!< general purpose flag
	short zipcmthd;  //!< compression method
	short ziptim;    //!< file modification time
	short zipdat;    //!< file modification date
	int zipccrc;     //!< CRC-32 checksum
	unsigned int zipcsiz;   //!< file size (after compression)
	unsigned int zipcunc;   //!< file size (uncompressed)
	unsigned short zipcfnl; //!< file name length
	unsigned short zipcxtl; //!< extra field length
	unsigned short zipccml; //!< comment length
	unsigned short zipdsk;  //!< disk number of file
	unsigned short zipint;  //!< internal attributes
	unsigned int zipext;    //!< external attributes
	unsigned int zipofst;   //!< offset to start of local file header
	char zipcfn[0];	        //!< file name (length as given above)
} ATTRIBUTE_PACKED;

/**
* @brief Placeholder value for size field in the central directory if
* the size is 64bit. See the documentation for the Zip64 Extended
* Information Extra Field.
*/
#define zip_size_64bit_placeholder 0xffffffff

/**
* @brief Header ID for extra field "ZIP64".
*/
#define zip_extra_header_id_zip64 0x0001

//! ZIP extra field structure.

//! See the documentation of the ZIP format for the meaning
//! of these fields.
struct zip_cd_ext {
	short tag;                   //!< extra field header ID
	short size;                  //!< extra field data size
	unsigned long long zipofst;  //!< offset to start of local file header (only valid if the struct is for a ZIP64 extra field)
} ATTRIBUTE_PACKED;

struct zip_enc {
	short efield_header;
	short efield_size;
	short version;
	char vendor_id1,vendor_id2;
	char encryption_strength;
	short compress_method;
} ATTRIBUTE_PACKED;

#define zip_eoc_sig 0x6054b50
#define zip_eoc_sig_rev 0x504b0506

//! ZIP end of central directory structure.

//! See the documentation of the ZIP format for the meaning
//! of these fields.
struct zip_eoc {
	int zipesig; 		/* end of central dir signature */
	unsigned short zipedsk; /* number of this disk */
	unsigned short zipecen; /* number of the disk with the start of the central directory */
	unsigned short zipenum; /* total number of entries in the central directory on this disk */
	unsigned short zipecenn; /* total number of entries in the central directory */
	unsigned int zipecsz; 	/* size of the central directory */
	unsigned int zipeofst; 	/* offset of start of central directory with respect to the starting disk number */
	short zipecoml; 	/* .ZIP file comment length */
	char zipecom[0];	/* .ZIP file comment */
} ATTRIBUTE_PACKED;

#define zip64_eoc_sig 0x6064b50
#define zip64_eoc_sig_rev 0x504b0606

struct zip64_eoc {
	int zip64esig;			/* zip64 end of central dir signature */
	unsigned long long zip64esize;	/* size of zip64 end of central directory record */
	unsigned short zip64ever;	/* version made by */
	unsigned short zip64eneed;	/* version needed to extract */
	unsigned int zip64edsk;		/* number of this disk */
	unsigned int zip64ecen;		/* number of the disk with the start of the central directory */
	unsigned long long zip64enum; 	/* total number of entries in the central directory on this disk */
	unsigned long long zip64ecenn;	/* total number of entries in the central directory */
	unsigned long long zip64ecsz;	/* size of the central directory */
	unsigned long long zip64eofst;	/* offset of start of central directory with respect to the starting disk number */
	char zip64ecom[0];		/* zip64 extensible data sector */
} ATTRIBUTE_PACKED;

#define zip64_eocl_sig 0x07064b50

struct zip64_eocl {
	int zip64lsig;
	int zip64ldsk;
	long long zip64lofst;
	int zip74lnum;
} ATTRIBUTE_PACKED;

struct zip_alignment_check {
	int x[sizeof(struct zip_cd) == 46 ? 1:-1];
};

#ifdef HAVE_PRAGMA_PACK
#pragma pack(pop)
#endif
#endif
