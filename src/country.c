#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <libintl.h>
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

#define gettext_noop(String) String
#define _(STRING)    gettext(STRING)
#define _n(STRING)    gettext_noop(STRING)

struct country country[]= {
	{ 1 ,"CZ",	"CZ",	"CZE",	_n("Czech Republic")},
	{ 2 ,"SK",	"SK",	"SVK",	_n("Slovakia")},
	{ 7 ,"RSM",	"SM",	"SMR",	_n("San Marino")},
	{11 ,"EST",	"EE",	"EST",	_n("Estonia")},
	{12 ,"GE",	"GE",	"GEO",	_n("Georgia")},
	{13 ,"LV",	"LV",	"LVA",	_n("Latvia")},
	{14 ,"LT",	"LT",	"LTU",	_n("Lithuania")},
	{15 ,"MD",	"MD",	"MDA",	_n("Moldova")},
	{16 ,"RUS",	"RU",	"RUS",	_n("Russian Federation")},
	{17 ,"UA",	"UA",	"UKR",	_n("Ukraine")},
	{18 ,"BY",	"BY",	"BLR",	_n("Belarus")},
	{20 ,"ET",	"EG",	"EGY",	_n("Egypt")},
	{30 ,"GR",	"GR",	"GRC",	_n("Greece")},
	{31 ,"NL",	"NL",	"NLD",	_n("Netherlands")},
	{32 ,"B",	"BE",	"BEL",	_n("Belgium")},
	{33 ,"F",	"FR",	"FRA",	_n("France")},
	{34 ,"E",	"ES",	"ESP",	_n("Spain")},
	{36 ,"H",	"HU",	"HUN",	_n("Hungary")},
	{39 ,"I",	"IT",	"ITA",	_n("Italy")},
	{40 ,"RO",	"RO",	"ROM",	_n("Romania")},
	{41 ,"CH",	"CH",	"CHE",	_n("Switzerland")},
	{43 ,"A",	"AT",	"AUT",	_n("Austria")},
	{44 ,"GB",	"GB",	"GBR",	_n("United Kingdom")},
	{45 ,"DK",	"DK",	"DNK",	_n("Denmark")},
	{46 ,"S",	"SE",	"SWE",	_n("Sweden")},
	{47 ,"N",	"NO",	"NOR",	_n("Norway")},
	{48 ,"PL",	"PL",	"POL",	_n("Poland")},
	{49 ,"D",	"DE",	"DEU",	_n("Germany")},
	{50 ,"GBZ",	"GI",	"GIB",	_n("Gibraltar")},
	{51 ,"P",	"PT",	"PRT",	_n("Portugal")},
	{52 ,"L",	"LU",	"LUX",	_n("Luxembourg")},
	{53 ,"IRL",	"IE",	"IRL",	_n("Ireland")},
	{54 ,"IS",	"IS",	"ISL",	_n("Iceland")},
	{55 ,"AL",	"AL",	"ALB",	_n("Albania")},
	{56 ,"M",	"MT",	"MLT",	_n("Malta")},
	{57 ,"CY",	"CY",	"CYP",	_n("Cyprus")},
	{58 ,"FIN",	"FI",	"FIN",	_n("Finland")},
	{59 ,"BG",	"BG",	"BGR",	_n("Bulgaria")},
	{61 ,"RL",	"LB",	"LBN",	_n("Lebanon")},
	{62 ,"AND",	"AD",	"AND",	_n("Andorra")},
	{63 ,"SYR",	"SY",	"SYR",	_n("Syria")},
	{66 ,"KSA",	"SA",	"SAU",	_n("Saudi Arabia")},
	{71 ,"LAR",	"LY",	"LYB",	_n("Libia")},
	{72 ,"IL",	"IL",	"ISR",	_n("Israel")},
	{73 ,"AUT",	"PS",	"PSE",	_n("Palestinia")},
	{75 ,"FL",	"LI",	"LIE",	_n("Liechtenstein")},
	{76 ,"MA",	"MA",	"MAR",	_n("Morocco")},
	{77 ,"DZ",	"DZ",	"DZA",	_n("Algeria")},
	{78 ,"TN",	"TN",	"TUN",	_n("Tunisia")},
	{81 ,"SRB",	"RS",	"SRB",	_n("Serbia")},
	{83 ,"HKJ",	"JO",	"JOR",	_n("Jordan")},
	{85 ,"NDH",	"HR",	"HRV",	_n("Croatia")},
	{86 ,"SLO",	"SI",	"SVN",	_n("Slovenia")},
	{87 ,"BIH",	"BA",	"BIH",	_n("Bosnia and Herzegovina")},
	{89 ,"MK",	"MK",	"MKD",	_n("Macedonia")},
	{90 ,"TR",	"TR",	"TUR",	_n("Turkey")},
	{93 ,"MC",	"MC",	"MCO",	_n("Monaco")},
	{94 ,"AZ",	"AZ",	"AZE",	_n("Azerbaijan")},
	{95 ,"ARM",	"AM",	"ARM",	_n("Armenia")},
	{98 ,"FO",	"FO",	"FRO",	_n("Faroe Islands")},
	{99 ,"WSA",	"EH",	"ESH",	_n("Western Sahara")},
	{336 ,NULL,	"SJ",	"SJM",	_n("Svalbard and Jan Mayen")},
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
		attr->u.str=gettext(country->name);
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
		attr->u.str=gettext(country->name);
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
		    match(this, attr_country_name, gettext(this->country->name))) {
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
