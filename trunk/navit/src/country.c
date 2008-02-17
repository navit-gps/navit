#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
#include "debug.h"
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
  { 20,	"AND",	"AD", "AND", _n("Andorra")},
  {784,	"UAE",	"AE", "ARE", _n("United Arab Emirates")},
  {  4,	"AFG",	"AF", "AFG", _n("Afghanistan")},
  { 28,	"AG",	"AG", "ATG", _n("Antigua and Barbuda")},
  {660,	NULL,	"AI", "AIA", _n("Anguilla")},
  {  8,	"AL",	"AL", "ALB", _n("Albania")},
  { 51,	"ARM",	"AM", "ARM", _n("Armenia")},
  {530,	"NA",	"AN", "ANT", _n("Netherlands Antilles")},
  { 24,	"ANG",	"AO", "AGO", _n("Angola")},
  { 10,	NULL,	"AQ", "ATA", _n("Antarctica")},
  { 32,	"RA",	"AR", "ARG", _n("Argentina")},
  { 16,	NULL,	"AS", "ASM", _n("American Samoa")},
  { 40,	"A",	"AT", "AUT", _n("Austria")},
  { 36,	"AUS",	"AU", "AUS", _n("Australia")},
  {533,	"ARU",	"AW", "ABW", _n("Aruba")},
  {248,	"AX",	"AX", "ALA", _n("Aland Islands")},
  { 31,	"AZ",	"AZ", "AZE", _n("Azerbaijan")},
  { 70,	"BiH",	"BA", "BIH", _n("Bosnia and Herzegovina")},
  { 52,	"BDS",	"BB", "BRB", _n("Barbados")},
  { 50,	"BD",	"BD", "BGD", _n("Bangladesh")},
  { 56,	"B",	"BE", "BEL", _n("Belgium")},
  {854,	"BF",	"BF", "BFA", _n("Burkina Faso")},
  {100,	"BG",	"BG", "BGR", _n("Bulgaria")},
  { 48,	"BRN",	"BH", "BHR", _n("Bahrain")},
  {108,	"RU",	"BI", "BDI", _n("Burundi")},
  {204,	"BJ",	"BJ", "BEN", _n("Benin")},
  {652,	NULL,	"BL", "BLM", _n("Saint Barthelemy")},
  { 60,	NULL,	"BM", "BMU", _n("Bermuda")},
  { 96,	"BRU",	"BN", "BRN", _n("Brunei Darussalam")},
  { 68,	"BOL",	"BO", "BOL", _n("Bolivia")},
  { 76,	"BR",	"BR", "BRA", _n("Brazil")},
  { 44,	"BS",	"BS", "BHS", _n("Bahamas")},
  { 64,	"BHT",	"BT", "BTN", _n("Bhutan")},
  { 74,	NULL,	"BV", "BVT", _n("Bouvet Island")},
  { 72,	"RB",	"BW", "BWA", _n("Botswana")},
  {112,	"BY",	"BY", "BLR", _n("Belarus")},
  { 84,	"BZ",	"BZ", "BLZ", _n("Belize")},
  {124,	"CDN",	"CA", "CAN", _n("Canada")},
  {166,	NULL,	"CC", "CCK", _n("Cocos (Keeling) Islands")},
  {180,	"CGO",	"CD", "COD", _n("Congo, Democratic Republic of the")},
  {140,	"RCA",	"CF", "CAF", _n("Central African Republic")},
  {178,	NULL,	"CG", "COG", _n("Congo")},
  {756,	"CH",	"CH", "CHE", _n("Switzerland")},
  {384,	"CI",	"CI", "CIV", _n("Cote d'Ivoire")},
  {184,	NULL,	"CK", "COK", _n("Cook Islands")},
  {152,	"RCH",	"CL", "CHL", _n("Chile")},
  {120,	"CAM",	"CM", "CMR", _n("Cameroon")},
  {156,	"RC",	"CN", "CHN", _n("China")},
  {170,	"CO",	"CO", "COL", _n("Colombia")},
  {188,	"CR",	"CR", "CRI", _n("Costa Rica")},
  {192,	"C",	"CU", "CUB", _n("Cuba")},
  {132,	"CV",	"CV", "CPV", _n("Cape Verde")},
  {162,	NULL,	"CX", "CXR", _n("Christmas Island")},
  {196,	"CY",	"CY", "CYP", _n("Cyprus")},
  {203,	"CZ",	"CZ", "CZE", _n("Czech Republic")},
  {276,	"D",	"DE", "DEU", _n("Germany")},
  {262,	"DJI",	"DJ", "DJI", _n("Djibouti")},
  {208,	"DK",	"DK", "DNK", _n("Denmark")},
  {212,	"WD",	"DM", "DMA", _n("Dominica")},
  {214,	"DOM",	"DO", "DOM", _n("Dominican Republic")},
  { 12,	"DZ",	"DZ", "DZA", _n("Algeria")},
  {218,	"EC",	"EC", "ECU", _n("Ecuador")},
  {233,	"EST",	"EE", "EST", _n("Estonia")},
  {818,	"ET",	"EG", "EGY", _n("Egypt")},
  {732,	"WSA",	"EH", "ESH", _n("Western Sahara")},
  {232,	"ER",	"ER", "ERI", _n("Eritrea")},
  {724,	"E",	"ES", "ESP", _n("Spain")},
  {231,	"ETH",	"ET", "ETH", _n("Ethiopia")},
  {246,	"FIN",	"FI", "FIN", _n("Finland")},
  {242,	"FJI",	"FJ", "FJI", _n("Fiji")},
  {238,	NULL,	"FK", "FLK", _n("Falkland Islands (Malvinas)")},
  {583,	"FSM",	"FM", "FSM", _n("Micronesia, Federated States of")},
  {234,	"FO",	"FO", "FRO", _n("Faroe Islands")},
  {250,	"F",	"FR", "FRA", _n("France")},
  {266,	"G",	"GA", "GAB", _n("Gabon")},
  {826,	"GB",	"GB", "GBR", _n("United Kingdom")},
  {308,	"WG",	"GD", "GRD", _n("Grenada")},
  {268,	"GE",	"GE", "GEO", _n("Georgia")},
  {254,	NULL,	"GF", "GUF", _n("French Guiana")},
  {831,	NULL,	"GG", "GGY", _n("Guernsey")},
  {288,	"GH",	"GH", "GHA", _n("Ghana")},
  {292,	"GBZ",	"GI", "GIB", _n("Gibraltar")},
  {304,	"KN",	"GL", "GRL", _n("Greenland")},
  {270,	"WAG",	"GM", "GMB", _n("Gambia")},
  {324,	"RG",	"GN", "GIN", _n("Guinea")},
  {312,	NULL,	"GP", "GLP", _n("Guadeloupe")},
  {226,	"GQ",	"GQ", "GNQ", _n("Equatorial Guinea")},
  {300,	"GR",	"GR", "GRC", _n("Greece")},
  {239,	NULL,	"GS", "SGS", _n("South Georgia and the South Sandwich Islands")},
  {320,	"GCA",	"GT", "GTM", _n("Guatemala")},
  {316,	NULL,	"GU", "GUM", _n("Guam")},
  {624,	"GUB",	"GW", "GNB", _n("Guinea-Bissau")},
  {328,	"GUY",	"GY", "GUY", _n("Guyana")},
  {344,	"HK",	"HK", "HKG", _n("Hong Kong")},
  {334,	NULL,	"HM", "HMD", _n("Heard Island and McDonald Islands")},
  {340,	"HN",	"HN", "HND", _n("Honduras")},
  {191,	"HR",	"HR", "HRV", _n("Croatia")},
  {332,	"RH",	"HT", "HTI", _n("Haiti")},
  {348,	"H",	"HU", "HUN", _n("Hungary")},
  {360,	"RI",	"ID", "IDN", _n("Indonesia")},
  {372,	"IRL",	"IE", "IRL", _n("Ireland")},
  {376,	"IL",	"IL", "ISR", _n("Israel")},
  {833,	NULL,	"IM", "IMN", _n("Isle of Man")},
  {356,	"IND",	"IN", "IND", _n("India")},
  { 86,	NULL,	"IO", "IOT", _n("British Indian Ocean Territory")},
  {368,	"IRQ",	"IQ", "IRQ", _n("Iraq")},
  {364,	"IR",	"IR", "IRN", _n("Iran, Islamic Republic of")},
  {352,	"IS",	"IS", "ISL", _n("Iceland")},
  {380,	"I",	"IT", "ITA", _n("Italy")},
  {832,	NULL,	"JE", "JEY", _n("Jersey")},
  {388,	"JA",	"JM", "JAM", _n("Jamaica")},
  {400,	"JOR",	"JO", "JOR", _n("Jordan")},
  {392,	"J",	"JP", "JPN", _n("Japan")},
  {404,	"EAK",	"KE", "KEN", _n("Kenya")},
  {417,	"KS",	"KG", "KGZ", _n("Kyrgyzstan")},
  {116,	"K",	"KH", "KHM", _n("Cambodia")},
  {296,	"KIR",	"KI", "KIR", _n("Kiribati")},
  {174,	"COM",	"KM", "COM", _n("Comoros")},
  {659,	"KAN",	"KN", "KNA", _n("Saint Kitts and Nevis")},
  {408,	"KP",	"KP", "PRK", _n("Korea, Democratic People's Republic of")},
  {410,	"ROK",	"KR", "KOR", _n("Korea, Republic of")},
  {414,	"KWT",	"KW", "KWT", _n("Kuwait")},
  {136,	NULL,	"KY", "CYM", _n("Cayman Islands")},
  {398,	"KZ",	"KZ", "KAZ", _n("Kazakhstan")},
  {418,	"LAO",	"LA", "LAO", _n("Lao People's Democratic Republic")},
  {422,	"RL",	"LB", "LBN", _n("Lebanon")},
  {662,	"WL",	"LC", "LCA", _n("Saint Lucia")},
  {438,	"FL",	"LI", "LIE", _n("Liechtenstein")},
  {144,	"CL",	"LK", "LKA", _n("Sri Lanka")},
  {430,	"LB",	"LR", "LBR", _n("Liberia")},
  {426,	"LS",	"LS", "LSO", _n("Lesotho")},
  {440,	"LT",	"LT", "LTU", _n("Lithuania")},
  {442,	"L",	"LU", "LUX", _n("Luxembourg")},
  {428,	"LV",	"LV", "LVA", _n("Latvia")},
  {434,	"LAR",	"LY", "LBY", _n("Libyan Arab Jamahiriya")},
  {504,	"MA",	"MA", "MAR", _n("Morocco")},
  {492,	"MC",	"MC", "MCO", _n("Monaco")},
  {498,	"MD",	"MD", "MDA", _n("Moldova, Republic of")},
  {499,	"MNE",	"ME", "MNE", _n("Montenegro")},
  {663,	NULL,	"MF", "MAF", _n("Saint Martin (French part)")},
  {450,	"RM",	"MG", "MDG", _n("Madagascar")},
  {584,	"MH",	"MH", "MHL", _n("Marshall Islands")},
  {807,	"MK",	"MK", "MKD", _n("Macedonia, the former Yugoslav Republic of")},
  {466,	"RMM",	"ML", "MLI", _n("Mali")},
  {104,	"MYA",	"MM", "MMR", _n("Myanmar")},
  {496,	"MGL",	"MN", "MNG", _n("Mongolia")},
  {446,	NULL,	"MO", "MAC", _n("Macao")},
  {580,	NULL,	"MP", "MNP", _n("Northern Mariana Islands")},
  {474,	NULL,	"MQ", "MTQ", _n("Martinique")},
  {478,	"RIM",	"MR", "MRT", _n("Mauritania")},
  {500,	NULL,	"MS", "MSR", _n("Montserrat")},
  {470,	"M",	"MT", "MLT", _n("Malta")},
  {480,	"MS",	"MU", "MUS", _n("Mauritius")},
  {462,	"MV",	"MV", "MDV", _n("Maldives")},
  {454,	"MW",	"MW", "MWI", _n("Malawi")},
  {484,	"MEX",	"MX", "MEX", _n("Mexico")},
  {458,	"MAL",	"MY", "MYS", _n("Malaysia")},
  {508,	"MOC",	"MZ", "MOZ", _n("Mozambique")},
  {516,	"NAM",	"NA", "NAM", _n("Namibia")},
  {540,	"NCL",	"NC", "NCL", _n("New Caledonia")},
  {562,	"RN",	"NE", "NER", _n("Niger")},
  {574,	NULL,	"NF", "NFK", _n("Norfolk Island")},
  {566,	"NGR",	"NG", "NGA", _n("Nigeria")},
  {558,	"NIC",	"NI", "NIC", _n("Nicaragua")},
  {528,	"NL",	"NL", "NLD", _n("Netherlands")},
  {578,	"N",	"NO", "NOR", _n("Norway")},
  {524,	"NEP",	"NP", "NPL", _n("Nepal")},
  {520,	"NAU",	"NR", "NRU", _n("Nauru")},
  {570,	NULL,	"NU", "NIU", _n("Niue")},
  {554,	"NZ",	"NZ", "NZL", _n("New Zealand")},
  {512,	"OM",	"OM", "OMN", _n("Oman")},
  {591,	"PA",	"PA", "PAN", _n("Panama")},
  {604,	"PE",	"PE", "PER", _n("Peru")},
  {258,	NULL,	"PF", "PYF", _n("French Polynesia")},
  {598,	"PNG",	"PG", "PNG", _n("Papua New Guinea")},
  {608,	"RP",	"PH", "PHL", _n("Philippines")},
  {586,	"PK",	"PK", "PAK", _n("Pakistan")},
  {616,	"PL",	"PL", "POL", _n("Poland")},
  {666,	NULL,	"PM", "SPM", _n("Saint Pierre and Miquelon")},
  {612,	NULL,	"PN", "PCN", _n("Pitcairn")},
  {630,	"PRI",	"PR", "PRI", _n("Puerto Rico")},
  {275,	"AUT",	"PS", "PSE", _n("Palestinian Territory, Occupied")},
  {620,	"P",	"PT", "PRT", _n("Portugal")},
  {585,	"PAL",	"PW", "PLW", _n("Palau")},
  {600,	"PY",	"PY", "PRY", _n("Paraguay")},
  {634,	"Q",	"QA", "QAT", _n("Qatar")},
  {638,	NULL,	"RE", "REU", _n("Reunion")},
  {642,	"RO",	"RO", "ROU", _n("Romania")},
  {688,	"SRB",	"RS", "SRB", _n("Serbia")},
  {643,	"RUS",	"RU", "RUS", _n("Russian Federation")},
  {646,	"RWA",	"RW", "RWA", _n("Rwanda")},
  {682,	"KSA",	"SA", "SAU", _n("Saudi Arabia")},
  { 90,	"SOL",	"SB", "SLB", _n("Solomon Islands")},
  {690,	"SY",	"SC", "SYC", _n("Seychelles")},
  {736,	"SUD",	"SD", "SDN", _n("Sudan")},
  {752,	"S",	"SE", "SWE", _n("Sweden")},
  {702,	"SGP",	"SG", "SGP", _n("Singapore")},
  {654,	NULL,	"SH", "SHN", _n("Saint Helena")},
  {705,	"SLO",	"SI", "SVN", _n("Slovenia")},
  {744,	NULL,	"SJ", "SJM", _n("Svalbard and Jan Mayen")},
  {703,	"SK",	"SK", "SVK", _n("Slovakia")},
  {694,	"WAL",	"SL", "SLE", _n("Sierra Leone")},
  {674,	"RSM",	"SM", "SMR", _n("San Marino")},
  {686,	"SN",	"SN", "SEN", _n("Senegal")},
  {706,	"SO",	"SO", "SOM", _n("Somalia")},
  {740,	"SME",	"SR", "SUR", _n("Suriname")},
  {678,	"STP",	"ST", "STP", _n("Sao Tome and Principe")},
  {222,	"ES",	"SV", "SLV", _n("El Salvador")},
  {760,	"SYR",	"SY", "SYR", _n("Syrian Arab Republic")},
  {748,	"SD",	"SZ", "SWZ", _n("Swaziland")},
  {796,	NULL,	"TC", "TCA", _n("Turks and Caicos Islands")},
  {148,	"TD",	"TD", "TCD", _n("Chad")},
  {260,	"ARK",	"TF", "ATF", _n("French Southern Territories")},
  {768,	"RT",	"TG", "TGO", _n("Togo")},
  {764,	"T",	"TH", "THA", _n("Thailand")},
  {762,	"TJ",	"TJ", "TJK", _n("Tajikistan")},
  {772,	NULL,	"TK", "TKL", _n("Tokelau")},
  {626,	"TL",	"TL", "TLS", _n("Timor-Leste")},
  {795,	"TM",	"TM", "TKM", _n("Turkmenistan")},
  {788,	"TN",	"TN", "TUN", _n("Tunisia")},
  {776,	"TON",	"TO", "TON", _n("Tonga")},
  {792,	"TR",	"TR", "TUR", _n("Turkey")},
  {780,	"TT",	"TT", "TTO", _n("Trinidad and Tobago")},
  {798,	"TUV",	"TV", "TUV", _n("Tuvalu")},
  {158,	NULL,	"TW", "TWN", _n("Taiwan, Province of China")},
  {834,	"EAT",	"TZ", "TZA", _n("Tanzania, United Republic of")},
  {804,	"UA",	"UA", "UKR", _n("Ukraine")},
  {800,	"EAU",	"UG", "UGA", _n("Uganda")},
  {581,	NULL,	"UM", "UMI", _n("United States Minor Outlying Islands")},
  {840,	"USA",	"US", "USA", _n("United States")},
  {858,	"ROU",	"UY", "URY", _n("Uruguay")},
  {860,	"UZ",	"UZ", "UZB", _n("Uzbekistan")},
  {336,	"SCV",	"VA", "VAT", _n("Holy See (Vatican City State)")},
  {670,	"WV",	"VC", "VCT", _n("Saint Vincent and the Grenadines")},
  {862,	"YV",	"VE", "VEN", _n("Venezuela")},
  { 92,	NULL,	"VG", "VGB", _n("Virgin Islands, British")},
  {850,	NULL,	"VI", "VIR", _n("Virgin Islands, U.S.")},
  {704,	"VN",	"VN", "VNM", _n("Viet Nam")},
  {548,	"VAN",	"VU", "VUT", _n("Vanuatu")},
  {876,	NULL,	"WF", "WLF", _n("Wallis and Futuna")},
  {882,	"WS",	"WS", "WSM", _n("Samoa")},
  {887,	"YAR",	"YE", "YEM", _n("Yemen")},
  {175,	NULL,	"YT", "MYT", _n("Mayotte")},
  {710,	"ZA",	"ZA", "ZAF", _n("South Africa")},
  {894,	"Z",	"ZM", "ZMB", _n("Zambia")},
  {716,	"ZW",	"ZW", "ZWE", _n("Zimbabwe")},
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
        struct country_search *this_=priv_data;
	struct country *country=this_->country;

        attr->type=attr_type;
        switch (attr_type) {
        case attr_any:
                while (this_->attr_next != attr_none) {
                        if (country_attr_get(this_, this_->attr_next, attr))
                                return 1;
                }
                return 0;
        case attr_label:
		attr->u.str=gettext(country->name);
		this_->attr_next=attr_country_id;
		return 1;
	case attr_country_id:
		attr->u.num=country->id;
		this_->attr_next=country->car ? attr_country_car : attr_country_iso2;
		return 1;
        case attr_country_car:
		attr->u.str=country->car;
		this_->attr_next=attr_country_iso2;
		return attr->u.str ? 1 : 0;
        case attr_country_iso2:
		attr->u.str=country->iso2;
		this_->attr_next=attr_country_iso3;
		return 1;
        case attr_country_iso3:
		attr->u.str=country->iso3;
		this_->attr_next=attr_country_name;
		return 1;
        case attr_country_name:
		attr->u.str=gettext(country->name);
		this_->attr_next=attr_none;
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
	if (search->type != attr_country_id)
		ret->len=strlen(search->u.str);
	else
		ret->len=0;
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
match(struct country_search *this_, enum attr_type type, const char *name)
{
	int ret;
	if (!name)
		return 0;
	if (this_->search.type != type && this_->search.type != attr_country_all)
		return 0;
	if (this_->partial)
		ret=(strncasecmp(this_->search.u.str, name, this_->len) == 0);
	else
		ret=(strcasecmp(this_->search.u.str, name) == 0);
	return ret;
	
}


struct item *
country_search_get_item(struct country_search *this_)
{
	for (;;) {
		if (this_->count >= sizeof(country)/sizeof(struct country))
			return NULL;
		this_->country=&country[this_->count++];
		if ((this_->search.type == attr_country_id && this_->search.u.num == this_->country->id) ||
                    match(this_, attr_country_iso3, this_->country->iso3) ||
		    match(this_, attr_country_iso2, this_->country->iso2) ||
		    match(this_, attr_country_car, this_->country->car) ||
		    match(this_, attr_country_name, gettext(this_->country->name))) {
			this_->item.id_lo=this_->country->id;
			return &this_->item;
		}
	}
}

static struct attr country_default_attr;
static char iso2[3];

struct attr *
country_default(void)
{
	char *lang;
	if (country_default_attr.u.str)
		return &country_default_attr;
	lang=getenv("LANG");
	if (!lang || strlen(lang) < 5)
		return NULL;
	strncpy(iso2, lang+3, 2);
	country_default_attr.type=attr_country_iso2;
	country_default_attr.u.str=iso2;
	return &country_default_attr;
}

void
country_search_destroy(struct country_search *this_)
{
	g_free(this_);
}
