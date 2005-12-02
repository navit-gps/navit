#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "country.h"

struct country country[]= {
	{16 ,"RUS",	"RU",	"RUS",	"Rußland"},
	{20 ,"ET",	"EG",	"EGY",	"Ägypten"},
	{30 ,"GR",	"GR",	"GRC",	"Griechenland"},
	{31 ,"NL",	"NL",	"NLD",	"Niederlande"},
	{32 ,"B",	"BE",	"BEL",	"Belgien"},
	{33 ,"F",	"FR",	"FRA",	"Frankreich"},
	{34 ,"E",	"ES",	"ESP",	"Spanien"},
	{36 ,"H",	"HU",	"HUN",	"Ungarn"},
	{39 ,"I",	"IT",	"ITA",	"Italien"},
	{40 ,"RO",	"RO",	"ROM",	"Rumänien"},
	{41 ,"CH",	"CH",	"CHE",	"Schweiz"},
	{43 ,"A",	"AT",	"AUT",	"Österreich"},
	{44 ,"GB",	"GB",	"GBR",	"Grossbritannien"},
	{45 ,"DK",	"DK",	"DNK",	"Dänemark"},
	{47 ,"N",	"NO",	"NOR",	"Norwegen"},
	{49 ,"D",	"DE",	"DEU",	"Deutschland"},
	{51 ,"P",	"PT",	"PRT",	"Portugal"},
	{52 ,"L",	"LU",	"LUX",	"Luxemburg"},
	{71 ,"LAR",	"LY",	"LYB",	"Libyen"},
	{76 ,"MA",	"MA",	"MAR",	"Marokko"},
	{78 ,"TN",	"TN",	"TUN",	"Tunesien"},
};

struct country *
country_get_by_id(int id)
{
	int i;
	for (i=0 ; i < sizeof(country)/sizeof(struct country); i++) {
		if (id == country[i].id) {
			return &country[i];
		}
	}
	return NULL;
}

static int
compare(const char *name1, const char *name2, int len, int partial)
{
	if (partial)
		return strncasecmp(name1, name2, len);
	else
		return strcasecmp(name1, name2);
}

static int
search(int offset, const char *name, int partial, int (*func)(struct country *cou, void *data), void *data)
{
	char *col;
	int i,ret,len=strlen(name);
	int debug=0;

	for (i=0 ; i < sizeof(country)/sizeof(struct country); i++) {
		col=G_STRUCT_MEMBER(char *,country+i,offset);
		if (debug)
			printf("comparing '%s'\n", col);
		if (!compare(col, name, len, partial)) {
			ret=(*func)(&country[i], data);
			if (ret)
				return 1;
		}
		col+=sizeof(struct country);
	}
	return 0;
}

int
country_search_by_name(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data)
{
	return search(G_STRUCT_OFFSET(struct country, name), name, partial, func, data);
}

int
country_search_by_car(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data)
{
	return search(G_STRUCT_OFFSET(struct country, car), name, partial, func, data);
}

int
country_search_by_iso2(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data)
{
	return search(G_STRUCT_OFFSET(struct country, iso2), name, partial, func, data);
}

int
country_search_by_iso3(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data)
{
	return search(G_STRUCT_OFFSET(struct country, iso3), name, partial, func, data);
}
