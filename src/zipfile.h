struct zip_lfh {
	int ziplocsig;
	short zipver;
	short zipgenfld;
	short zipmthd;
	short ziptime;
	short zipdate;
	int zipcrc;
	int zipsize;
	int zipuncmp;
	short zipfnln;
	short zipxtraln;
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
	int zipcsiz;
	int zipcunc;
	short zipcfnl;
	short zipcxtl;
	short zipccml;
	short zipdsk;
	short zipint;
	int zipext;
	int zipofst;
	char zipcfn[0];	
} __attribute__ ((packed));

struct zip_eoc {
	int zipesig;
	short zipedsk;
	short zipecen;
	short zipenum;
	short zipecenn;
	int zipecsz;
	int zipeofst;
	short zipecoml;
	char zipecom[0];
} __attribute__ ((packed));
