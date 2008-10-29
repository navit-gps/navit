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
#ifdef HAVE_API_WIN32_CE
#warning OK
/* cegcc doesn't honor the __attribute__(packed) need pragma to work */
#pragma pack(push)
#pragma pack(1)
#endif

#define zip_lfh_sig 0x04034b50

struct zip_lfh {
	int ziplocsig;
	short zipver;
	short zipgenfld;
	short zipmthd;
	short ziptime;
	short zipdate;
	int zipcrc;
	unsigned int zipsize;
	unsigned int zipuncmp;
	unsigned short zipfnln;
	unsigned short zipxtraln;
	char zipname[0];
} __attribute__ ((packed));

#define zip_cd_sig 0x02014b50

struct zip_cd {
	int zipcensig;
	char zipcver;
	char zipcos;
	char zipcvxt;
	char zipcexos;
	short zipcflg;
	short zipcmthd;
	short ziptim;
	short zipdat;
	int zipccrc;
	unsigned int zipcsiz;
	unsigned int zipcunc;
	unsigned short zipcfnl;
	unsigned short zipcxtl;
	unsigned short zipccml;
	unsigned short zipdsk;
	unsigned short zipint;
	unsigned int zipext;
	unsigned int zipofst;
	char zipcfn[0];	
} __attribute__ ((packed));

#define zip_eoc_sig 0x6054b50

struct zip_eoc {
	int zipesig;
	unsigned short zipedsk;
	unsigned short zipecen;
	unsigned short zipenum;
	unsigned short zipecenn;
	unsigned int zipecsz;
	unsigned int zipeofst;
	short zipecoml;
	char zipecom[0];
} __attribute__ ((packed));

#define __ZIPFILE_H__
#ifdef __CEGCC__
#pragma pack(pop)
#endif

#endif
