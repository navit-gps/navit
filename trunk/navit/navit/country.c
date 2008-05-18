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

static struct country country[]= {
  { 20,	"AND",	"AD", "AND", /* 020 */ _n("Andorra")},
  {784,	"UAE",	"AE", "ARE", /* 784 */ _n("United Arab Emirates")},
  {  4,	"AFG",	"AF", "AFG", /* 004 */ _n("Afghanistan")},
  { 28,	"AG",	"AG", "ATG", /* 028 */ _n("Antigua and Barbuda")},
  {660,	NULL,	"AI", "AIA", /* 660 */ _n("Anguilla")},
  {  8,	"AL",	"AL", "ALB", /* 008 */ _n("Albania")},
  { 51,	"ARM",	"AM", "ARM", /* 051 */ _n("Armenia")},
  {530,	"NA",	"AN", "ANT", /* 530 */ _n("Netherlands Antilles")},
  { 24,	"ANG",	"AO", "AGO", /* 024 */ _n("Angola")},
  { 10,	NULL,	"AQ", "ATA", /* 010 */ _n("Antarctica")},
  { 32,	"RA",	"AR", "ARG", /* 032 */ _n("Argentina")},
  { 16,	NULL,	"AS", "ASM", /* 016 */ _n("American Samoa")},
  { 40,	"A",	"AT", "AUT", /* 040 */ _n("Austria")},
  { 36,	"AUS",	"AU", "AUS", /* 036 */ _n("Australia")},
  {533,	"ARU",	"AW", "ABW", /* 533 */ _n("Aruba")},
  {248,	"AX",	"AX", "ALA", /* 248 */ _n("Aland Islands")},
  { 31,	"AZ",	"AZ", "AZE", /* 031 */ _n("Azerbaijan")},
  { 70,	"BiH",	"BA", "BIH", /* 070 */ _n("Bosnia and Herzegovina")},
  { 52,	"BDS",	"BB", "BRB", /* 052 */ _n("Barbados")},
  { 50,	"BD",	"BD", "BGD", /* 050 */ _n("Bangladesh")},
  { 56,	"B",	"BE", "BEL", /* 056 */ _n("Belgium")},
  {854,	"BF",	"BF", "BFA", /* 854 */ _n("Burkina Faso")},
  {100,	"BG",	"BG", "BGR", /* 100 */ _n("Bulgaria")},
  { 48,	"BRN",	"BH", "BHR", /* 048 */ _n("Bahrain")},
  {108,	"RU",	"BI", "BDI", /* 108 */ _n("Burundi")},
  {204,	"BJ",	"BJ", "BEN", /* 204 */ _n("Benin")},
  {652,	NULL,	"BL", "BLM", /* 652 */ _n("Saint Barthelemy")},
  { 60,	NULL,	"BM", "BMU", /* 060 */ _n("Bermuda")},
  { 96,	"BRU",	"BN", "BRN", /* 096 */ _n("Brunei Darussalam")},
  { 68,	"BOL",	"BO", "BOL", /* 068 */ _n("Bolivia")},
  { 76,	"BR",	"BR", "BRA", /* 076 */ _n("Brazil")},
  { 44,	"BS",	"BS", "BHS", /* 044 */ _n("Bahamas")},
  { 64,	"BHT",	"BT", "BTN", /* 064 */ _n("Bhutan")},
  { 74,	NULL,	"BV", "BVT", /* 074 */ _n("Bouvet Island")},
  { 72,	"RB",	"BW", "BWA", /* 072 */ _n("Botswana")},
  {112,	"BY",	"BY", "BLR", /* 112 */ _n("Belarus")},
  { 84,	"BZ",	"BZ", "BLZ", /* 084 */ _n("Belize")},
  {124,	"CDN",	"CA", "CAN", /* 124 */ _n("Canada")},
  {166,	NULL,	"CC", "CCK", /* 166 */ _n("Cocos (Keeling) Islands")},
  {180,	"CGO",	"CD", "COD", /* 180 */ _n("Congo, Democratic Republic of the")},
  {140,	"RCA",	"CF", "CAF", /* 140 */ _n("Central African Republic")},
  {178,	NULL,	"CG", "COG", /* 178 */ _n("Congo")},
  {756,	"CH",	"CH", "CHE", /* 756 */ _n("Switzerland")},
  {384,	"CI",	"CI", "CIV", /* 384 */ _n("Cote d'Ivoire")},
  {184,	NULL,	"CK", "COK", /* 184 */ _n("Cook Islands")},
  {152,	"RCH",	"CL", "CHL", /* 152 */ _n("Chile")},
  {120,	"CAM",	"CM", "CMR", /* 120 */ _n("Cameroon")},
  {156,	"RC",	"CN", "CHN", /* 156 */ _n("China")},
  {170,	"CO",	"CO", "COL", /* 170 */ _n("Colombia")},
  {188,	"CR",	"CR", "CRI", /* 188 */ _n("Costa Rica")},
  {192,	"C",	"CU", "CUB", /* 192 */ _n("Cuba")},
  {132,	"CV",	"CV", "CPV", /* 132 */ _n("Cape Verde")},
  {162,	NULL,	"CX", "CXR", /* 162 */ _n("Christmas Island")},
  {196,	"CY",	"CY", "CYP", /* 196 */ _n("Cyprus")},
  {203,	"CZ",	"CZ", "CZE", /* 203 */ _n("Czech Republic")},
  {276,	"D",	"DE", "DEU", /* 276 */ _n("Germany")},
  {262,	"DJI",	"DJ", "DJI", /* 262 */ _n("Djibouti")},
  {208,	"DK",	"DK", "DNK", /* 208 */ _n("Denmark")},
  {212,	"WD",	"DM", "DMA", /* 212 */ _n("Dominica")},
  {214,	"DOM",	"DO", "DOM", /* 214 */ _n("Dominican Republic")},
  { 12,	"DZ",	"DZ", "DZA", /* 012 */ _n("Algeria")},
  {218,	"EC",	"EC", "ECU", /* 218 */ _n("Ecuador")},
  {233,	"EST",	"EE", "EST", /* 233 */ _n("Estonia")},
  {818,	"ET",	"EG", "EGY", /* 818 */ _n("Egypt")},
  {732,	"WSA",	"EH", "ESH", /* 732 */ _n("Western Sahara")},
  {232,	"ER",	"ER", "ERI", /* 232 */ _n("Eritrea")},
  {724,	"E",	"ES", "ESP", /* 724 */ _n("Spain")},
  {231,	"ETH",	"ET", "ETH", /* 231 */ _n("Ethiopia")},
  {246,	"FIN",	"FI", "FIN", /* 246 */ _n("Finland")},
  {242,	"FJI",	"FJ", "FJI", /* 242 */ _n("Fiji")},
  {238,	NULL,	"FK", "FLK", /* 238 */ _n("Falkland Islands (Malvinas)")},
  {583,	"FSM",	"FM", "FSM", /* 583 */ _n("Micronesia, Federated States of")},
  {234,	"FO",	"FO", "FRO", /* 234 */ _n("Faroe Islands")},
  {250,	"F",	"FR", "FRA", /* 250 */ _n("France")},
  {266,	"G",	"GA", "GAB", /* 266 */ _n("Gabon")},
  {826,	"GB",	"GB", "GBR", /* 826 */ _n("United Kingdom")},
  {308,	"WG",	"GD", "GRD", /* 308 */ _n("Grenada")},
  {268,	"GE",	"GE", "GEO", /* 268 */ _n("Georgia")},
  {254,	NULL,	"GF", "GUF", /* 254 */ _n("French Guiana")},
  {831,	NULL,	"GG", "GGY", /* 831 */ _n("Guernsey")},
  {288,	"GH",	"GH", "GHA", /* 288 */ _n("Ghana")},
  {292,	"GBZ",	"GI", "GIB", /* 292 */ _n("Gibraltar")},
  {304,	"KN",	"GL", "GRL", /* 304 */ _n("Greenland")},
  {270,	"WAG",	"GM", "GMB", /* 270 */ _n("Gambia")},
  {324,	"RG",	"GN", "GIN", /* 324 */ _n("Guinea")},
  {312,	NULL,	"GP", "GLP", /* 312 */ _n("Guadeloupe")},
  {226,	"GQ",	"GQ", "GNQ", /* 226 */ _n("Equatorial Guinea")},
  {300,	"GR",	"GR", "GRC", /* 300 */ _n("Greece")},
  {239,	NULL,	"GS", "SGS", /* 239 */ _n("South Georgia and the South Sandwich Islands")},
  {320,	"GCA",	"GT", "GTM", /* 320 */ _n("Guatemala")},
  {316,	NULL,	"GU", "GUM", /* 316 */ _n("Guam")},
  {624,	"GUB",	"GW", "GNB", /* 624 */ _n("Guinea-Bissau")},
  {328,	"GUY",	"GY", "GUY", /* 328 */ _n("Guyana")},
  {344,	"HK",	"HK", "HKG", /* 344 */ _n("Hong Kong")},
  {334,	NULL,	"HM", "HMD", /* 334 */ _n("Heard Island and McDonald Islands")},
  {340,	"HN",	"HN", "HND", /* 340 */ _n("Honduras")},
  {191,	"HR",	"HR", "HRV", /* 191 */ _n("Croatia")},
  {332,	"RH",	"HT", "HTI", /* 332 */ _n("Haiti")},
  {348,	"H",	"HU", "HUN", /* 348 */ _n("Hungary")},
  {360,	"RI",	"ID", "IDN", /* 360 */ _n("Indonesia")},
  {372,	"IRL",	"IE", "IRL", /* 372 */ _n("Ireland")},
  {376,	"IL",	"IL", "ISR", /* 376 */ _n("Israel")},
  {833,	NULL,	"IM", "IMN", /* 833 */ _n("Isle of Man")},
  {356,	"IND",	"IN", "IND", /* 356 */ _n("India")},
  { 86,	NULL,	"IO", "IOT", /* 086 */ _n("British Indian Ocean Territory")},
  {368,	"IRQ",	"IQ", "IRQ", /* 368 */ _n("Iraq")},
  {364,	"IR",	"IR", "IRN", /* 364 */ _n("Iran, Islamic Republic of")},
  {352,	"IS",	"IS", "ISL", /* 352 */ _n("Iceland")},
  {380,	"I",	"IT", "ITA", /* 380 */ _n("Italy")},
  {832,	NULL,	"JE", "JEY", /* 832 */ _n("Jersey")},
  {388,	"JA",	"JM", "JAM", /* 388 */ _n("Jamaica")},
  {400,	"JOR",	"JO", "JOR", /* 400 */ _n("Jordan")},
  {392,	"J",	"JP", "JPN", /* 392 */ _n("Japan")},
  {404,	"EAK",	"KE", "KEN", /* 404 */ _n("Kenya")},
  {417,	"KS",	"KG", "KGZ", /* 417 */ _n("Kyrgyzstan")},
  {116,	"K",	"KH", "KHM", /* 116 */ _n("Cambodia")},
  {296,	"KIR",	"KI", "KIR", /* 296 */ _n("Kiribati")},
  {174,	"COM",	"KM", "COM", /* 174 */ _n("Comoros")},
  {659,	"KAN",	"KN", "KNA", /* 659 */ _n("Saint Kitts and Nevis")},
  {408,	"KP",	"KP", "PRK", /* 408 */ _n("Korea, Democratic People's Republic of")},
  {410,	"ROK",	"KR", "KOR", /* 410 */ _n("Korea, Republic of")},
  {414,	"KWT",	"KW", "KWT", /* 414 */ _n("Kuwait")},
  {136,	NULL,	"KY", "CYM", /* 136 */ _n("Cayman Islands")},
  {398,	"KZ",	"KZ", "KAZ", /* 398 */ _n("Kazakhstan")},
  {418,	"LAO",	"LA", "LAO", /* 418 */ _n("Lao People's Democratic Republic")},
  {422,	"RL",	"LB", "LBN", /* 422 */ _n("Lebanon")},
  {662,	"WL",	"LC", "LCA", /* 662 */ _n("Saint Lucia")},
  {438,	"FL",	"LI", "LIE", /* 438 */ _n("Liechtenstein")},
  {144,	"CL",	"LK", "LKA", /* 144 */ _n("Sri Lanka")},
  {430,	"LB",	"LR", "LBR", /* 430 */ _n("Liberia")},
  {426,	"LS",	"LS", "LSO", /* 426 */ _n("Lesotho")},
  {440,	"LT",	"LT", "LTU", /* 440 */ _n("Lithuania")},
  {442,	"L",	"LU", "LUX", /* 442 */ _n("Luxembourg")},
  {428,	"LV",	"LV", "LVA", /* 428 */ _n("Latvia")},
  {434,	"LAR",	"LY", "LBY", /* 434 */ _n("Libyan Arab Jamahiriya")},
  {504,	"MA",	"MA", "MAR", /* 504 */ _n("Morocco")},
  {492,	"MC",	"MC", "MCO", /* 492 */ _n("Monaco")},
  {498,	"MD",	"MD", "MDA", /* 498 */ _n("Moldova, Republic of")},
  {499,	"MNE",	"ME", "MNE", /* 499 */ _n("Montenegro")},
  {663,	NULL,	"MF", "MAF", /* 663 */ _n("Saint Martin (French part)")},
  {450,	"RM",	"MG", "MDG", /* 450 */ _n("Madagascar")},
  {584,	"MH",	"MH", "MHL", /* 584 */ _n("Marshall Islands")},
  {807,	"MK",	"MK", "MKD", /* 807 */ _n("Macedonia, the former Yugoslav Republic of")},
  {466,	"RMM",	"ML", "MLI", /* 466 */ _n("Mali")},
  {104,	"MYA",	"MM", "MMR", /* 104 */ _n("Myanmar")},
  {496,	"MGL",	"MN", "MNG", /* 496 */ _n("Mongolia")},
  {446,	NULL,	"MO", "MAC", /* 446 */ _n("Macao")},
  {580,	NULL,	"MP", "MNP", /* 580 */ _n("Northern Mariana Islands")},
  {474,	NULL,	"MQ", "MTQ", /* 474 */ _n("Martinique")},
  {478,	"RIM",	"MR", "MRT", /* 478 */ _n("Mauritania")},
  {500,	NULL,	"MS", "MSR", /* 500 */ _n("Montserrat")},
  {470,	"M",	"MT", "MLT", /* 470 */ _n("Malta")},
  {480,	"MS",	"MU", "MUS", /* 480 */ _n("Mauritius")},
  {462,	"MV",	"MV", "MDV", /* 462 */ _n("Maldives")},
  {454,	"MW",	"MW", "MWI", /* 454 */ _n("Malawi")},
  {484,	"MEX",	"MX", "MEX", /* 484 */ _n("Mexico")},
  {458,	"MAL",	"MY", "MYS", /* 458 */ _n("Malaysia")},
  {508,	"MOC",	"MZ", "MOZ", /* 508 */ _n("Mozambique")},
  {516,	"NAM",	"NA", "NAM", /* 516 */ _n("Namibia")},
  {540,	"NCL",	"NC", "NCL", /* 540 */ _n("New Caledonia")},
  {562,	"RN",	"NE", "NER", /* 562 */ _n("Niger")},
  {574,	NULL,	"NF", "NFK", /* 574 */ _n("Norfolk Island")},
  {566,	"NGR",	"NG", "NGA", /* 566 */ _n("Nigeria")},
  {558,	"NIC",	"NI", "NIC", /* 558 */ _n("Nicaragua")},
  {528,	"NL",	"NL", "NLD", /* 528 */ _n("Netherlands")},
  {578,	"N",	"NO", "NOR", /* 578 */ _n("Norway")},
  {524,	"NEP",	"NP", "NPL", /* 524 */ _n("Nepal")},
  {520,	"NAU",	"NR", "NRU", /* 520 */ _n("Nauru")},
  {570,	NULL,	"NU", "NIU", /* 570 */ _n("Niue")},
  {554,	"NZ",	"NZ", "NZL", /* 554 */ _n("New Zealand")},
  {512,	"OM",	"OM", "OMN", /* 512 */ _n("Oman")},
  {591,	"PA",	"PA", "PAN", /* 591 */ _n("Panama")},
  {604,	"PE",	"PE", "PER", /* 604 */ _n("Peru")},
  {258,	NULL,	"PF", "PYF", /* 258 */ _n("French Polynesia")},
  {598,	"PNG",	"PG", "PNG", /* 598 */ _n("Papua New Guinea")},
  {608,	"RP",	"PH", "PHL", /* 608 */ _n("Philippines")},
  {586,	"PK",	"PK", "PAK", /* 586 */ _n("Pakistan")},
  {616,	"PL",	"PL", "POL", /* 616 */ _n("Poland")},
  {666,	NULL,	"PM", "SPM", /* 666 */ _n("Saint Pierre and Miquelon")},
  {612,	NULL,	"PN", "PCN", /* 612 */ _n("Pitcairn")},
  {630,	"PRI",	"PR", "PRI", /* 630 */ _n("Puerto Rico")},
  {275,	"AUT",	"PS", "PSE", /* 275 */ _n("Palestinian Territory, Occupied")},
  {620,	"P",	"PT", "PRT", /* 620 */ _n("Portugal")},
  {585,	"PAL",	"PW", "PLW", /* 585 */ _n("Palau")},
  {600,	"PY",	"PY", "PRY", /* 600 */ _n("Paraguay")},
  {634,	"Q",	"QA", "QAT", /* 634 */ _n("Qatar")},
  {638,	NULL,	"RE", "REU", /* 638 */ _n("Reunion")},
  {642,	"RO",	"RO", "ROU", /* 642 */ _n("Romania")},
  {688,	"SRB",	"RS", "SRB", /* 688 */ _n("Serbia")},
  {643,	"RUS",	"RU", "RUS", /* 643 */ _n("Russian Federation")},
  {646,	"RWA",	"RW", "RWA", /* 646 */ _n("Rwanda")},
  {682,	"KSA",	"SA", "SAU", /* 682 */ _n("Saudi Arabia")},
  { 90,	"SOL",	"SB", "SLB", /* 090 */ _n("Solomon Islands")},
  {690,	"SY",	"SC", "SYC", /* 690 */ _n("Seychelles")},
  {736,	"SUD",	"SD", "SDN", /* 736 */ _n("Sudan")},
  {752,	"S",	"SE", "SWE", /* 752 */ _n("Sweden")},
  {702,	"SGP",	"SG", "SGP", /* 702 */ _n("Singapore")},
  {654,	NULL,	"SH", "SHN", /* 654 */ _n("Saint Helena")},
  {705,	"SLO",	"SI", "SVN", /* 705 */ _n("Slovenia")},
  {744,	NULL,	"SJ", "SJM", /* 744 */ _n("Svalbard and Jan Mayen")},
  {703,	"SK",	"SK", "SVK", /* 703 */ _n("Slovakia")},
  {694,	"WAL",	"SL", "SLE", /* 694 */ _n("Sierra Leone")},
  {674,	"RSM",	"SM", "SMR", /* 674 */ _n("San Marino")},
  {686,	"SN",	"SN", "SEN", /* 686 */ _n("Senegal")},
  {706,	"SO",	"SO", "SOM", /* 706 */ _n("Somalia")},
  {740,	"SME",	"SR", "SUR", /* 740 */ _n("Suriname")},
  {678,	"STP",	"ST", "STP", /* 678 */ _n("Sao Tome and Principe")},
  {222,	"ES",	"SV", "SLV", /* 222 */ _n("El Salvador")},
  {760,	"SYR",	"SY", "SYR", /* 760 */ _n("Syrian Arab Republic")},
  {748,	"SD",	"SZ", "SWZ", /* 748 */ _n("Swaziland")},
  {796,	NULL,	"TC", "TCA", /* 796 */ _n("Turks and Caicos Islands")},
  {148,	"TD",	"TD", "TCD", /* 148 */ _n("Chad")},
  {260,	"ARK",	"TF", "ATF", /* 260 */ _n("French Southern Territories")},
  {768,	"RT",	"TG", "TGO", /* 768 */ _n("Togo")},
  {764,	"T",	"TH", "THA", /* 764 */ _n("Thailand")},
  {762,	"TJ",	"TJ", "TJK", /* 762 */ _n("Tajikistan")},
  {772,	NULL,	"TK", "TKL", /* 772 */ _n("Tokelau")},
  {626,	"TL",	"TL", "TLS", /* 626 */ _n("Timor-Leste")},
  {795,	"TM",	"TM", "TKM", /* 795 */ _n("Turkmenistan")},
  {788,	"TN",	"TN", "TUN", /* 788 */ _n("Tunisia")},
  {776,	"TON",	"TO", "TON", /* 776 */ _n("Tonga")},
  {792,	"TR",	"TR", "TUR", /* 792 */ _n("Turkey")},
  {780,	"TT",	"TT", "TTO", /* 780 */ _n("Trinidad and Tobago")},
  {798,	"TUV",	"TV", "TUV", /* 798 */ _n("Tuvalu")},
  {158,	NULL,	"TW", "TWN", /* 158 */ _n("Taiwan, Province of China")},
  {834,	"EAT",	"TZ", "TZA", /* 834 */ _n("Tanzania, United Republic of")},
  {804,	"UA",	"UA", "UKR", /* 804 */ _n("Ukraine")},
  {800,	"EAU",	"UG", "UGA", /* 800 */ _n("Uganda")},
  {581,	NULL,	"UM", "UMI", /* 581 */ _n("United States Minor Outlying Islands")},
  {840,	"USA",	"US", "USA", /* 840 */ _n("United States")},
  {858,	"ROU",	"UY", "URY", /* 858 */ _n("Uruguay")},
  {860,	"UZ",	"UZ", "UZB", /* 860 */ _n("Uzbekistan")},
  {336,	"SCV",	"VA", "VAT", /* 336 */ _n("Holy See (Vatican City State)")},
  {670,	"WV",	"VC", "VCT", /* 670 */ _n("Saint Vincent and the Grenadines")},
  {862,	"YV",	"VE", "VEN", /* 862 */ _n("Venezuela")},
  { 92,	NULL,	"VG", "VGB", /* 092 */ _n("Virgin Islands, British")},
  {850,	NULL,	"VI", "VIR", /* 850 */ _n("Virgin Islands, U.S.")},
  {704,	"VN",	"VN", "VNM", /* 704 */ _n("Viet Nam")},
  {548,	"VAN",	"VU", "VUT", /* 548 */ _n("Vanuatu")},
  {876,	NULL,	"WF", "WLF", /* 876 */ _n("Wallis and Futuna")},
  {882,	"WS",	"WS", "WSM", /* 882 */ _n("Samoa")},
  {887,	"YAR",	"YE", "YEM", /* 887 */ _n("Yemen")},
  {175,	NULL,	"YT", "MYT", /* 175 */ _n("Mayotte")},
  {710,	"ZA",	"ZA", "ZAF", /* 710 */ _n("South Africa") },
  {894,	"Z",	"ZM", "ZMB", /* 894 */ _n("Zambia")},
  {716,	"ZW",	"ZW", "ZWE", /* 716 */ _n("Zimbabwe")},
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
