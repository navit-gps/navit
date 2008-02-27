#ifndef __ZIPFILE_H__

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

#endif
