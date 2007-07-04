#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "attr.h"
#include "item.h"
#include "country.h"
#include "search.h"

struct country {
	int id;
	char *car;
	char *iso2;
	char *iso3;
	char *name;
};

struct country country[]= {
	{ 1 ,"CZ",	"CZ",	"CZE",	"Tschechische Republik"},
	{ 2 ,"SK",	"SK",	"SVK",	"Slowakei"},
	{ 7 ,"RSM",	"SM",	"SMR",	"San Marino"},
	{11 ,"EST",	"EE",	"EST",	"Estland"},
	{12 ,"GE",	"GE",	"GEO",	"Georgien"},
	{13 ,"LV",	"LV",	"LVA",	"Lettland"},
	{14 ,"LT",	"LT",	"LTU",	"Litauen"},
	{15 ,"MD",	"MD",	"MDA",	"Moldawien"},
	{16 ,"RUS",	"RU",	"RUS",	"Rußland"},
	{17 ,"UA",	"UA",	"UKR",	"Ukraine"},
	{18 ,"BY",	"BY",	"BLR",	"Weißrussland"},
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
	{46 ,"S",	"SE",	"SWE",	"Schweden"},
	{47 ,"N",	"NO",	"NOR",	"Norwegen"},
	{48 ,"PL",	"PL",	"POL",	"Polen"},
	{49 ,"D",	"DE",	"DEU",	"Deutschland"},
	{50 ,"GBZ",	"GI",	"GIB",	"Gibraltar"},
	{51 ,"P",	"PT",	"PRT",	"Portugal"},
	{52 ,"L",	"LU",	"LUX",	"Luxemburg"},
	{53 ,"IRL",	"IE",	"IRL",	"Irland"},
	{54 ,"IS",	"IS",	"ISL",	"Island"},
	{55 ,"AL",	"AL",	"ALB",	"Albanien"},
	{56 ,"M",	"MT",	"MLT",	"Malta"},
	{57 ,"CY",	"CY",	"CYP",	"Zypern"},
	{58 ,"FIN",	"FI",	"FIN",	"Finnland"},
	{59 ,"BG",	"BG",	"BGR",	"Bulgarien"},
	{61 ,"RL",	"LB",	"LBN",	"Libanon"},
	{62 ,"AND",	"AD",	"AND",	"Andorra"},
	{63 ,"SYR",	"SY",	"SYR",	"Syrien"},
	{66 ,"KSA",	"SA",	"SAU",	"Saudi-Arabien"},
	{71 ,"LAR",	"LY",	"LYB",	"Libyen"},
	{72 ,"IL",	"IL",	"ISR",	"Israel"},
	{73 ,"AUT",	"PS",	"PSE",	"Palästinensische Autonomiegebiete"},
	{75 ,"FL",	"LI",	"LIE",	"Liechtenstein"},
	{76 ,"MA",	"MA",	"MAR",	"Marokko"},
	{77 ,"DZ",	"DZ",	"DZA",	"Algerien"},
	{78 ,"TN",	"TN",	"TUN",	"Tunesien"},
	{81 ,"SRB",	"RS",	"SRB",	"Serbien"},
	{83 ,"HKJ",	"JO",	"JOR",	"Jordanien"},
	{85 ,"NDH",	"HR",	"HRV",	"Kroatien"},
	{86 ,"SLO",	"SI",	"SVN",	"Slowenien"},
	{87 ,"BIH",	"BA",	"BIH",	"Bosnien und Herzegowina"},
	{89 ,"MK",	"MK",	"MKD",	"Mazedonien"},
	{90 ,"TR",	"TR",	"TUR",	"Türkei"},
	{93 ,"MC",	"MC",	"MCO",	"Monaco"},
	{94 ,"AZ",	"AZ",	"AZE",	"Aserbaidschan"},
	{95 ,"ARM",	"AM",	"ARM",	"Armenien"},
	{98 ,"FO",	"FO",	"FRO",	"Färöer"},
	{99 ,"WSA",	"EH",	"ESH",	"Westsahara"},
	{336 ,NULL,	"SJ",	"SJM",	"Svalbard und Jan Mayen"},
};


struct country_search {
	struct attr search;
	int len;
	int partial;
	struct item item;
	int count;
	struct country *country;
	enum attr_type attr_next;
};

static int
country_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
        struct country_search *this=priv_data;
	struct country *country=this->country;

        attr->type=attr_type;
        switch (attr_type) {
        case attr_any:
                while (this->attr_next != attr_none) {
                        if (country_attr_get(this, this->attr_next, attr))
                                return 1;
                }
                return 0;
        case attr_label:
		attr->u.str=country->name;
		this->attr_next=attr_id;
		return 1;
	case attr_id:
		attr->u.num=country->id;
		this->attr_next=country->car ? attr_country_car : attr_country_iso2;
		return 1;
        case attr_country_car:
		attr->u.str=country->car;
		this->attr_next=attr_country_iso2;
		return attr->u.str ? 1 : 0;
        case attr_country_iso2:
		attr->u.str=country->iso2;
		this->attr_next=attr_country_iso3;
		return 1;
        case attr_country_iso3:
		attr->u.str=country->iso3;
		this->attr_next=attr_country_name;
		return 1;
        case attr_country_name:
		attr->u.str=country->name;
		this->attr_next=attr_none;
		return 1;
 	default:
                return 0;
        }
}



struct item_methods country_meth = {
	NULL, 			/* coord_rewind */
	NULL, 			/* coord_get */
	NULL, 			/* attr_rewind */
	country_attr_get, 	/* attr_get */
};

struct country_search *
country_search_new(struct attr *search, int partial)
{
	struct country_search *ret=g_new(struct country_search, 1);
	ret->search=*search;
	ret->len=strlen(search->u.str);
	ret->partial=partial;
	ret->count=0;

	ret->item.type=type_country_label;
	ret->item.id_hi=0;		
	ret->item.map=NULL;
	ret->item.meth=&country_meth;
	ret->item.priv_data=ret;

	return ret;
}

static int
match(struct country_search *this, enum attr_type type, const char *name)
{
	int ret;
	if (!name)
		return 0;
	if (this->search.type != type && this->search.type != attr_country_all)
		return 0;
	if (this->partial)
		ret=(strncasecmp(this->search.u.str, name, this->len) == 0);
	else
		ret=(strcasecmp(this->search.u.str, name) == 0);
	return ret;
	
}


struct item *
country_search_get_item(struct country_search *this)
{
	for (;;) {
		if (this->count >= sizeof(country)/sizeof(struct country))
			return NULL;
		this->country=&country[this->count++];
		if (match(this, attr_country_iso3, this->country->iso3) ||
		    match(this, attr_country_iso2, this->country->iso2) ||
		    match(this, attr_country_car, this->country->car) ||
		    match(this, attr_country_name, this->country->name)) {
			this->item.id_lo=this->country->id;
			return &this->item;
		}
	}
}

void
country_search_destroy(struct country_search *this)
{
	g_free(this);
}
