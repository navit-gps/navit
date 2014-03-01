/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include "navit_lfs.h"
#ifdef _MSC_VER
#define strcasecmp _stricmp
#define snprintf _snprintf
#else
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "maptool.h"
#include "debug.h"
#include "linguistics.h"
#include "country.h"
#include "file.h"
#include "profile.h"
#include "types.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#define M_PI_4     0.785398163397448309616
#endif

extern int doway2poi;

static int in_way, in_node, in_relation;
osmid nodeid,wayid;

static GHashTable *attr_hash,*country_table_hash,*attr_hash;

static char *attr_present;
static int attr_present_count;

static struct item_bin item;


int maxspeed_attr_value;

char debug_attr_buffer[BUFFER_SIZE];

int flags[4];
int flagsa[4];

int flags_attr_value;

struct attr_bin osmid_attr;
long long osmid_attr_value;

char is_in_buffer[BUFFER_SIZE];

char attr_strings_buffer[BUFFER_SIZE*16];
int attr_strings_buffer_len;


struct coord coord_buffer[65536];

struct attr_mapping {
	enum item_type type;
	int attr_present_idx_count;
	int attr_present_idx[0];
};

static void nodes_ref_item_bin(struct item_bin *ib);


static struct attr_mapping **attr_mapping_node;
static int attr_mapping_node_count;
static struct attr_mapping **attr_mapping_way;
static int attr_mapping_way_count;
static struct attr_mapping **attr_mapping_way2poi;
static int attr_mapping_way2poi_count;
static struct attr_mapping **attr_mapping_rel2poly_place;
static int attr_mapping_rel2poly_place_count;

static int attr_longest_match(struct attr_mapping **mapping, int mapping_count, enum item_type *types, int types_count);
static void attr_longest_match_clear(void);


enum attr_strings {
	attr_string_phone,
	attr_string_fax,
	attr_string_email,
	attr_string_url,
	attr_string_district_name,
	attr_string_street_name,
	attr_string_street_name_systematic,
	attr_string_house_number,
	attr_string_label,
	attr_string_postal,
	attr_string_population,
	attr_string_county_name,
	attr_string_last,
};

char *attr_strings[attr_string_last];

char *osm_types[]={"unknown","node","way","relation"};

#define IS_REF(c) ((c).x >= (1 << 30))
#define REF(c) ((unsigned)(c).y)
#define SET_REF(c,ref) do { (c).x = 1 << 30; (c).y = ref ; } while(0)

/* Table of country codes with possible is_in spellings. 
 *  Note: If you update this list, check also country array in country.c 
 */
struct country_table {
	int countryid;
	char *names;
	char *admin_levels;
	FILE *file;
	int size;
	struct rect r;
	int nparts;
} country_table[] = {
	{ 4,"Afghanistan"},
	{ 8,"Albania"},
	{ 10,"Antarctica"},
	{ 12,"Algeria"},
	{ 16,"American Samoa"},
	{ 20,"Andorra"},
	{ 24,"Angola"},
	{ 28,"Antigua and Barbuda"},
	{ 31,"Azerbaijan"},
	{ 32,"Argentina,República Argentina,AR "},
	{ 36,"Australia,AUS"},
	{ 40,"Austria,Österreich,AUT"},
	{ 44,"Bahamas"},
	{ 48,"Bahrain"},
	{ 50,"Bangladesh"},
	{ 51,"Armenia"},
	{ 52,"Barbados"},
	{ 56,"Belgium,Belgique,Belgie,België,Belgien"},
	{ 60,"Bermuda"},
	{ 64,"Bhutan"},
	{ 68,"Bolivia, Plurinational State of"},
	{ 70,"Bosnia and Herzegovina,Bosna i Hercegovina,Босна и Херцеговина"},
	{ 72,"Botswana"},
	{ 74,"Bouvet Island"},
	{ 76,"Brazil"},
	{ 84,"Belize"},
	{ 86,"British Indian Ocean Territory"},
	{ 90,"Solomon Islands"},
	{ 92,"Virgin Islands, British"},
	{ 96,"Brunei Darussalam"},
	{ 100,"Bulgaria,България"},
	{ 104,"Myanmar"},
	{ 108,"Burundi"},
	{ 112,"Belarus"},
	{ 116,"Cambodia"},
	{ 120,"Cameroon"},
	{ 124,"Canada"},
	{ 132,"Cape Verde"},
	{ 136,"Cayman Islands"},
	{ 140,"Central African Republic"},
	{ 144,"Sri Lanka"},
	{ 148,"Chad"},
	{ 152,"Chile"},
	{ 156,"China"},
	{ 158,"Taiwan, Province of China"},
	{ 162,"Christmas Island"},
	{ 166,"Cocos (Keeling) Islands"},
	{ 170,"Colombia"},
	{ 174,"Comoros"},
	{ 175,"Mayotte"},
	{ 178,"Congo"},
	{ 180,"Congo, the Democratic Republic of the"},
	{ 184,"Cook Islands"},
	{ 188,"Costa Rica"},
	{ 191,"Croatia,Republika Hrvatska,HR"},
	{ 192,"Cuba"},
	{ 196,"Cyprus"},
	{ 203,"Czech Republic,Česká republika,CZ"},
	{ 204,"Benin"},
 	{ 208,"Denmark,Danmark,DK"},
	{ 212,"Dominica"},
	{ 214,"Dominican Republic"},
	{ 218,"Ecuador"},
	{ 222,"El Salvador"},
	{ 226,"Equatorial Guinea"},
	{ 231,"Ethiopia"},
	{ 232,"Eritrea"},
	{ 233,"Estonia"},
	{ 234,"Faroe Islands,Føroyar"},
	{ 238,"Falkland Islands (Malvinas)"},
	{ 239,"South Georgia and the South Sandwich Islands"},
	{ 242,"Fiji"},
 	{ 246,"Finland,Suomi"},
	{ 248,"Åland Islands"},
 	{ 250,"France,République française,FR","3s5c7M"},
	{ 254,"French Guiana"},
	{ 258,"French Polynesia"},
	{ 260,"French Southern Territories"},
	{ 262,"Djibouti"},
	{ 266,"Gabon"},
	{ 268,"Georgia"},
	{ 270,"Gambia"},
	{ 275,"Palestinian Territory, Occupied"},
	{ 276,"Germany,Deutschland,Bundesrepublik Deutschland","345c7M"},
	{ 288,"Ghana"},
	{ 292,"Gibraltar"},
	{ 296,"Kiribati"},
	{ 300,"Greece"},
	{ 304,"Greenland"},
	{ 308,"Grenada"},
	{ 312,"Guadeloupe"},
	{ 316,"Guam"},
	{ 320,"Guatemala"},
	{ 324,"Guinea"},
	{ 328,"Guyana"},
	{ 332,"Haiti"},
	{ 334,"Heard Island and McDonald Islands"},
	{ 336,"Holy See (Vatican City State)"},
	{ 340,"Honduras"},
	{ 344,"Hong Kong"},
	{ 348,"Hungary,Magyarország"},
	{ 352,"Iceland"},
	{ 356,"India"},
	{ 360,"Indonesia"},
	{ 364,"Iran, Islamic Republic of"},
	{ 368,"Iraq"},
	{ 372,"Ireland"},
	{ 376,"Israel"},
 	{ 380,"Italy,Italia"},
	{ 384,"Côte d'Ivoire"},
	{ 388,"Jamaica"},
	{ 392,"Japan"},
	{ 398,"Kazakhstan"},
	{ 400,"Jordan"},
	{ 404,"Kenya"},
	{ 408,"Korea, Democratic People's Republic of"},
	{ 410,"Korea, Republic of"},
	{ 414,"Kuwait"},
	{ 417,"Kyrgyzstan"},
	{ 418,"Lao People's Democratic Republic"},
	{ 422,"Lebanon"},
	{ 426,"Lesotho"},
	{ 428,"Latvia"},
	{ 430,"Liberia"},
	{ 434,"Libyan Arab Jamahiriya"},
	{ 438,"Liechtenstein"},
 	{ 440,"Lithuania,Lietuva"},
	{ 442,"Luxembourg"},
	{ 446,"Macao"},
	{ 450,"Madagascar"},
	{ 454,"Malawi"},
	{ 458,"Malaysia"},
	{ 462,"Maldives"},
	{ 466,"Mali"},
	{ 470,"Malta"},
	{ 474,"Martinique"},
	{ 478,"Mauritania"},
	{ 480,"Mauritius"},
	{ 484,"Mexico"},
	{ 492,"Monaco"},
	{ 496,"Mongolia"},
	{ 498,"Moldova, Republic of"},
	{ 499,"Montenegro,Црна Гора,Crna Gora"},
	{ 500,"Montserrat"},
	{ 504,"Morocco"},
	{ 508,"Mozambique"},
	{ 512,"Oman"},
	{ 516,"Namibia"},
	{ 520,"Nauru"},
	{ 524,"Nepal"},
	{ 528,"Nederland,The Netherlands,Niederlande,NL,Netherlands"},
	{ 530,"Netherlands Antilles"},
	{ 531,"Curacao"},
	{ 533,"Aruba"},
	{ 534,"Sint Maarten (Dutch part)"},
	{ 535,"Bonaire, Sint Eustatius and Saba"},
	{ 540,"New Caledonia"},
	{ 548,"Vanuatu"},
	{ 554,"New Zealand"},
	{ 558,"Nicaragua"},
	{ 562,"Niger"},
	{ 566,"Nigeria"},
	{ 570,"Niue"},
	{ 574,"Norfolk Island"},
	{ 578,"Norway,Norge,Noreg,NO"},
	{ 580,"Northern Mariana Islands"},
	{ 581,"United States Minor Outlying Islands"},
	{ 583,"Micronesia, Federated States of"},
	{ 584,"Marshall Islands"},
	{ 585,"Palau"},
	{ 586,"Pakistan"},
	{ 591,"Panama"},
	{ 598,"Papua New Guinea"},
	{ 600,"Paraguay"},
	{ 604,"Peru"},
	{ 608,"Philippines"},
	{ 612,"Pitcairn"},
	{ 616,"Poland,Polska,PL"},
	{ 620,"Portugal"},
	{ 624,"Guinea-Bissau"},
	{ 626,"Timor-Leste"},
	{ 630,"Puerto Rico"},
	{ 634,"Qatar"},
	{ 638,"Réunion"},
	{ 642,"România,Romania,RO"},
	{ 643,"Россия,Российская Федерация,Russia,Russian Federation","3s5c7m"},
	{ 646,"Rwanda"},
	{ 652,"Saint Barthélemy"},
	{ 654,"Saint Helena, Ascension and Tristan da Cunha"},
	{ 659,"Saint Kitts and Nevis"},
	{ 660,"Anguilla"},
	{ 662,"Saint Lucia"},
	{ 663,"Saint Martin (French part)"},
	{ 666,"Saint Pierre and Miquelon"},
	{ 670,"Saint Vincent and the Grenadines"},
	{ 674,"San Marino"},
	{ 678,"Sao Tome and Principe"},
	{ 682,"Saudi Arabia"},
	{ 686,"Senegal"},
	{ 688,"Srbija,Србија,Serbia"},
	{ 690,"Seychelles"},
	{ 694,"Sierra Leone"},
	{ 702,"Singapore"},
	{ 703,"Slovakia,Slovensko,SK"},
	{ 704,"Viet Nam"},
	{ 705,"Slovenia,Republika Slovenija,SI"},
	{ 706,"Somalia"},
	{ 710,"South Africa"},
	{ 716,"Zimbabwe"},
 	{ 724,"Spain,Espana,España,Reino de Espana,Reino de España","345scm"},
 	{ 728,"South Sudan"},
	{ 732,"Western Sahara"},
	{ 736,"Sudan"},
	{ 740,"Suriname"},
	{ 744,"Svalbard and Jan Mayen"},
	{ 748,"Swaziland"},
	{ 752,"Sweden,Sverige,Konungariket Sverige,SE"},
	{ 756,"Switzerland,Schweiz","3s5c7M"},
	{ 760,"Syrian Arab Republic"},
	{ 762,"Tajikistan"},
	{ 764,"Thailand"},
	{ 768,"Togo"},
	{ 772,"Tokelau"},
	{ 776,"Tonga"},
	{ 780,"Trinidad and Tobago"},
	{ 784,"United Arab Emirates"},
	{ 788,"Tunisia"},
	{ 792,"Turkey"},
	{ 795,"Turkmenistan"},
	{ 796,"Turks and Caicos Islands"},
	{ 798,"Tuvalu"},
	{ 800,"Uganda"},
	{ 804,"Ukraine","3s5c78"},
	{ 807,"Macedonia,Македонија"},
	{ 818,"Egypt"},
	{ 826,"United Kingdom,UK","3s5c7m"},
	{ 831,"Guernsey"},
	{ 832,"Jersey"},
	{ 833,"Isle of Man"},
	{ 834,"Tanzania, United Republic of"},
	{ 840,"USA","3s5c7m"},
	{ 850,"Virgin Islands, U.S."},
	{ 854,"Burkina Faso"},
	{ 858,"Uruguay"},
	{ 860,"Uzbekistan"},
	{ 862,"Venezuela, Bolivarian Republic of"},
	{ 876,"Wallis and Futuna"},
	{ 882,"Samoa"},
	{ 887,"Yemen"},
	{ 894,"Zambia"},
	{ 999,"Unknown"},
};

// first char - item type
//   =w - ways
//   =? - used both for nodes and ways
//   otherwise - nodes

static char *attrmap={
	"n	*=*			point_unkn\n"
//	"n	Annehmlichkeit=Hochsitz	poi_hunting_stand\n"
	"?	addr:housenumber=*	house_number\n"
	"?	aeroway=aerodrome	poi_airport\n"
	"?	aeroway=airport		poi_airport\n"
	"?	aeroway=helipad		poi_heliport\n"
	"?	aeroway=terminal	poi_airport\n"
	"?	amenity=atm		poi_atm\n"
	"?	amenity=bank		poi_bank\n"
	"?	amenity=bar		poi_bar\n"
	"n	amenity=bench		poi_bench\n"
	"?	amenity=bicycle_rental	poi_bicycle_rental\n"
	"?	amenity=bicycle_parking	poi_bicycle_parking\n"
	"?	amenity=biergarten	poi_biergarten\n"
	"?	amenity=bus_station	poi_bus_station\n"
	"?	amenity=cafe		poi_cafe\n"
	"?	amenity=car_sharing	poi_car_sharing\n"
	"?	amenity=car_wash	poi_car_wash\n"
	"?	amenity=cinema		poi_cinema\n"
	"?	amenity=college		poi_school_college\n"
	"?	amenity=courthouse	poi_justice\n"
	"?	amenity=drinking_water	poi_potable_water\n"
	"?	amenity=fast_food	poi_fastfood\n"
	"?	amenity=fire_station	poi_firebrigade\n"
	"?	amenity=fountain	poi_fountain\n"
	"?	amenity=fuel		poi_fuel\n"
	"?	amenity=grave_yard	poi_cemetery\n"
	"?	amenity=hospital	poi_hospital\n"
	"?	amenity=hunting_stand	poi_hunting_stand\n"
	"?	amenity=kindergarten	poi_kindergarten\n"
	"?	amenity=library		poi_library\n"
	"?	amenity=nightclub	poi_nightclub\n"
	"?	amenity=park_bench	poi_bench\n"
	"?	amenity=parking		poi_car_parking\n"
	"?	amenity=pharmacy	poi_pharmacy\n"
	"?	amenity=place_of_worship,religion=christian	poi_church\n"
	"?	amenity=place_of_worship			poi_worship\n"
	"?	amenity=police		poi_police\n"
	"?	amenity=post_box	poi_post_box\n"
	"?	amenity=post_office	poi_post_office\n"
	"?	amenity=prison		poi_prison\n"
	"?	amenity=pub		poi_pub\n"
	"?	amenity=public_building	poi_public_office\n"
	"?	amenity=recycling	poi_recycling\n"
	"?	amenity=restaurant,cuisine=fine_dining		poi_dining\n"
	"?	amenity=restaurant				poi_restaurant\n"
	"?	amenity=school		poi_school\n"
	"?	amenity=shelter		poi_shelter\n"
	"?	amenity=taxi		poi_taxi\n"
	"?	amenity=tec_common	tec_common\n"
	"?	amenity=telephone	poi_telephone\n"
	"?	amenity=theatre		poi_theater\n"
	"?	amenity=toilets		poi_restroom\n"
	"?	amenity=townhall	poi_townhall\n"
	"?	amenity=university	poi_school_university\n"
	"?	amenity=vending_machine	poi_vending_machine\n"
	"n	barrier=bollard		barrier_bollard\n"
	"n	barrier=cycle_barrier	barrier_cycle\n"
	"n	barrier=lift_gate	barrier_lift_gate\n"
	"?	car=car_rental		poi_car_rent\n"
	"?	highway=bus_station	poi_bus_station\n"
	"?	highway=bus_stop	poi_bus_stop\n"
	"n	highway=mini_roundabout	mini_roundabout\n"
	"n	highway=motorway_junction	highway_exit\n"
	"n	highway=stop		traffic_sign_stop\n"
	"n	highway=toll_booth	poi_toll_booth\n"
	"n	highway=traffic_signals	traffic_signals\n"
	"n	highway=turning_circle	turning_circle\n"
	"?	historic=boundary_stone	poi_boundary_stone\n"
	"?	historic=castle		poi_castle\n"
	"?	historic=memorial	poi_memorial\n"
	"?	historic=monument	poi_monument\n"
	"?	historic=ruins		poi_ruins\n"
//	"?	historic=*		poi_ruins\n"
	"?	landuse=cemetery	poi_cemetery\n"
	"?	leisure=fishing		poi_fish\n"
	"?	leisure=golf_course	poi_golf\n"
	"?	leisure=marina		poi_marine\n"
	"?	leisure=playground	poi_playground\n"
	"?	leisure=slipway		poi_boat_ramp\n"
	"?	leisure=sports_centre	poi_sport\n"
	"?	leisure=stadium		poi_stadium\n"
	"?	man_made=tower		poi_tower\n"
	"?	military=airfield	poi_military\n"
	"?	military=barracks	poi_military\n"
	"?	military=bunker		poi_military\n"
	"?	military=danger_area	poi_danger_area\n"
	"?	military=range		poi_military\n"
	"?	natural=bay		poi_bay\n"
	"?	natural=peak,ele=*		poi_peak\n"     // show only major peaks with elevation
	"?	natural=tree		poi_tree\n"
	"n	place=city		town_label_2e5\n"
	"n	place=hamlet		town_label_2e2\n"
	"n	place=locality		town_label_2e0\n"
	"n	place=suburb		district_label\n"
	"n	place=town		town_label_2e4\n"
	"n	place=village		town_label_2e3\n"
	"n	power=tower		power_tower\n"
	"n	power=sub_station	power_substation\n"
	"n	railway=halt		poi_rail_halt\n"
	"n	railway=level_crossing	poi_level_crossing\n"
	"?	railway=station		poi_rail_station\n"
	"?	railway=tram_stop	poi_rail_tram_stop\n"
	"?	shop=baker		poi_shop_baker\n"
	"?	shop=bakery		poi_shop_baker\n"
	"?	shop=beverages		poi_shop_beverages\n"
	"?	shop=bicycle		poi_shop_bicycle\n"
	"?	shop=butcher		poi_shop_butcher\n"
	"?	shop=car		poi_car_dealer_parts\n"
	"?	shop=car_repair		poi_repair_service\n"
	"?	shop=clothes		poi_shop_apparel\n"
	"?	shop=convenience	poi_shop_grocery\n"
	"?	shop=drogist		poi_shop_drugstore\n"
	"?	shop=florist		poi_shop_florist\n"
	"?	shop=fruit		poi_shop_fruit\n"
	"?	shop=furniture		poi_shop_furniture\n"
	"?	shop=garden_centre	poi_shop_handg\n"
	"?	shop=hardware		poi_shop_handg\n"
	"?	shop=hairdresser	poi_hairdresser\n"
	"?	shop=kiosk		poi_shop_kiosk\n"
	"?	shop=optician		poi_shop_optician\n"
	"?	shop=parfum		poi_shop_parfum\n"
	"?	shop=photo		poi_shop_photo\n"
	"?	shop=shoes		poi_shop_shoes\n"
	"?	shop=supermarket	poi_shopping\n"
	"?	sport=10pin		poi_bowling\n"
	"?	sport=baseball		poi_baseball\n"
	"?	sport=basketball	poi_basketball\n"
	"?	sport=climbing		poi_climbing\n"
	"?	sport=golf		poi_golf\n"
	"?	sport=motor_sports	poi_motor_sport\n"
	"?	sport=skiing		poi_skiing\n"
	"?	sport=soccer		poi_soccer\n"
	"?	sport=stadium		poi_stadium\n"
	"?	sport=swimming		poi_swimming\n"
	"?	sport=tennis		poi_tennis\n"
	"?	tourism=attraction	poi_attraction\n"
	"?	tourism=camp_site	poi_camp_rv\n"
	"?	tourism=caravan_site	poi_camp_rv\n"
	"?	tourism=guest_house	poi_guesthouse\n"
	"?	tourism=hostel		poi_hostel\n"
	"?	tourism=hotel		poi_hotel\n"
	"?	tourism=information	poi_information\n"
	"?	tourism=motel		poi_motel\n"
	"?	tourism=museum		poi_museum_history\n"
	"?	tourism=picnic_site	poi_picnic\n"
	"?	tourism=theme_park	poi_resort\n"
	"?	tourism=viewpoint	poi_viewpoint\n"
	"?	tourism=zoo		poi_zoo\n"
	"n	traffic_sign=city_limit	traffic_sign_city_limit\n"
	"n	highway=speed_camera	tec_common\n"
	"w	*=*			street_unkn\n"
	"w	addr:interpolation=even	house_number_interpolation_even\n"
	"w	addr:interpolation=odd	house_number_interpolation_odd\n"
	"w	addr:interpolation=all	house_number_interpolation_all\n"
	"w	addr:interpolation=alphabetic	house_number_interpolation_alphabetic\n"
	"w	aerialway=cable_car	lift_cable_car\n"
	"w	aerialway=chair_lift	lift_chair\n"
	"w	aerialway=drag_lift	lift_drag\n"
	"w	aeroway=aerodrome	poly_airport\n"
	"w	aeroway=apron		poly_apron\n"
	"w	aeroway=runway		aeroway_runway\n"
	"w	aeroway=taxiway		aeroway_taxiway\n"
	"w	aeroway=terminal	poly_terminal\n"
	"w	amenity=college		poly_college\n"
	"w	amenity=grave_yard	poly_cemetery\n"
	"w	amenity=parking		poly_car_parking\n"
	"w	amenity=place_of_worship	poly_building\n"
	"w	amenity=university	poly_university\n"
	"w	boundary=administrative,admin_level=2	border_country\n"
	"w	boundary=civil		border_civil\n"
	"w	boundary=national_park	border_national_park\n"
	"w	boundary=political	border_political\n"
	"w	building=*		poly_building\n"
	"w	contour_ext=elevation_major	height_line_1\n"
	"w	contour_ext=elevation_medium	height_line_2\n"
	"w	contour_ext=elevation_minor	height_line_3\n"
	"w	highway=bridleway	bridleway\n"
	"w	highway=bus_guideway	bus_guideway\n"
	"w	highway=construction	street_construction\n"
	"w	highway=cyclepath	cycleway\n"
	"w	highway=cycleway	cycleway\n"
	"w	highway=footway		footway\n"
	"w	highway=footway,piste:type=nordic	footway_and_piste_nordic\n"
	"w	highway=living_street	living_street\n"
	"w	highway=minor		street_1_land\n"
	"w	highway=parking_lane	street_parking_lane\n"
	"w	highway=path				path\n"
	"w	highway=path,bicycle=designated		cycleway\n"
	"w	highway=path,bicycle=official		cycleway\n"
	"w	highway=path,bicycle=designated,foot=designated		cycleway\n"
	"w	highway=path,bicycle=official,foot=official		cycleway\n"
	"w	highway=path,foot=designated		footway\n"
	"w	highway=path,foot=official		footway\n"
	"w	highway=path,horse=designated		bridleway\n"
	"w	highway=path,horse=official		bridleway\n"
	"w	highway=path,sac_scale=alpine_hiking			hiking_alpine\n"
	"w	highway=path,sac_scale=demanding_alpine_hiking		hiking_alpine_demanding\n"
	"w	highway=path,sac_scale=demanding_mountain_hiking	hiking_mountain_demanding\n"
	"w	highway=path,sac_scale=difficult_alpine_hiking		hiking_alpine_difficult\n"
	"w	highway=path,sac_scale=hiking				hiking\n"
	"w	highway=path,sac_scale=mountain_hiking			hiking_mountain\n"
	"w	highway=pedestrian			street_pedestrian\n"
	"w	highway=pedestrian,area=1		poly_pedestrian\n"
	"w	highway=plaza				poly_plaza\n"
	"w	highway=motorway			highway_land\n"
	"w	highway=motorway,rural=0		highway_city\n"
	"w	highway=motorway_link			ramp\n"
	"w	highway=trunk				street_n_lanes\n"
	"w	highway=trunk_link			ramp\n"
	"w	highway=primary				street_4_land\n"
	"w	highway=primary,name=*,rural=1		street_4_land\n"
	"w	highway=primary,name=*			street_4_city\n"
	"w	highway=primary,rural=0			street_4_city\n"
	"w	highway=primary_link			ramp\n"
	"w	highway=secondary			street_3_land\n"
	"w	highway=secondary,name=*,rural=1	street_3_land\n"
	"w	highway=secondary,name=*		street_3_city\n"
	"w	highway=secondary,rural=0		street_3_city\n"
	"w	highway=secondary,area=1		poly_street_3\n"
	"w	highway=secondary_link			ramp\n"
	"w	highway=tertiary			street_2_land\n"
	"w	highway=tertiary,name=*,rural=1		street_2_land\n"
	"w	highway=tertiary,name=*			street_2_city\n"
	"w	highway=tertiary,rural=0		street_2_city\n"
	"w	highway=tertiary,area=1			poly_street_2\n"
	"w	highway=tertiary_link			ramp\n"
	"w	highway=residential			street_1_city\n"
	"w	highway=residential,area=1		poly_street_1\n"
	"w	highway=unclassified			street_1_city\n"
	"w	highway=unclassified,area=1		poly_street_1\n"
	"w	highway=road				street_1_city\n"
	"w	highway=service				street_service\n"
	"w	highway=service,area=1			poly_service\n"
	"w	highway=service,service=parking_aisle	street_parking_lane\n"
	"w	highway=track				track_gravelled\n"
	"w	highway=track,surface=grass		track_grass\n"
	"w	highway=track,surface=gravel		track_gravelled\n"
	"w	highway=track,surface=ground		track_ground\n"
	"w	highway=track,surface=paved		track_paved\n"
	"w	highway=track,surface=unpaved		track_unpaved\n"
	"w	highway=track,tracktype=grade1		track_paved\n"
	"w	highway=track,tracktype=grade2		track_gravelled\n"
	"w	highway=track,tracktype=grade3		track_unpaved\n"
	"w	highway=track,tracktype=grade4		track_ground\n"
	"w	highway=track,tracktype=grade5		track_grass\n"
	"w	highway=track,surface=paved,tracktype=grade1		track_paved\n"
	"w	highway=track,surface=gravel,tracktype=grade2		track_gravelled\n"
	"w	highway=track,surface=unpaved,tracktype=grade3		track_unpaved\n"
	"w	highway=track,surface=ground,tracktype=grade4		track_ground\n"
	"w	highway=track,surface=grass,tracktype=grade5		track_grass\n"
	"w	highway=unsurfaced			track_gravelled\n"
	"w	highway=steps				steps\n"
	"w	historic=archaeological_site	poly_archaeological_site\n"
	"w	historic=battlefield	poly_battlefield\n"
	"w	historic=ruins		poly_ruins\n"
	"w	historic=town_gate	poly_building\n"
	"w	landuse=allotments	poly_allotments\n"
	"w	landuse=basin		poly_basin\n"
	"w	landuse=brownfield	poly_brownfield\n"
	"w	landuse=cemetery	poly_cemetery\n"
	"w	landuse=commercial	poly_commercial\n"
	"w	landuse=construction	poly_construction\n"
	"w	landuse=farm		poly_farm\n"
	"w	landuse=farmland	poly_farm\n"
	"w	landuse=farmyard	poly_town\n"
	"w	landuse=forest		poly_wood\n"
	"w	landuse=greenfield	poly_greenfield\n"
	"w	landuse=industrial	poly_industry\n"
	"w	landuse=landfill	poly_landfill\n"
	"w	landuse=military	poly_military\n"
	"w	landuse=meadow		poly_meadow\n"
	"w	landuse=plaza		poly_plaza\n"
	"w	landuse=quarry		poly_quarry\n"
	"w	landuse=railway		poly_railway\n"
	"w	landuse=recreation_ground		poly_recreation_ground\n"
	"w	landuse=reservoir	poly_reservoir\n"
	"w	landuse=residential	poly_town\n"
	"w	landuse=residential,area=1	poly_town\n"
	"w	landuse=retail		poly_retail\n"
	"w	landuse=village_green	poly_village_green\n"
	"w	landuse=vineyard	poly_farm\n"
	"w	leisure=common		poly_common\n"
	"w	leisure=fishing		poly_fishing\n"
	"w	leisure=garden		poly_garden\n"
	"w	leisure=golf_course	poly_golf_course\n"
	"w	leisure=marina		poly_marina\n"
	"w	leisure=nature_reserve	poly_nature_reserve\n"
	"w	leisure=park		poly_park\n"
	"w	leisure=pitch		poly_sports_pitch\n"
	"w	leisure=playground	poly_playground\n"
	"w	leisure=sports_centre	poly_sport\n"
	"w	leisure=stadium		poly_sports_stadium\n"
	"w	leisure=track		poly_sports_track\n"
	"w	leisure=water_park	poly_water_park\n"
	"w	military=airfield	poly_airfield\n"
	"w	military=barracks	poly_barracks\n"
	"w	military=danger_area	poly_danger_area\n"
	"w	military=naval_base	poly_naval_base\n"
	"w	military=range		poly_range\n"
	"w	natural=beach		poly_beach\n"
	"w	natural=coastline	water_line\n"
	"w	natural=fell		poly_fell\n"
	"w	natural=glacier		poly_glacier\n"
	"w	natural=heath		poly_heath\n"
	"w	natural=land		poly_land\n"
	"w	natural=marsh		poly_marsh\n"
	"w	natural=meadow		poly_meadow\n"
	"w	natural=mud		poly_mud\n"
	"w	natural=scree		poly_scree\n"
	"w	natural=scrub		poly_scrub\n"
	"w	natural=water		poly_water\n"
	"w	natural=wood		poly_wood\n"
	"w	piste:type=downhill,piste:difficulty=advanced		piste_downhill_advanced\n"
	"w	piste:type=downhill,piste:difficulty=easy		piste_downhill_easy\n"
	"w	piste:type=downhill,piste:difficulty=expert		piste_downhill_expert\n"
	"w	piste:type=downhill,piste:difficulty=freeride		piste_downhill_freeride\n"
	"w	piste:type=downhill,piste:difficulty=intermediate	piste_downhill_intermediate\n"
	"w	piste:type=downhill,piste:difficulty=novice		piste_downhill_novice\n"
	"w	piste:type=nordic	piste_nordic\n"
	"w	place=suburb		poly_place1\n"
	"w	place=hamlet		poly_place2\n"
	"w	place=isolated_dwelling	poly_place2\n"
	"w	place=locality		poly_place2\n"
	"w	place=village		poly_place3\n"
	"w	place=municipality	poly_place4\n"
	"w	place=town		poly_place5\n"
	"w	place=city		poly_place6\n"
	"w	de:place=city		poly_place6\n"
	"w	power=line		powerline\n"
	"w	railway=abandoned	rail_abandoned\n"
	"w	railway=disused		rail_disused\n"
	"w	railway=light_rail	rail_light\n"
	"w	railway=monorail	rail_mono\n"
	"w	railway=narrow_gauge	rail_narrow_gauge\n"
	"w	railway=preserved	rail_preserved\n"
	"w	railway=rail		rail\n"
	"w	railway=subway		rail_subway\n"
	"w	railway=tram		rail_tram\n"
	"w	route=ferry		ferry\n"
	"w	route=ski		piste_nordic\n"
	"w	sport=*			poly_sport\n"
	"w	tourism=artwork		poly_artwork\n"
	"w	tourism=attraction	poly_attraction\n"
	"w	tourism=camp_site	poly_camp_site\n"
	"w	tourism=caravan_site	poly_caravan_site\n"
	"w	tourism=picnic_site	poly_picnic_site\n"
	"w	tourism=theme_park	poly_theme_park\n"
	"w	tourism=zoo		poly_zoo\n"
	"w	waterway=canal		water_canal\n"
	"w	waterway=drain		water_drain\n"
	"w	waterway=river		water_river\n"
	"w	waterway=riverbank	poly_water\n"
	"w	waterway=stream		water_stream\n"
	"w	barrier=ditch	ditch\n"
	"w	barrier=hedge	hedge\n"
	"w	barrier=fence	fence\n"
	"w	barrier=wall	wall\n"
	"w	barrier=retaining_wall	retaining_wall\n"
	"w	barrier=city_wall	city_wall\n"
};

static void
build_attrmap_line(char *line)
{
	char *t=NULL,*kvl=NULL,*i=NULL,*p,*kv;
	struct attr_mapping *attr_mapping=g_malloc0(sizeof(struct attr_mapping));
	int idx,attr_mapping_count=0;
	t=line;
	p=strpbrk(t," \t");
	if (p) {
		while (*p && isspace(*p))
			*p++='\0';
		kvl=p;
		p=strpbrk(kvl," \t");;
	}
	if (p) {
		while (*p && isspace(*p))
			*p++='\0';
		i=p;
	}
	if (t[0] == 'w') {
		if (! i)
			i="street_unkn";
	} else {
		if (! i)
			i="point_unkn";
	}
	attr_mapping->type=item_from_name(i);
	if (!attr_mapping->type) {
		printf("no id found for '%s'\n",i);
	}
	while ((kv=strtok(kvl, ","))) {
		kvl=NULL;
		if (!(idx=(int)(long)g_hash_table_lookup(attr_hash, kv))) {
			idx=attr_present_count++;
			g_hash_table_insert(attr_hash, kv, (gpointer)(long)idx);
		}
		attr_mapping=g_realloc(attr_mapping, sizeof(struct attr_mapping)+(attr_mapping_count+1)*sizeof(int));
		attr_mapping->attr_present_idx[attr_mapping_count++]=idx;
		attr_mapping->attr_present_idx_count=attr_mapping_count;
	}
	if (t[0]== 'w') {
		attr_mapping_way=g_realloc(attr_mapping_way, sizeof(*attr_mapping_way)*(attr_mapping_way_count+1));
		attr_mapping_way[attr_mapping_way_count++]=attr_mapping;
		if(item_is_poly_place(*attr_mapping)) {
			attr_mapping_rel2poly_place=g_realloc(attr_mapping_rel2poly_place, sizeof(*attr_mapping_rel2poly_place)*(attr_mapping_rel2poly_place_count+1));
			attr_mapping_rel2poly_place[attr_mapping_rel2poly_place_count++]=attr_mapping;
		}
	}
	if (t[0]== '?' && doway2poi) {
		attr_mapping_way2poi=g_realloc(attr_mapping_way2poi, sizeof(*attr_mapping_way2poi)*(attr_mapping_way2poi_count+1));
		attr_mapping_way2poi[attr_mapping_way2poi_count++]=attr_mapping;
	}
	if (t[0]!= 'w') {
		attr_mapping_node=g_realloc(attr_mapping_node, sizeof(*attr_mapping_node)*(attr_mapping_node_count+1));
		attr_mapping_node[attr_mapping_node_count++]=attr_mapping;
	}

}

static void
build_attrmap(FILE* rule_file)
{
	attr_hash=g_hash_table_new(g_str_hash, g_str_equal);
	attr_present_count=1;

    // build attribute map from rule file if given
    if( rule_file )
    {
        char buffer[200], *p;
        while (fgets( buffer, 200, rule_file )) {
            p=strchr(buffer,'\n');
            if(p)
                *p = 0;
            build_attrmap_line( g_strdup( buffer ) );
        }
        fclose( rule_file );
    }
    // use hardcoded default attributes
    else
    {
        char *p,*map=g_strdup(attrmap);
        while (map) {
            p=strchr(map,'\n');
            if (p)
                *p++='\0';
            if (strlen(map))
                build_attrmap_line(map);
            map=p;
        }
    }

	attr_present=g_malloc0(sizeof(*attr_present)*attr_present_count);
}

static void
build_countrytable(void)
{
	int i;
	char *names,*str,*tok;
	country_table_hash=g_hash_table_new(g_str_hash, g_str_equal);
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		names=g_strdup(country_table[i].names);
		str=names;
		while ((tok=strtok(str, ","))) {
			str=NULL;
			g_hash_table_insert(country_table_hash, tok,  (gpointer)&country_table[i]);
		}
	}
}

void
osm_warning(char *type, osmid id, int cont, char *fmt, ...)
{
	char str[4096];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	fprintf(stderr,"%shttp://www.openstreetmap.org/browse/%s/"OSMID_FMT" %s",cont ? "":"OSM Warning:",type,id,str);
}

void
osm_info(char *type, osmid id, int cont, char *fmt, ...)
{
	char str[4096];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	fprintf(stderr,"%shttp://www.openstreetmap.org/browse/%s/"OSMID_FMT" %s",cont ? "":"OSM Info:",type,id,str);
}

static void
attr_strings_clear(void)
{
	attr_strings_buffer_len=0;
	memset(attr_strings, 0, sizeof(attr_strings));
}

static void
attr_strings_save(enum attr_strings id, char *str)
{
	attr_strings[id]=attr_strings_buffer+attr_strings_buffer_len;
	strcpy(attr_strings[id], str);
	attr_strings_buffer_len+=strlen(str)+1;
}

osmid
item_bin_get_nodeid(struct item_bin *ib)
{
	unsigned long long *ret=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
	if (ret)
		return *ret;
	return 0;
}

osmid
item_bin_get_wayid(struct item_bin *ib)
{
	long long *ret=item_bin_get_attr(ib, attr_osm_wayid, NULL);
	if (ret)
		return *ret;
	return 0;
}

osmid
item_bin_get_relationid(struct item_bin *ib)
{
	unsigned long long *ret=item_bin_get_attr(ib, attr_osm_relationid, NULL);
	if (ret)
		return *ret;
	return 0;
}

osmid
item_bin_get_id(struct item_bin *ib)
{
	osmid ret;
	if (ib->type < 0x80000000)
		return item_bin_get_nodeid(ib);
	ret=item_bin_get_wayid(ib);
	if (!ret)
		ret=item_bin_get_relationid(ib);
	return ret;
}

static int node_is_tagged;
static void relation_add_tag(char *k, char *v);

static int
access_value(char *v)
{
	if (!strcmp(v,"1"))
		return 1;
	if (!strcmp(v,"yes"))
		return 1;
	if (!strcmp(v,"designated"))
		return 1;
	if (!strcmp(v,"official"))
		return 1;
	if (!strcmp(v,"permissive"))
		return 1;
	if (!strcmp(v,"0"))
		return 2;
	if (!strcmp(v,"no"))
		return 2;
	if (!strcmp(v,"agricultural"))
		return 2;
	if (!strcmp(v,"forestry"))
		return 2;
	if (!strcmp(v,"private"))
		return 2;
	if (!strcmp(v,"delivery"))
		return 2;
	if (!strcmp(v,"destination"))
		return 2;
	return 3;
}

static void 
osm_update_attr_present(char *k, char *v);

void
osm_add_tag(char *k, char *v)
{
	int level=2;
	if (in_relation) {
		relation_add_tag(k,v);
		return;
	}
	if (! strcmp(k,"ele"))
		level=9;
	if (! strcmp(k,"time"))
		level=9;
	if (! strcmp(k,"created_by"))
		level=9;
	if (! strncmp(k,"tiger:",6) || !strcmp(k,"AND_nodes"))
		level=9;
	if (! strcmp(k,"converted_by") || ! strcmp(k,"source"))
		level=8;
	if (! strncmp(k,"osmarender:",11) || !strncmp(k,"svg:",4))
		level=8;
	if (! strcmp(k,"layer"))
		level=7;
	if (! strcasecmp(v,"true") || ! strcasecmp(v,"yes"))
		v="1";
	if (! strcasecmp(v,"false") || ! strcasecmp(v,"no"))
		v="0";
	if (! strcmp(k,"oneway")) {
		if (!strcmp(v,"1")) {
			flags[0] |= AF_ONEWAY | AF_ROUNDABOUT_VALID;
		}
		if (! strcmp(v,"-1")) {
			flags[0] |= AF_ONEWAYREV | AF_ROUNDABOUT_VALID;
		}
		if (!in_way)
			level=6;
		else
			level=5;
	}
	if (! strcmp(k,"junction")) {
		if (! strcmp(v,"roundabout"))
			flags[0] |= AF_ONEWAY | AF_ROUNDABOUT | AF_ROUNDABOUT_VALID;
	}
	if (! strcmp(k,"maxspeed")) {
		if (strstr(v, "mph")) {
			maxspeed_attr_value = (int)floor(atof(v) * 1.609344);
		} else {
			maxspeed_attr_value = atoi(v);
		}
		if (maxspeed_attr_value)
			flags[0] |= AF_SPEED_LIMIT;
		level=5;
	}
	if (! strcmp(k,"toll")) {
		if (!strcmp(v,"1")) {
			flags[0] |= AF_TOLL;
		}
	}
	if (! strcmp(k,"access")) {
		if (strcmp(v,"destination")) 
			flagsa[access_value(v)] |= AF_DANGEROUS_GOODS|AF_EMERGENCY_VEHICLES|AF_TRANSPORT_TRUCK|AF_DELIVERY_TRUCK|AF_PUBLIC_BUS|AF_TAXI|AF_HIGH_OCCUPANCY_CAR|AF_CAR|AF_MOTORCYCLE|AF_MOPED|AF_HORSE|AF_BIKE|AF_PEDESTRIAN;
		else
			flags[0] |= AF_THROUGH_TRAFFIC_LIMIT;
		level=5;
	}
	if (! strcmp(k,"vehicle")) {
		flags[access_value(v)] |= AF_DANGEROUS_GOODS|AF_EMERGENCY_VEHICLES|AF_TRANSPORT_TRUCK|AF_DELIVERY_TRUCK|AF_PUBLIC_BUS|AF_TAXI|AF_HIGH_OCCUPANCY_CAR|AF_CAR|AF_MOTORCYCLE|AF_MOPED|AF_BIKE;
		level=5;
	}
	if (! strcmp(k,"motor_vehicle")) {
		flags[access_value(v)] |= AF_DANGEROUS_GOODS|AF_EMERGENCY_VEHICLES|AF_TRANSPORT_TRUCK|AF_DELIVERY_TRUCK|AF_PUBLIC_BUS|AF_TAXI|AF_HIGH_OCCUPANCY_CAR|AF_CAR|AF_MOTORCYCLE|AF_MOPED;
		level=5;
	}
	if (! strcmp(k,"bicycle")) {
		flags[access_value(v)] |= AF_BIKE;
		level=5;
	}
	if (! strcmp(k,"foot")) {
		flags[access_value(v)] |= AF_PEDESTRIAN;
		level=5;
	}
	if (! strcmp(k,"horse")) {
		flags[access_value(v)] |= AF_HORSE;
		level=5;
	}
	if (! strcmp(k,"moped")) {
		flags[access_value(v)] |= AF_MOPED;
		level=5;
	}
	if (! strcmp(k,"motorcycle")) {
		flags[access_value(v)] |= AF_MOTORCYCLE;
		level=5;
	}
	if (! strcmp(k,"motorcar")) {
		flags[access_value(v)] |= AF_CAR;
		level=5;
	}
	if (! strcmp(k,"hov")) {
		flags[access_value(v)] |= AF_HIGH_OCCUPANCY_CAR;
		level=5;
	}
	if (! strcmp(k,"bus")) {
		flags[access_value(v)] |= AF_PUBLIC_BUS;
		level=5;
	}
	if (! strcmp(k,"taxi")) {
		flags[access_value(v)] |= AF_TAXI;
		level=5;
	}
	if (! strcmp(k,"goods")) {
		flags[access_value(v)] |= AF_DELIVERY_TRUCK;
		level=5;
	}
	if (! strcmp(k,"hgv")) {
		flags[access_value(v)] |= AF_TRANSPORT_TRUCK;
		level=5;
	}
	if (! strcmp(k,"emergency")) {
		flags[access_value(v)] |= AF_EMERGENCY_VEHICLES;
		level=5;
	}
	if (! strcmp(k,"hazmat")) {
		flags[access_value(v)] |= AF_DANGEROUS_GOODS;
		level=5;
	}
	if (! strcmp(k,"tunnel") && !strcmp(v,"1")) {
		flags[0] |= AF_UNDERGROUND;
	}
	if (! strcmp(k,"note"))
		level=5;
	if (! strcmp(k,"name")) {
		attr_strings_save(attr_string_label, v);
		level=5;
	}
	if (! strcmp(k,"addr:email")) {
		attr_strings_save(attr_string_email, v);
		level=5;
	}
	if (! strcmp(k,"addr:suburb")) {
		attr_strings_save(attr_string_district_name, v);
		level=5;
	}
	if (! strcmp(k,"addr:housenumber")) {
		attr_strings_save(attr_string_house_number, v);
		level=5;
	}
	if (! strcmp(k,"addr:street")) {
		attr_strings_save(attr_string_street_name, v);
		level=5;
	}
	if (! strcmp(k,"phone")) {
		attr_strings_save(attr_string_phone, v);
		level=5;
	}
	if (! strcmp(k,"fax")) {
		attr_strings_save(attr_string_fax, v);
		level=5;
	}
	if (! strcmp(k,"postal_code")) {
		attr_strings_save(attr_string_postal, v);
		level=5;
	}
	if (! strcmp(k,"addr:postcode") && !attr_strings[attr_string_postal]) {
		attr_strings_save(attr_string_postal, v);
		level=5;
	}
	if (! strcmp(k,"openGeoDB:postal_codes") && !attr_strings[attr_string_postal]) {
		attr_strings_save(attr_string_postal, v);
		level=5;
	}
	if (! strcmp(k,"population")) {
		attr_strings_save(attr_string_population, v);
		level=5;
	}
	if (! strcmp(k,"openGeoDB:population") && !attr_strings[attr_string_population]) {
		attr_strings_save(attr_string_population, v);
		level=5;
	}
	if (! strcmp(k,"ref")) {
		if (in_way)
			attr_strings_save(attr_string_street_name_systematic, v);
		level=5;
	}
	if (! strcmp(k,"openGeoDB:is_in")) { 
		if (!is_in_buffer[0]) 
			strcpy(is_in_buffer, v); 
			level=5; 
	} 
	if (! strcmp(k,"is_in")) {
		if (!is_in_buffer[0])
			strcpy(is_in_buffer, v);
		level=5;
	}
	if (! strcmp(k,"is_in:country")) {
 		/**
		* Sometimes there is no is_in tag, only is_in:country.
		* I put this here so it can be overwritten by the previous if clause if there IS an is_in tag.
		*/
		strcpy(is_in_buffer, v);
		level=5;
	}
	if (! strcmp(k,"place_county")) {
		/** 
		* Ireland uses the place_county OSM tag to describe what county a town is in.
		* This would be equivalent to is_in: Town; Locality; Country
		* A real world example would be Node: Moycullen (52234625)
		* The tag is processed as Moycullen; Galway; Ireland
		* where Galway is the county
		*/
		strcpy(is_in_buffer, "Ireland");
		attr_strings_save(attr_string_county_name, v);
		level=5;
	}
	if (! strcmp(k,"gnis:ST_alpha")) {
		/*	assume a gnis tag means it is part of the USA:
			http://en.wikipedia.org/wiki/Geographic_Names_Information_System
			many US towns do not have is_in tags
		*/
		strcpy(is_in_buffer, "USA");
		level=5;
	}
	if (! strcmp(k,"lanes")) {
		level=5;
	}
	if (attr_debug_level >= level) {
		int bytes_left = sizeof( debug_attr_buffer ) - strlen(debug_attr_buffer) - 1;
		if ( bytes_left > 0 )
		{
			snprintf(debug_attr_buffer+strlen(debug_attr_buffer), bytes_left,  " %s=%s", k, v);
			debug_attr_buffer[ sizeof( debug_attr_buffer ) -  1 ] = '\0';
			node_is_tagged=1;
		}
	}
	if (level < 6)
		node_is_tagged=1;

	osm_update_attr_present(k, v);
}

static void 
osm_update_attr_present(char *k, char *v)
{
	int idx;
	char *p, buffer[BUFFER_SIZE*2+2];

	strcpy(buffer,"*=*");
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=1;

	sprintf(buffer,"%s=*", k);
	for(p=buffer;*p;p++)
		if(isspace(*p))	*p='_';
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=2;

	sprintf(buffer,"*=%s", v);
	for(p=buffer;*p;p++)
		if(isspace(*p))	*p='_';
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=2;

	sprintf(buffer,"%s=%s", k, v);
	for(p=buffer;*p;p++)
		if(isspace(*p))	*p='_';
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=4;
}

int coord_count;

static void
extend_buffer(struct buffer *b)
{
	b->malloced+=b->malloced_step;
	b->base=realloc(b->base, b->malloced);
	if (b->base == NULL) {
		fprintf(stderr,"realloc of %d bytes failed\n",(int)b->malloced);
		exit(1);
	}

}

osmid nodeid_last;
GHashTable *node_hash,*way_hash;

static void
node_buffer_to_hash(void)
{
	int i,count=node_buffer.size/sizeof(struct node_item);
	struct node_item *ni=(struct node_item *)node_buffer.base;
	for (i = 0 ; i < count ; i++)
		g_hash_table_insert(node_hash, (gpointer)(long)(ni[i].id), (gpointer)(long)i);
}

static struct node_item *ni;

void
flush_nodes(int final)
{
	fprintf(stderr,"flush_nodes %d\n",final);
	save_buffer("coords.tmp",&node_buffer,slices*slice_size);
	if (!final) {
		node_buffer.size=0;
	}
	slices++;
}

void
osm_add_node(osmid id, double lat, double lon)
{
      in_node=1;
      if (node_buffer.size + sizeof(struct node_item) > node_buffer.malloced)
	      extend_buffer(&node_buffer);
      attr_strings_clear();
      node_is_tagged=0;
      nodeid=id;
      item.type=type_point_unkn;
      debug_attr_buffer[0]='\0';
      is_in_buffer[0]='\0';
      debug_attr_buffer[0]='\0';
      osmid_attr.type=attr_osm_nodeid;
      osmid_attr.len=3;
      osmid_attr_value=id;
      if (node_buffer.size + sizeof(struct node_item) > slice_size) {
	      flush_nodes(0);
      }
      ni=(struct node_item *)(node_buffer.base+node_buffer.size);
      ni->id=id;
      ni->ref_node=0;
      ni->ref_way=0;
      ni->ref_ref=0;
      ni->dummy=0;
      ni->c.x=lon*6371000.0*M_PI/180;
      ni->c.y=log(tan(M_PI_4+lat*M_PI/360))*6371000.0;
      node_buffer.size+=sizeof(struct node_item);
      if (! node_hash) {
	      if (ni->id > nodeid_last) {
		      nodeid_last=ni->id;
	      } else {
		      fprintf(stderr,"INFO: Nodes out of sequence (new " OSMID_FMT " vs old " OSMID_FMT "), adding hash\n", (osmid)ni->id, nodeid_last);
		      node_hash=g_hash_table_new(NULL, NULL);
		      node_buffer_to_hash();
	      }
      } else
	      if (!g_hash_table_lookup(node_hash, (gpointer)(long)(ni->id)))
		      g_hash_table_insert(node_hash, (gpointer)(long)(ni->id), (gpointer)(long)(ni-(struct node_item *)node_buffer.base));
	      else {
		      node_buffer.size-=sizeof(struct node_item);
		      nodeid=0;
	      }

}

void
clear_node_item_buffer(void)
{
      int j,count=node_buffer.size/sizeof(struct node_item);
      struct node_item *ni=(struct node_item *)(node_buffer.base);
      for (j = 0 ; j < count ; j++) {
	      ni[j].ref_way=0;
      }
}

static struct node_item *
node_item_get(osmid id)
{
      struct node_item *ni=(struct node_item *)(node_buffer.base);
      int count=node_buffer.size/sizeof(struct node_item);
      int interval=count/4;
      int p=count/2;
      if(interval==0) {
	      // If fewer than 4 nodes defined so far set interval to 1 to
	      // avoid infinite loop
	      interval = 1;
      }
      if (node_hash) {
	      int i;
	      i=(int)(long)(g_hash_table_lookup(node_hash, (gpointer)(long)(id)));
	      return ni+i;
      }
      if (ni[0].id > id)
	      return NULL;
      if (ni[count-1].id < id)
	      return NULL;
      while (ni[p].id != id) {
#if 0
	      fprintf(stderr,"p=%d count=%d interval=%d id=%d ni[p].id=%d\n", p, count, interval, id, ni[p].id);
#endif
	      if (ni[p].id < id) {
		      p+=interval;
		      if (interval == 1) {
			      if (p >= count)
				      return NULL;
			      if (ni[p].id > id)
				      return NULL;
		      } else {
			      if (p >= count)
				      p=count-1;
		      }
	      } else {
		      p-=interval;
		      if (interval == 1) {
			      if (p < 0)
				      return NULL;
			      if (ni[p].id < id)
				      return NULL;
		      } else {
			      if (p < 0)
				      p=0;
		      }
	      }
	      if (interval > 1)
		      interval/=2;
      }
      return &ni[p];
}
#if 0
static int
load_node(FILE *coords, int p, struct node_item *ret)
{
      fseek(coords, p*sizeof(struct node_item), SEEK_SET);
      if (fread(ret, sizeof(*ret), 1, coords) != 1) {
	      fprintf(stderr,"read failed\n");
	      return 0;
      }
      return 1;
}
#endif

#if 0
static int
node_item_get_from_file(FILE *coords, osmid id, struct node_item *ret)
{
      int count;
      int interval;
      int p;
      if (node_hash) {
	      int i;
	      i=(int)(long)(g_hash_table_lookup(node_hash, (gpointer)(long)id));
	      fseek(coords, i*sizeof(*ret), SEEK_SET);
	      if (fread(ret, sizeof(*ret), 1, coords) == 1)
		      return 1;
	      else
		      return 0;
      }

      fseek(coords, 0, SEEK_END);
      count=ftello(coords)/sizeof(struct node_item);
      interval=count/4;
      p=count/2;
      if(interval==0) {
	      // If fewer than 4 nodes defined so far set interval to 1 to
	      // avoid infinite loop
	      interval = 1;
      }
      if (!load_node(coords, p, ret))
	      return 0;
      for (;;) {
	      if (ret->id == id)
		      return 1;
	      if (ret->id < id) {
		      p+=interval;
		      if (interval == 1) {
			      if (p >= count)
				      return 0;
			      if (!load_node(coords, p, ret))
				      return 0;
			      if (ret->id > id)
				      return 0;
		      } else {
			      if (p >= count)
				      p=count-1;
			      if (!load_node(coords, p, ret))
				      return 0;
		      }
	      } else {
		      p-=interval;
		      if (interval == 1) {
			      if (p < 0)
				      return 0;
			      if (!load_node(coords, p, ret))
				      return 0;
			      if (ret->id < id)
				      return 0;
		      } else {
			      if (p < 0)
				      p=0;
			      if (!load_node(coords, p, ret))
				      return 0;
		      }
	      }
	      if (interval > 1)
		      interval/=2;
      }
}
#endif 
void
osm_add_way(osmid id)
{
	static osmid wayid_last;

	in_way=1;
	wayid=id;
	coord_count=0;
	attr_strings_clear();
	item.type=type_street_unkn;
	debug_attr_buffer[0]='\0';
	maxspeed_attr_value=0;
	flags_attr_value = 0;
	memset(flags, 0, sizeof(flags));
	memset(flagsa, 0, sizeof(flagsa));
	debug_attr_buffer[0]='\0';
	osmid_attr_value=id;
	if (wayid < wayid_last && !way_hash) {
		fprintf(stderr,"INFO: Ways out of sequence (new "OSMID_FMT" vs old "OSMID_FMT"), adding hash\n", wayid, wayid_last);
		way_hash=g_hash_table_new(NULL, NULL);
	}
	wayid_last=wayid;
}

char relation_type[BUFFER_SIZE];
char iso_code[BUFFER_SIZE];
int admin_level;
int boundary;

void
osm_add_relation(osmid id)
{
	osmid_attr_value=id;
	in_relation=1;
	debug_attr_buffer[0]='\0';
	relation_type[0]='\0';
	iso_code[0]='\0';
	admin_level=-1;
	boundary=0;
	item_bin_init(item_bin, type_none);
	item_bin_add_attr_longlong(item_bin, attr_osm_relationid, osmid_attr_value);
}

static int
country_id_from_iso2(char *iso)
{
	int ret=0;
	if (iso) {
		struct country_search *search;
		struct attr country_iso2,country_id;
		struct item *item;
		country_iso2.type=attr_country_iso2;
		country_iso2.u.str=iso;
		search=country_search_new(&country_iso2,0);
		if ((item=country_search_get_item(search)) && item_attr_get(item, attr_country_id, &country_id))
			ret=country_id.u.num;
			
		country_search_destroy(search);
	}
	return ret;
}

static struct country_table *
country_from_countryid(int id)
{
	int i;
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		if (country_table[i].countryid == id)
			return &country_table[i];
	}
	return NULL;
}

struct country_table *
country_from_iso2(char *iso)
{
	return country_from_countryid(country_id_from_iso2(iso));
}


void
osm_end_relation(struct maptool_osm *osm)
{
	enum item_type type;

	in_relation=0;

	if(attr_longest_match(attr_mapping_rel2poly_place, attr_mapping_rel2poly_place_count, &type, 1)) {
		item_bin->type=type;
	}
	else 
		type=type_none;

	if ((!strcmp(relation_type, "multipolygon") || !strcmp(relation_type, "boundary")) && (boundary || type!=type_none)) {
#if 0
		if (admin_level == 2) {
			FILE *f;
			fprintf(stderr,"Multipolygon for %s\n", iso_code);
			char *name=g_strdup_printf("country_%s.tmp",iso_code);
			f=fopen(name,"w");
			item_bin_write(item_bin, f);
			fclose(f);
		}
#endif
		item_bin_write(item_bin, osm->boundaries);
	}

	if (!strcmp(relation_type, "restriction") && (item_bin->type == type_street_turn_restriction_no || item_bin->type == type_street_turn_restriction_only))
		item_bin_write(item_bin, osm->turn_restrictions);

	if (!strcmp(relation_type, "associatedStreet") )
		item_bin_write(item_bin, osm->associated_streets);
		
	attr_longest_match_clear();
}

void
osm_add_member(int type, osmid ref, char *role)
{
	char member_buffer[BUFFER_SIZE*3+3];
	struct attr memberattr = { attr_osm_member };

	sprintf(member_buffer,"%d:"LONGLONG_FMT":%s", type, (long long) ref, role);
	memberattr.u.str=member_buffer;
	item_bin_add_attr(item_bin, &memberattr);
}

static void
relation_add_tag(char *k, char *v)
{
	int add_tag=1;
#if 0
	fprintf(stderr,"add tag %s %s\n",k,v);
#endif
	if (!strcmp(k,"type")) {
		strcpy(relation_type, v);
		add_tag=0;
	}
	else if (!strcmp(k,"restriction")) {
		if (!strncmp(v,"no_",3)) {
			item_bin->type=type_street_turn_restriction_no;
			add_tag=0;
		} else if (!strncmp(v,"only_",5)) {
			item_bin->type=type_street_turn_restriction_only;
			add_tag=0;
		} else {
			item_bin->type=type_none;
			osm_warning("relation", osmid_attr_value, 0, "Unknown restriction %s\n",v);
		}
	} else if (!strcmp(k,"admin_level")) {
		admin_level=atoi(v);
	} else if (!strcmp(k,"boundary")) {
		if (!strcmp(v,"administrative") || !strcmp(v,"postal_code")) {
			boundary=1;
		}
	} else if (!strcmp(k,"ISO3166-1")) {
		strcpy(iso_code, v);
	}
	if (add_tag) {
		char *tag;
		tag=g_alloca(strlen(k)+strlen(v)+2);
		sprintf(tag,"%s=%s",k,v);
		item_bin_add_attr_string(item_bin, attr_osm_tag, tag);
	}

	osm_update_attr_present(k,v);
}


static int
attr_longest_match(struct attr_mapping **mapping, int mapping_count, enum item_type *types, int types_count)
{
	int i,j,longest=0,ret=0,sum,val;
	struct attr_mapping *curr;
	for (i = 0 ; i < mapping_count ; i++) {
		sum=0;
		curr=mapping[i];
		for (j = 0 ; j < curr->attr_present_idx_count ; j++) {
			val=attr_present[curr->attr_present_idx[j]];
			if (val)
				sum+=val;
			else {
				sum=-1;
				break;
			}
		}
		if (sum > longest) {
			longest=sum;
			ret=0;
		}
		if (sum > 0 && sum == longest && ret < types_count)
			types[ret++]=curr->type;
	}
	return ret;
}

static void
attr_longest_match_clear(void)
{
	memset(attr_present, 0, sizeof(*attr_present)*attr_present_count);
}

void
osm_end_way(struct maptool_osm *osm)
{
	int i,count;
	int *def_flags,add_flags;
	enum item_type types[10];
	struct item_bin *item_bin;
	int count_lines=0, count_areas=0;

	in_way=0;

	if (! osm->ways)
		return;

	if (dedupe_ways_hash) {
		if (g_hash_table_lookup(dedupe_ways_hash, (gpointer)(long)wayid))
			return;
		g_hash_table_insert(dedupe_ways_hash, (gpointer)(long)wayid, (gpointer)1);
	}

	count=attr_longest_match(attr_mapping_way, attr_mapping_way_count, types, sizeof(types)/sizeof(enum item_type));
	if (!count) {
		count=1;
		types[0]=type_street_unkn;
	}
	if (count >= 10) {
		fprintf(stderr,"way id "OSMID_FMT"\n",osmid_attr_value);
		dbg_assert(count < 10);
	}
	for (i = 0 ; i < count ; i++) {
		add_flags=0;
		if (types[i] == type_none)
			continue;
		if (ignore_unkown && (types[i] == type_street_unkn || types[i] == type_point_unkn))
			continue;
		if (types[i] != type_street_unkn) {
			if(types[i]<type_area) 	
				count_lines++;	
			else	
				count_areas++;
		}
		item_bin=init_item(types[i]);
		item_bin_add_coord(item_bin, coord_buffer, coord_count);
		nodes_ref_item_bin(item_bin);
		def_flags=item_get_default_flags(types[i]);
		if (def_flags) {
			flags_attr_value=((*def_flags & ~flagsa[2]) | flags[0] | flags[1] | flagsa[1]) & ~flags[2];
			if (flags_attr_value != *def_flags)
				add_flags=1;
		}
		item_bin_add_attr_string(item_bin, def_flags ? attr_street_name : attr_label, attr_strings[attr_string_label]);
		item_bin_add_attr_string(item_bin, attr_district_name, attr_strings[attr_string_district_name]);
		item_bin_add_attr_string(item_bin, attr_street_name_systematic, attr_strings[attr_string_street_name_systematic]);
		item_bin_add_attr_longlong(item_bin, attr_osm_wayid, osmid_attr_value);
		if (debug_attr_buffer[0])
			item_bin_add_attr_string(item_bin, attr_debug, debug_attr_buffer);
		if (add_flags)
			item_bin_add_attr_int(item_bin, attr_flags, flags_attr_value);
		if (maxspeed_attr_value)
			item_bin_add_attr_int(item_bin, attr_maxspeed, maxspeed_attr_value);
		if(i>0)
			item_bin_add_attr_int(item_bin, attr_duplicate, 1);
		item_bin_write(item_bin,osm->ways);
	}
	if(osm->line2poi) {
		count=attr_longest_match(attr_mapping_way2poi, attr_mapping_way2poi_count, types, sizeof(types)/sizeof(enum item_type));
		dbg_assert(count < 10);
		for (i = 0 ; i < count ; i++) {
			if (types[i] == type_none || types[i] == type_point_unkn)
				continue;
			item_bin=init_item(types[i]);
			item_bin_add_coord(item_bin, coord_buffer, coord_count);
			item_bin_add_attr_string(item_bin, attr_label, attr_strings[attr_string_label]);
			item_bin_add_attr_string(item_bin, attr_house_number, attr_strings[attr_string_house_number]);
			item_bin_add_attr_string(item_bin, attr_district_name, attr_strings[attr_string_district_name]);
			item_bin_add_attr_string(item_bin, attr_street_name, attr_strings[attr_string_street_name]);
			item_bin_add_attr_string(item_bin, attr_phone, attr_strings[attr_string_phone]);
			item_bin_add_attr_string(item_bin, attr_fax, attr_strings[attr_string_fax]);
			item_bin_add_attr_string(item_bin, attr_email, attr_strings[attr_string_email]);
			item_bin_add_attr_string(item_bin, attr_county_name, attr_strings[attr_string_county_name]); 
			item_bin_add_attr_string(item_bin, attr_url, attr_strings[attr_string_url]);
			item_bin_add_attr_longlong(item_bin, attr_osm_wayid, osmid_attr_value);
			item_bin_write(item_bin, count_areas<=count_lines ? osm->line2poi:osm->poly2poi);
		}
	}
	attr_longest_match_clear();
}

void
osm_end_node(struct maptool_osm *osm)
{
	int count,i;
	char *postal;
	enum item_type types[10];
	struct item_bin *item_bin;
	in_node=0;

	if (!osm->nodes || ! node_is_tagged || ! nodeid)
		return;
	count=attr_longest_match(attr_mapping_node, attr_mapping_node_count, types, sizeof(types)/sizeof(enum item_type));
	if (!count) {
		types[0]=type_point_unkn;
		count=1;
	}
	if (count >= 10) {
		fprintf(stderr,"node id "OSMID_FMT"\n",osmid_attr_value);
		dbg_assert(count < 10);
	}
	for (i = 0 ; i < count ; i++) {
		if (types[i] == type_none)
			continue;
		if (ignore_unkown && (types[i] == type_street_unkn || types[i] == type_point_unkn))
			continue;
		item_bin=init_item(types[i]);
		if (item_is_town(*item_bin) && attr_strings[attr_string_population])
			item_bin_set_type_by_population(item_bin, atoi(attr_strings[attr_string_population]));
		item_bin_add_coord(item_bin, &ni->c, 1);
		item_bin_add_attr_string(item_bin, item_is_town(*item_bin) ? attr_town_name : attr_label, attr_strings[attr_string_label]);
		item_bin_add_attr_string(item_bin, attr_house_number, attr_strings[attr_string_house_number]);
		item_bin_add_attr_string(item_bin, attr_street_name, attr_strings[attr_string_street_name]);
		item_bin_add_attr_string(item_bin, attr_phone, attr_strings[attr_string_phone]);
		item_bin_add_attr_string(item_bin, attr_fax, attr_strings[attr_string_fax]);
		item_bin_add_attr_string(item_bin, attr_email, attr_strings[attr_string_email]);
		item_bin_add_attr_string(item_bin, attr_county_name, attr_strings[attr_string_county_name]);
		item_bin_add_attr_string(item_bin, attr_url, attr_strings[attr_string_url]);
		item_bin_add_attr_longlong(item_bin, attr_osm_nodeid, osmid_attr_value);
		item_bin_add_attr_string(item_bin, attr_debug, debug_attr_buffer);
		postal=attr_strings[attr_string_postal];
		if (postal) {
			char *sep=strchr(postal,',');
			if (sep)
				*sep='\0';
			item_bin_add_attr_string(item_bin, item_is_town(*item_bin) ? attr_town_postal : attr_postal, postal);
		}
		item_bin_write(item_bin,osm->nodes);
		if (item_is_town(*item_bin) && attr_strings[attr_string_label] && osm->towns) {
			item_bin=init_item(item_bin->type);
			item_bin_add_coord(item_bin, &ni->c, 1);
			item_bin_add_attr_string(item_bin, attr_osm_is_in, is_in_buffer);
			item_bin_add_attr_longlong(item_bin, attr_osm_nodeid, osmid_attr_value);
			item_bin_add_attr_string(item_bin, attr_town_postal, postal);
			item_bin_add_attr_string(item_bin, attr_county_name, attr_strings[attr_string_county_name]);
			item_bin_add_attr_string(item_bin, item_is_district(*item_bin)?attr_district_name:attr_town_name, attr_strings[attr_string_label]);
			item_bin_write(item_bin, osm->towns);
		}
	}
	processed_nodes_out++;
	attr_longest_match_clear();
}

static struct country_table *
osm_process_town_unknown_country(void)
{
	static struct country_table *unknown;
	if (!unknown)
		unknown=country_from_countryid(999);

	return unknown;
}

static struct country_table *
osm_process_town_by_is_in(struct item_bin *ib,char *is_in, struct attr *attrs, GHashTable *town_hash)
{
	struct country_table *result=NULL, *lookup;
	char *tok,*dup=g_strdup(is_in),*buf=dup;

	int find_town_name = 0;

	if (item_is_district(*ib))
		find_town_name = 1;

	while ((tok=strtok(buf, ",;"))) {
		while (*tok==' ')
			tok++;
		if (find_town_name && g_hash_table_lookup(town_hash, tok)) {
			attrs[10].type = attr_town_name;
			attrs[10].u.str = g_strdup(tok);
			find_town_name = 0;
		}
		lookup=g_hash_table_lookup(country_table_hash,tok);
		if (lookup) {
			if (result && result->countryid != lookup->countryid) {
				char *label=item_bin_get_attr(ib, attr_town_name, NULL);
				osm_warning("node",item_bin_get_nodeid(ib),0,"conflict for %s is_in=%s country %d vs %d\n", label, is_in, lookup->countryid, result->countryid);
			}
			result=lookup;
		}
		buf=NULL;
	}
	g_free(dup);

	return result;
}

static struct country_table *
osm_process_town_by_boundary(GList *bl, struct item_bin *ib, struct coord *c, struct attr *attrs)
{
	GList *l,*matches=boundary_find_matches(bl, c);
	struct boundary *match=NULL;
	
	l=matches;
	while (l) {
		struct boundary *b=l->data;
		if (b->country) {
			if (match && match->country->countryid!=b->country->countryid) {
				osm_warning("node",item_bin_get_nodeid(ib),0,"node (0x%x,0x%x) conflict country ", c->x, c->y);
				osm_warning("relation",boundary_relid(match),1,"country %d vs ",match->country->countryid);
				osm_warning("relation",boundary_relid(b),1,"country %d\n",b->country->countryid);
			}
			match=b;
		}
		l=g_list_next(l);
	}

	if (match) {
		if (match && match->country && match->country->admin_levels) {
			long long *nodeid=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
			long long node_id=0;
			int end=strlen(match->country->admin_levels)+3;
			int a;
			int max_adm_level=0;

			if(nodeid)
				node_id=*nodeid;

			l=matches;
			while (l) {
				struct boundary *b=l->data;
				char *admin_level=osm_tag_value(b->ib, "admin_level");
				char *postal=osm_tag_value(b->ib, "postal_code");
				if (admin_level) {
					char *name;
					a=atoi(admin_level);
					if (a > 2 && a < end) {
						enum attr_type attr_type=attr_none;
						switch(match->country->admin_levels[a-3]) {
						case 's':
							attr_type=attr_state_name;
							break;
						case 'c':
							attr_type=attr_county_name;
							break;
						case 'M':
							b->ib->type=type_poly_place6;
						case 'm':
							attr_type=attr_municipality_name;
							break;
						}
						name=osm_tag_value(b->ib, "name");
						if (name && attr_type != attr_none) {
							attrs[a-2].type=attr_type;
							attrs[a-2].u.str=name;
						}
					}
					if(b->admin_centre && b->admin_centre==node_id) {
						if(!max_adm_level || max_adm_level<a){
							max_adm_level=a;
						}
					}
				}
				if (postal) {
					attrs[0].type=attr_town_postal;
					attrs[0].u.str=postal;
				}
				l=g_list_next(l);
			}

			/* Administrative centres are not to be contained in their own districts. */
			if(max_adm_level>0)
				for(a=end-1;a>max_adm_level && a>2;a--)
					attrs[a-2].type=type_none;
		}
		g_list_free(matches);	
		return match->country; 
	} else {
		g_list_free(matches);	
		return NULL;
	}
}

static void 
osm_town_relations_to_poly(GList *boundaries, FILE *towns_poly)
{
	while(boundaries) {
		struct boundary *b=boundaries->data;
		if(item_is_poly_place(*b->ib)) {
			GList *s=b->sorted_segments;
			while(s) {
				struct geom_poly_segment *seg=s->data;
				if((seg->type==geom_poly_segment_type_way_outer || seg->type==geom_poly_segment_type_way_unknown) && coord_is_equal(*seg->first,*seg->last)) {
					struct item_bin *ib=init_item(b->ib->type);
					void *a;
					item_bin_add_coord(ib, seg->first, seg->last-seg->first+1);
					a=osm_tag_value(b->ib, "name");
					if(a)
						item_bin_add_attr_string(ib,attr_label,a);
					a=osm_tag_value(b->ib, "osm_relationid");
					if(a)
						item_bin_add_attr_longlong(ib,attr_osm_relationid,atol(a));
					item_bin_write(ib, towns_poly);
				}
				s=g_list_next(s);
			}
		}
		osm_town_relations_to_poly(b->children, towns_poly);
		boundaries=g_list_next(boundaries);
	}
}


void
osm_process_towns(FILE *in, FILE *boundaries, FILE *ways, char *suffix)
{
	struct item_bin *ib;
	GList *bl;
	GHashTable *town_hash;
	struct attr attrs[11];
	FILE *towns_poly;

	profile(0,NULL);
	bl=process_boundaries(boundaries, ways);

	profile(1,"processed boundraries\n");

	town_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	while ((ib=read_item(in)))  {
		if (!item_is_district(*ib))
		{
			char *townname=item_bin_get_attr(ib, attr_town_name, NULL);
			g_hash_table_insert(town_hash, strdup(townname), (gpointer)1);
		}
	}
	fseek(in, 0, SEEK_SET);

	profile(1, "Finished town table rebuild\n");

	while ((ib=read_item(in)))  {
		struct coord *c=(struct coord *)(ib+1);
		struct country_table *result=NULL;
		char *is_in=item_bin_get_attr(ib, attr_osm_is_in, NULL);
		int i;

		memset(attrs, 0, sizeof(attrs));
		result=osm_process_town_by_boundary(bl, ib, c, attrs);
		if (!result)
			result=osm_process_town_by_is_in(ib, is_in, attrs, town_hash);
		else if (item_is_district(*ib)) // just for the town name
			osm_process_town_by_is_in(ib, is_in, attrs, town_hash);

		// treat a district like a town, if we could not find the town it blongs to
		if (!item_bin_get_attr(ib, attr_town_name, NULL) && attrs[10].type != attr_town_name) {
			char *district_name = item_bin_get_attr(ib, attr_district_name, NULL);

			if (district_name) {
				struct attr attr_new_town_name;
				attr_new_town_name.type = attr_town_name;
				attr_new_town_name.u.str = district_name;

				item_bin_add_attr(ib, &attr_new_town_name);
				item_bin_remove_attr(ib, district_name);
			}
		}

		if (!result && unknown_country)
			result=osm_process_town_unknown_country();
		if (result) {
			if (!result->file) {
				char *name=g_strdup_printf("country_%d.unsorted.tmp", result->countryid);
				result->file=fopen(name,"wb");
				g_free(name);
			}
			if (result->file) {
				long long *nodeid;
				if (is_in)
					item_bin_remove_attr(ib, is_in);
				nodeid=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
				if (nodeid)
					item_bin_remove_attr(ib, nodeid);
				if (attrs[0].type != attr_none) {
					char *postal=item_bin_get_attr(ib, attr_town_postal, NULL);
					if (postal)
						item_bin_remove_attr(ib, postal);
				}
				for (i = 0 ; i < 11 ; i++) {
					if (attrs[i].type != attr_none)
						item_bin_add_attr(ib, &attrs[i]);
				}
				if(item_bin_get_attr(ib, attr_district_name, NULL))
					item_bin_write_match(ib, attr_district_name, attr_district_name_match, 5, result->file);
				else
					item_bin_write_match(ib, attr_town_name, attr_town_name_match, 5, result->file);
			}
		}
	}

	towns_poly=tempfile(suffix,"towns_poly",1);
	osm_town_relations_to_poly(bl, towns_poly);
	fclose(towns_poly);
	
	g_hash_table_destroy(town_hash);
	free_boundaries(bl);

	profile(0, "Finished processing towns\n");
}

void
sort_countries(int keep_tmpfiles)
{
	int i;
	struct country_table *co;
	char *name_in,*name_out;
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if (co->file) {
			fclose(co->file);
			co->file=NULL;
		}
		name_in=g_strdup_printf("country_%d.unsorted.tmp", co->countryid);
		name_out=g_strdup_printf("country_%d.tmp", co->countryid);
		co->r=world_bbox;
		item_bin_sort_file(name_in, name_out, &co->r, &co->size);
		if (!keep_tmpfiles)
			unlink(name_in);
		g_free(name_in);
		g_free(name_out);
	}
}

struct relation_member {
	int type;
	long long id;
	char *role;
};

static int
get_relation_member(char *str, struct relation_member *memb)
{
	int len;
	sscanf(str,"%d:"LONGLONG_FMT":%n",&memb->type,&memb->id,&len);
	memb->role=str+len;
	return 1;
}

static int
search_relation_member(struct item_bin *ib, char *role, struct relation_member *memb, int *min_count)
{
	char *str=NULL;
	int count=0;
	while ((str=item_bin_get_attr(ib, attr_osm_member, str))) {
		if (!get_relation_member(str, memb))
			return 0;
		count++;
	 	if (!strcmp(memb->role, role) && (!min_count || *min_count < count)) {
			if (min_count)
				*min_count=count;
			return 1;
		}
	}
	return 0;
}

#if 0
static int
load_way_index(FILE *ways_index, int p, long long *idx)
{
	int step=sizeof(*idx)*2;
	fseek(ways_index, p*step, SEEK_SET);
	if (fread(idx, step, 1, ways_index) != 1) {
		fprintf(stderr,"read failed\n");
		return 0;
	}
	return 1;
}
#endif

#if 0
static int
seek_to_way(FILE *way, FILE *ways_index, osmid wayid)
{
	long offset;
	long long idx[2];
	int count,interval,p;
	if (way_hash) {
		if (!(g_hash_table_lookup_extended(way_hash, (gpointer)(long)wayid, NULL, (gpointer)&offset)))
			return 0;
		fseek(way, offset, SEEK_SET);
		return 1;
	}
	fseek(ways_index, 0, SEEK_END);
	count=ftello(ways_index)/sizeof(idx);
	interval=count/4;
	p=count/2;
	if(interval==0) {
		// If fewer than 4 nodes defined so far set interval to 1 to
		// avoid infinite loop
		interval = 1;
	}
	if (!load_way_index(ways_index, p, idx))
		return 0;
	for (;;) {
		if (idx[0] == wayid) {
			fseek(way, idx[1], SEEK_SET);
			return 1;
		}
		if (idx[0] < wayid) {
			p+=interval;
			if (interval == 1) {
				if (p >= count)
					return 0;
				if (!load_way_index(ways_index, p, idx))
					return 0;
				if (idx[0] > wayid)
					return 0;
			} else {
				if (p >= count)
					p=count-1;
				if (!load_way_index(ways_index, p, idx))
					return 0;
			}
		} else {
			p-=interval;
			if (interval == 1) {
				if (p < 0)
					return 0;
				if (!load_way_index(ways_index, p, idx))
					return 0;
				if (idx[0] < wayid)
					return 0;
			} else {
				if (p < 0)
					p=0;
				if (!load_way_index(ways_index, p, idx))
					return 0;
			}
		}
		if (interval > 1)
			interval/=2;
	}
}
#endif

#if 0
static struct coord *
get_way(FILE *way, FILE *ways_index, struct coord *c, long long wayid, struct item_bin *ret, int debug)
{
	long long currid;
	int last;
	struct coord *ic;
	if (!seek_to_way(way, ways_index, wayid)) {
		if (debug)
			fprintf(stderr,"not found in index");
		return NULL;
	}
	while (item_bin_read(ret, way)) {
		currid=item_bin_get_wayid(ret);
		if (debug)
			fprintf(stderr,LONGLONG_FMT":",currid);
		if (currid != wayid)
			return NULL;
		ic=(struct coord *)(ret+1);
		last=ret->clen/2-1;
		if (debug)
			fprintf(stderr,"(0x%x,0x%x)-(0x%x,0x%x)",ic[0].x,ic[0].y,ic[last].x,ic[last].y);
		if (!c)
			return &ic[0];
		if (ic[0].x == c->x && ic[0].y == c->y)
			return &ic[last];
		if (ic[last].x == c->x && ic[last].y == c->y)
			return &ic[0];
	}
	return NULL;
}
#endif

struct associated_street {
	osmid relid;
	char *name;
};

struct associated_street_member_func_priv {
	FILE *out;
	GList *allocations;
};

static void
process_associated_street_member(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv)
{
	struct associated_street_member_func_priv *fp=func_priv;
	struct associated_street *rel=relation_priv;
	if(!fp->out) {
		/* Pass 1, fill associated street names in relation_priv */
		char *name;
		if(!rel->name && item_is_street(*member) && (name=item_bin_get_attr(member,attr_street_name,NULL))!=NULL ) {
			rel->name=g_strdup(name);
			fp->allocations=g_list_prepend(fp->allocations, rel->name);
		}
	} else {
		/* Pass 2, add associated street names to relation members which do not have street name attr defined but 
		   have house number defined or are streets */
		int type_implies_streetname=item_is_street(*member) ||
			member->type==type_house_number_interpolation_even ||
			member->type==type_house_number_interpolation_odd || 
			member->type==type_house_number_interpolation_all ||
			member->type==type_house_number_interpolation_alphabetic;
		if(rel->name && !item_bin_get_attr(member,attr_street_name,NULL) && (type_implies_streetname || item_bin_get_attr(member,attr_house_number,NULL)))
			item_bin_add_attr_string(member, attr_street_name, rel->name);
		item_bin_write(member,fp->out);
	}
}

static void
relation_func_writethrough(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv)
{
	FILE *out=*(FILE **)func_priv;
	if(out)
		item_bin_write(member,out);
}


static void
process_associated_streets_setup(FILE *in, struct relations *relations, struct associated_street_member_func_priv *fp)
{
	struct relation_member relm;
	long long relid;
	struct item_bin *ib;
	struct relations_func *relations_func;
	int min_count;
	
	fseek(in, 0, SEEK_SET);
	relations_func=relations_func_new(process_associated_street_member, fp);
	while ((ib=read_item(in))) {
		char *name=osm_tag_value(ib, "name");
		int namelen=name?strlen(name)+1:0;
		struct associated_street *rel=g_malloc0(sizeof(struct associated_street)+namelen);
		relid=item_bin_get_relationid(ib);
		rel->relid=relid;
		if(name) {
			rel->name=(char*)(rel+1);
			g_strlcpy(rel->name,name,namelen);
		}
		min_count=0;
		while(search_relation_member(ib, "street",&relm,&min_count)) {
			if(relm.type==2)
				relations_add_func(relations, relations_func, rel, NULL, relm.type, relm.id);
		}
		min_count=0;
		while(search_relation_member(ib, "house",&relm,&min_count)) {
			relations_add_func(relations, relations_func, rel, NULL, relm.type, relm.id);
		}
		min_count=0;
		while(search_relation_member(ib, "addr:houselink",&relm,&min_count)) {
			relations_add_func(relations, relations_func, rel, NULL, relm.type, relm.id);
		}
		min_count=0;
		while(search_relation_member(ib, "address",&relm,&min_count)) {
			relations_add_func(relations, relations_func, rel, NULL, relm.type, relm.id);
		}
	}
	relations_func=relations_func_new(relation_func_writethrough, &fp->out);
	relations_add_func(relations, relations_func, NULL, NULL, -1, 0);
}

void
process_associated_streets(FILE *in, FILE *ways_in, FILE *ways_out, FILE *nodes_in, FILE *nodes_out, FILE *nodes2_in, FILE *nodes2_out)
{
	struct relations *relations=relations_new();
	struct associated_street_member_func_priv fp={NULL,NULL};
	fseek(in, 0, SEEK_SET);
 	process_associated_streets_setup(in, relations, &fp);

	/* Set noname relations names from their street members */
	fseek(ways_in, 0, SEEK_SET);
	relations_process(relations, NULL, ways_in, NULL);

	/* Set street names on all members */
	fp.out=ways_out;
	fseek(ways_in, 0, SEEK_SET);
	relations_process(relations, NULL, ways_in, NULL);

	fp.out=nodes_out;
	fseek(nodes_in, 0, SEEK_SET);
	relations_process(relations, NULL, nodes_in, NULL);

	if(nodes2_in) {
		fp.out=nodes2_out;
		fseek(nodes2_in, 0, SEEK_SET);
		relations_process(relations, NULL, nodes2_in, NULL);
	}

	relations_destroy(relations);
	g_list_foreach(fp.allocations, (GFunc)free, NULL);
	g_list_free(fp.allocations);
}


struct turn_restriction {
	osmid relid;
	enum item_type type;
	struct coord *c[3];
	int c_count[3];
	struct rect r;
	int order;
};

static void
process_turn_restrictions_member(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv)
{
	int i,count,type=(long)member_priv;
	struct turn_restriction *turn_restriction=relation_priv;
	struct coord *c=(struct coord *)(member+1);
	int ccount=member->clen/2;

	if (member->type < type_line) 
		count=1;	
	else
		count=2;
	turn_restriction->c[type]=g_renew(struct coord, turn_restriction->c[type], turn_restriction->c_count[type]+count);
	turn_restriction->c[type][turn_restriction->c_count[type]++]=c[0];
	if (count > 1) 
		turn_restriction->c[type][turn_restriction->c_count[type]++]=c[ccount-1];
	if(IS_REF(turn_restriction->r.l)) {
		turn_restriction->r.l=c[0];
		turn_restriction->r.h=c[0];
		i=1;
	} else
		i=0;
	for(;i<ccount;i++)
		bbox_extend(&c[i], &turn_restriction->r);

	i=item_order_by_type(member->type);
	if(i<turn_restriction->order)
		turn_restriction->order=i;

}

static void
process_turn_restrictions_fromto(struct turn_restriction *t, int type, struct coord **c)
{
	int i,j;
	for (i = 0 ; i < t->c_count[type] ; i+=2) {
		for (j = 0 ; j < t->c_count[1] ; j++) {
			if (coord_is_equal(t->c[type][i],t->c[1][j])) {
				c[0]=&t->c[type][i+1];
				c[1]=&t->c[type][i];
				return;
			}
			if (coord_is_equal(t->c[type][i+1],t->c[1][j])) {
				c[0]=&t->c[type][i];
				c[1]=&t->c[type][i+1];
				return;
			}
		}
	}
}

static void
process_turn_restrictions_dump_coord(struct coord *c, int count)
{
	int i;
	for (i = 0 ; i < count ; i++) {
		fprintf(stderr,"(0x%x,0x%x)",c[i].x,c[i].y);
	}
}

static void
process_turn_restrictions_finish(GList *tr, FILE *out)
{
	GList *l=tr;
	while (l) {
		struct turn_restriction *t=l->data;
		struct coord *c[4];
		struct item_bin *ib=item_bin;

		if (!t->c_count[0]) {
			osm_warning("relation",t->relid,0,"turn restriction: from member not found\n");
		} else if (!t->c_count[1]) {
			osm_warning("relation",t->relid,0,"turn restriction: via member not found\n");
		} else if (!t->c_count[2]) {
			osm_warning("relation",t->relid,0,"turn restriction: to member not found\n");
		} else {
			process_turn_restrictions_fromto(t, 0, c);
			process_turn_restrictions_fromto(t, 2, c+2);
			if (!c[0] || !c[2]) {
				osm_warning("relation",t->relid,0,"turn restriction: via (");
				process_turn_restrictions_dump_coord(t->c[1], t->c_count[1]);
				fprintf(stderr,")");
				if (!c[0]) {
					fprintf(stderr," failed to connect to from (");
					process_turn_restrictions_dump_coord(t->c[0], t->c_count[0]);
					fprintf(stderr,")");
				}
				if (!c[2]) {
					fprintf(stderr," failed to connect to to (");
					process_turn_restrictions_dump_coord(t->c[2], t->c_count[2]);
					fprintf(stderr,")");
				}
				fprintf(stderr,"\n");
			} else {
				if (t->c_count[1] <= 2) {
					int order;
					char tilebuf[20]="";
					item_bin_init(ib,t->type);
					item_bin_add_coord(ib, c[0], 1);
					item_bin_add_coord(ib, c[1], 1);
					if (t->c_count[1] > 1) 
						item_bin_add_coord(ib, c[3], 1);
					item_bin_add_coord(ib, c[2], 1);

					order=tile(&t->r,"",tilebuf,sizeof(tilebuf)-1,overlap,NULL);
					if(order > t->order)
						order=t->order;
					item_bin_add_attr_range(ib,attr_order,0,order);
					
					item_bin_write(ib, out);
				}
				
			}
		}
		g_free(t);
		l=g_list_next(l);
	}
	g_list_free(tr);
}

static GList *
process_turn_restrictions_setup(FILE *in, struct relations *relations)
{
	struct relation_member fromm,tom,viam,tmpm;
	long long relid;
	struct item_bin *ib;
	struct relations_func *relations_func;
	int min_count;
	GList *turn_restrictions=NULL;
	
	fseek(in, 0, SEEK_SET);
	relations_func=relations_func_new(process_turn_restrictions_member, NULL);
	while ((ib=read_item(in))) {
		struct turn_restriction *turn_restriction;
		relid=item_bin_get_relationid(ib);
		min_count=0;
		if (!search_relation_member(ib, "from",&fromm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: from member missing\n");
			continue;
		}
		if (search_relation_member(ib, "from",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple from members\n");
			continue;
		}
		min_count=0;
		if (!search_relation_member(ib, "to",&tom,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: to member missing\n");
			continue;
		}
		if (search_relation_member(ib, "to",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple to members\n");
			continue;
		}
		min_count=0;
		if (!search_relation_member(ib, "via",&viam,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: via member missing\n");
			continue;
		}
		if (search_relation_member(ib, "via",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple via member\n");
			continue;
		}
		if (fromm.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for from member ");
			osm_warning(osm_types[fromm.type],fromm.id,1,"\n");
			continue;
		}
		if (tom.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for to member ");
			osm_warning(osm_types[tom.type],tom.id,1,"\n");
			continue;
		}
		if (viam.type != 1 && viam.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for via member ");
			osm_warning(osm_types[viam.type],viam.id,1,"\n");
			continue;
		}
		turn_restriction=g_new0(struct turn_restriction, 1);
		turn_restriction->relid=relid;
		turn_restriction->type=ib->type;
		turn_restriction->r.l.x=1<<30;
		turn_restriction->order=255;
		relations_add_func(relations, relations_func, turn_restriction, (gpointer) 0, fromm.type, fromm.id);
		relations_add_func(relations, relations_func, turn_restriction, (gpointer) 1, viam.type, viam.id);
		relations_add_func(relations, relations_func, turn_restriction, (gpointer) 2, tom.type, tom.id);
		turn_restrictions=g_list_append(turn_restrictions, turn_restriction);
	}
	return turn_restrictions;
}

void
process_turn_restrictions(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out)
{
	struct relations *relations=relations_new();
	GList *turn_restrictions;
	fseek(in, 0, SEEK_SET);
	turn_restrictions=process_turn_restrictions_setup(in, relations);
	relations_process(relations, coords, ways, NULL);
	process_turn_restrictions_finish(turn_restrictions, out);
	relations_destroy(relations);
}
#if 0
void
process_turn_restrictions_old(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out)
{
	struct relation_member fromm,tom,viam,tmpm;
	struct node_item ni;
	long long relid;
	char from_buffer[65536],to_buffer[65536],via_buffer[65536];
	struct item_bin *ib,*from=(struct item_bin *)from_buffer,*to=(struct item_bin *)to_buffer,*via=(struct item_bin *)via_buffer;
	struct coord *fromc,*toc,*viafrom,*viato,*tmp;
	int min_count;
	while ((ib=read_item(in))) {
		relid=item_bin_get_relationid(ib);
		min_count=0;
		if (!search_relation_member(ib, "from",&fromm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: from member missing\n");
			continue;
		}
		if (search_relation_member(ib, "from",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple from members\n");
			continue;
		}
		min_count=0;
		if (!search_relation_member(ib, "to",&tom,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: to member missing\n");
			continue;
		}
		if (search_relation_member(ib, "to",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple to members\n");
			continue;
		}
		min_count=0;
		if (!search_relation_member(ib, "via",&viam,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: via member missing\n");
			continue;
		}
		if (search_relation_member(ib, "via",&tmpm,&min_count)) {
			osm_warning("relation",relid,0,"turn restriction: multiple via member\n");
			continue;
		}
		if (fromm.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for from member ");
			osm_warning(osm_types[fromm.type],fromm.id,1,"\n");
			continue;
		}
		if (tom.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for to member ");
			osm_warning(osm_types[tom.type],tom.id,1,"\n");
			continue;
		}
		if (viam.type != 1 && viam.type != 2) {
			osm_warning("relation",relid,0,"turn restriction: wrong type for via member ");
			osm_warning(osm_types[viam.type],viam.id,1,"\n");
			continue;
		}
		if (viam.type == 1) {
			if (!node_item_get_from_file(coords, viam.id, &ni)) {
				osm_warning("relation",relid,0,"turn restriction: failed to get via member ");
				osm_warning(osm_types[viam.type],viam.id,1,"\n");
				continue;
			}
			viafrom=&ni.c;
			viato=&ni.c;
		} else {
			if (!(viafrom=get_way(ways, ways_index, NULL, viam.id, via, 0))) {
				osm_warning("relation",relid,0,"turn restriction: failed to get first via coordinate from ");
				osm_warning(osm_types[viam.type],viam.id,1,"\n");
				continue;
			}
			if (!(viato=get_way(ways, ways_index, viafrom, viam.id, via, 0))) {
				osm_warning("relation",relid,0,"turn restriction: failed to get last via coordinate from ");
				osm_warning(osm_types[viam.type],viam.id,1,"\n");
				continue;
			}

		}
#if 0
		fprintf(stderr,"via "LONGLONG_FMT" vs %d\n",viam.id, ni.id);
		fprintf(stderr,"coord 0x%x,0x%x\n",ni.c.x,ni.c.y);
		fprintf(stderr,"Lookup "LONGLONG_FMT"\n",fromm.id);
#endif
		if (!(fromc=get_way(ways, ways_index, viafrom, fromm.id, from, 0))) {
			if (viam.type == 1 || !(fromc=get_way(ways, ways_index, viato, fromm.id, from, 0))) {
				osm_warning("relation",relid,0,"turn restriction: failed to connect via ");
				osm_warning(osm_types[viam.type],viam.id,1," 0x%x,0x%x-0x%x,0x%x to from member ",viafrom->x,viafrom->y,viato->x,viato->y);
				osm_warning(osm_types[fromm.type],fromm.id,1," (");
				get_way(ways, ways_index, viafrom, fromm.id, from, 1);
				fprintf(stderr,")\n");
				continue;
			} else {
				tmp=viato;
				viato=viafrom;
				viafrom=tmp;
			}
		}
		if (!(toc=get_way(ways, ways_index, viato, tom.id, to, 0))) {
			osm_warning("relation",relid,0,"turn restriction: failed to connect via ");
			osm_warning(osm_types[viam.type],viam.id,1," 0x%x,0x%x-0x%x,0x%x to to member ",viafrom->x,viafrom->y,viato->x,viato->y);
			osm_warning(osm_types[tom.type],tom.id,1," (");
			get_way(ways, ways_index, viato, tom.id, to, 1);
			fprintf(stderr,")\n");
			continue;
		}
#if 0
		fprintf(stderr,"(0x%x,0x%x)-(0x%x,0x%x)-(0x%x,0x%x)\n",fromc->x,fromc->y, ni.c.x, ni.c.y, toc->x, toc->y);
#endif
		item_bin_init(ib,ib->type);
		item_bin_add_coord(ib, fromc, 1);
		item_bin_add_coord(ib, viafrom, 1);
		if (viam.type == 2)
			item_bin_add_coord(ib, viato, 1);
		item_bin_add_coord(ib, toc, 1);
		item_bin_write(item_bin, out);
	}
}
#endif

#if 0
static void
process_countries(FILE *way, FILE *ways_index)
{
	FILE *in=fopen("country_de.tmp","r");
	struct item_bin *ib;
	char buffer2[400000];
	struct item_bin *ib2=(struct item_bin *)buffer2;
	GList *segments=NULL,*sort_segments;
	fseek(in, 0, SEEK_SET);
	while ((ib=read_item(in))) {
		char *str=NULL;
		struct relation_member member;
		while ((str=item_bin_get_attr(ib, attr_osm_member, str))) {
			if (!get_relation_member(str, &member))
				break;
			if (member.type == 2) {
				if (!seek_to_way(way, ways_index, member.id)) {
					fprintf(stderr,"not found in index");
					break;
				}
				while (item_bin_read(ib2, way)) {
					if (item_bin_get_wayid(ib2) != member.id)
						break;
					segments=g_list_prepend(segments,item_bin_to_poly_segment(ib2, geom_poly_segment_type_way_unknown));
					break;
				}
			}
		}
	}
	sort_segments=geom_poly_segments_sort(segments, geom_poly_segment_type_way_left_side);
	FILE *tmp=fopen("tst.txt","w");
	while (sort_segments) {
		struct geom_poly_segment *seg=sort_segments->data;
		if (!seg) {
			fprintf(stderr,"is null\n");
		} else {
			fprintf(stderr,"segment %p %s area "LONGLONG_FMT"\n",sort_segments,coord_is_equal(*seg->first, *seg->last) ? "closed":"open",geom_poly_area(seg->first,seg->last-seg->first+1));
		}
#if 0
		int count=seg->last-seg->first+1;
		item_bin_init(ib, type_border_country);
		item_bin_add_coord(ib, seg->first, count);
		item_bin_dump(ib, tmp);
#endif

		sort_segments=g_list_next(sort_segments);
	}
	fclose(tmp);
	fclose(in);
}
#endif

static void
node_ref_way(osmid node)
{
	struct node_item *ni;
	ni=node_item_get(node);
	if (ni)
		ni->ref_way++;
}

static void
nodes_ref_item_bin(struct item_bin *ib)
{
	int i;
	struct coord *c=(struct coord *)(ib+1);
	for (i = 0 ; i < ib->clen/2 ; i++) 
		node_ref_way(REF(c[i]));
}


void
osm_add_nd(osmid ref)
{
	SET_REF(coord_buffer[coord_count], ref);
	coord_count++;
	if (coord_count > 65536) {
		fprintf(stderr,"ERROR: Overflow\n");
		exit(1);
	}
}

static void
write_item_part(FILE *out, FILE *out_index, FILE *out_graph, struct item_bin *orig, int first, int last, long long *last_id)
{
	struct item_bin new;
	struct coord *c=(struct coord *)(orig+1);
	char *attr=(char *)(c+orig->clen/2);
	int attr_len=orig->len-orig->clen-2;
	processed_ways++;
	new.type=orig->type;
	new.clen=(last-first+1)*2;
	new.len=new.clen+attr_len+2;
	if (out_index) {
		osmid idx[2];
		idx[0]=item_bin_get_wayid(orig);
		idx[1]=ftello(out);
		if (way_hash) {
			if (!(g_hash_table_lookup_extended(way_hash, (gpointer)(long)idx[0], NULL, NULL)))
				g_hash_table_insert(way_hash, (gpointer)(long)idx[0], (gpointer)(long)idx[1]);
		} else {
			if (!last_id || *last_id != idx[0])
				fwrite(idx, sizeof(idx), 1, out_index);
			if (last_id)
				*last_id=idx[0];
		}

	}
#if 0
	fprintf(stderr,"first %d last %d type 0x%x len %d clen %d attr_len %d\n", first, last, new.type, new.len, new.clen, attr_len);
#endif
	fwrite(&new, sizeof(new), 1, out);
	fwrite(c+first, new.clen*4, 1, out);
	fwrite(attr, attr_len*4, 1, out);
#if 0
		fwrite(&new, sizeof(new), 1, out_graph);
		fwrite(c+first, new.clen*4, 1, out_graph);
		fwrite(attr, attr_len*4, 1, out_graph);
#endif
}

void
ref_ways(FILE *in)
{
	struct item_bin *ib;

	fseek(in, 0, SEEK_SET);
	while ((ib=read_item(in))) 
		nodes_ref_item_bin(ib);
}

void
resolve_ways(FILE *in, FILE *out)
{
	struct item_bin *ib;
	struct coord *c;
	int i;
	struct node_item *ni;

	fseek(in, 0, SEEK_SET);
	while ((ib=read_item(in))) {
		c=(struct coord *)(ib+1);
		for (i = 0 ; i < ib->clen/2 ; i++) {
			if(!IS_REF(c[i]))
				continue;
			ni=node_item_get(REF(c[i]));
			if(ni) {
				c[i].x=ni->c.x;
				c[i].y=ni->c.y;
			}
		}
		item_bin_write(ib,out);
	}
}

FILE *
resolve_ways_file(FILE *in, char *suffix, char *filename)
{
	char *newfilename=g_strdup_printf("%s_new",filename);
	FILE *new=tempfile(suffix,newfilename,1);
	resolve_ways(in, new);
	fclose(in);
	fclose(new);
	tempfile_rename(suffix,newfilename,filename);
	g_free(newfilename);
	return tempfile(suffix,filename,0);
}

/**
  * Get POI coordinates from area/line coordinates.
  * @param in *in input file with area/line coordinates.
  * @param in *out output file with POI coordinates
  * @param in type input file original contents type: type_line or type_area
  * @returns nothing
  */
void
process_way2poi(FILE *in, FILE *out, int type)
{
	struct item_bin *ib;
	while ((ib=read_item(in))) {
		int count=ib->clen/2;
		if(count>1 && ib->type<type_line) {
			struct coord *c=(struct coord *)(ib+1), c1, c2;
			int done=0;
			if(type==type_area) {
				if(count<3) {
					osm_warning("way",item_bin_get_wayid(ib),0,"Broken polygon, less than 3 points defined\n");
				}  else if(!geom_poly_centroid(c, count, &c1)) {
					osm_warning("way",item_bin_get_wayid(ib),0,"Broken polygon, area is 0\n");
				} else {
					if(geom_poly_point_inside(c, count, &c1)) {
						c[0]=c1;
					} else {
						geom_poly_closest_point(c, count, &c1, &c2);
						c[0]=c2;
					}
					done=1;
				}
			}
			if(!done) {
				geom_line_middle(c,count,&c1);
				c[0]=c1;
			}
			write_item_part(out, NULL, NULL, ib, 0, 0, NULL);
		}
	}
}


int
map_find_intersections(FILE *in, FILE *out, FILE *out_index, FILE *out_graph, FILE *out_coastline, int final)
{
	struct coord *c;
	int i,ccount,last,remaining;
	osmid ndref;
	struct item_bin *ib;
	struct node_item *ni;
	long long last_id=0;

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	while ((ib=read_item(in))) {
#if 0
		fprintf(stderr,"type 0x%x len %d clen %d\n", ib->type, ib->len, ib->clen);
#endif
		ccount=ib->clen/2;
		if (ccount <= 1)
			continue;
		c=(struct coord *)(ib+1);
		last=0;
		for (i = 0 ; i < ccount ; i++) {
			if (IS_REF(c[i])) {
				ndref=REF(c[i]);
				ni=node_item_get(ndref);
				if (ni) {
					c[i]=ni->c;
					if (ni->ref_way > 1 && i != 0 && i != ccount-1 && i != last && item_get_default_flags(ib->type)) {
						write_item_part(out, out_index, out_graph, ib, last, i, &last_id);
						last=i;
					}
				} else if (final) {
					osm_warning("way",item_bin_get_wayid(ib),0,"Non-existing reference to ");
					osm_warning("node",ndref,1,"\n");
					remaining=(ib->len+1)*4-sizeof(struct item_bin)-i*sizeof(struct coord);
					memmove(&c[i], &c[i+1], remaining);
					ib->clen-=2;
					ib->len-=2;
					i--;
					ccount--;
				}
			}
		}
		if (ccount) {
			write_item_part(out, out_index, out_graph, ib, last, ccount-1, &last_id);
			if (final && ib->type == type_water_line && out_coastline) {
				write_item_part(out_coastline, NULL, NULL, ib, last, ccount-1, NULL);
			}
		}
	}
	sig_alrm(0);
	sig_alrm_end();
	return 0;
}

static void
index_country_add(struct zip_info *info, int country_id, char*first_key, char *last_key, char *tile, char *filename, int size, FILE *out)
{
	struct item_bin *item_bin=init_item(type_countryindex);
	int num=0, zip_num;
	char tilename[32];

	do {
		snprintf(tilename,sizeof(tilename),"%ss%d", tile, num);
		num++;
		zip_num=add_aux_tile(info, tilename, filename, size);
	} while (zip_num == -1);
	
	item_bin_add_attr_int(item_bin, attr_country_id, country_id);

	if(first_key)
		item_bin_add_attr_string(item_bin, attr_first_key, first_key);

	if(last_key)
		item_bin_add_attr_string(item_bin, attr_last_key, last_key);

	item_bin_add_attr_int(item_bin, attr_zipfile_ref, zip_num);
	item_bin_write(item_bin, out);
}

void
write_countrydir(struct zip_info *zip_info, int max_index_size)
{
	int i;
	int max=11;
	char filename[32];
	struct country_table *co;
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if(co->size) { 
			FILE *in;
			char countrypart[32];
			char partsuffix[32];
			FILE *out=NULL;
			char *outname=NULL;
			int partsize;
			char buffer[50000];
			struct item_bin *ib=(struct item_bin*)buffer;
			int ibsize;
			char tileco[32]="";
			char tileprev[32]="";
			char tilecur[32]="";
			char *countryindexname;
			FILE *countryindex;
			char key[1024]="",first_key[1024]="",last_key[1024]="";
			
			tile(&co->r, "", tileco, max, overlap, NULL);
			
			snprintf(filename,sizeof(filename),"country_%d.tmp", co->countryid);
			in=fopen(filename,"rb");

			snprintf(countrypart,sizeof(countrypart),"country_%d_p",co->countryid);

			countryindex=tempfile("0",countrypart,1);
			countryindexname=tempfile_name("0",countrypart);
			
			partsize=0;

			while(1) {
				int r=item_bin_read(ib,in);
				struct attr_bin *a;
				ibsize=r>0?(ib->len+1)*4 : 0;
				if(ibsize) {
					g_strlcpy(tileprev,tilecur,sizeof(tileprev));
					a=item_bin_get_attr_bin(ib, attr_tile_name, NULL);
					if(a) {
						g_strlcpy(tilecur,(char *)(a+1),sizeof(tilecur));
						item_bin_remove_attr(ib,a+1);
					}
					else
						tilecur[0]=0;

					a=item_bin_get_attr_bin_last(ib);
					if(a && ATTR_IS_STRING(a->type))
						g_strlcpy(key,(char *)(a+1),sizeof(key));
				}

				/* If output file is already opened, and:
				     - we have reached end of input file, or 
				     - adding new tile would make index part too big, or
				     - item just read belongs to a different tile than the previous one,
				    then close existing output file, put reference to the country index tile.*/
				if(out && (!r || (partsize && ((partsize+ibsize)>max_index_size)) || strcmp(tileprev,tilecur)) ) {
					partsize=ftello(out);
					fclose(out);
					out=NULL;
					index_country_add(zip_info,co->countryid,first_key,last_key,strlen(tileco)>strlen(tileprev)?tileco:tileprev,outname,partsize,countryindex);
					g_free(outname);
					outname=NULL;
					g_strlcpy(first_key,key,sizeof(first_key));
				}

				/* No items left, finish this country index. */
				if(!r)
					break;

				/* Open new output file. */
				if(!out) {
					co->nparts++;
					snprintf(partsuffix,sizeof(partsuffix),"%d",co->nparts);
					out=tempfile(partsuffix,countrypart,1);
					outname=tempfile_name(partsuffix,countrypart);
					partsize=0;
				}

				item_bin_write(ib,out);
				partsize+=ibsize;
				g_strlcpy(last_key,key,sizeof(last_key));
			}
			
			partsize=ftello(countryindex);
			if(partsize)
				index_country_add(zip_info,co->countryid,NULL,NULL,tileco,countryindexname, partsize, zip_get_index(zip_info));
			fclose(countryindex);
			g_free(countryindexname);
		}
	}
}

void
load_countries(void)
{
	char filename[32];
	FILE *f;
	int i;
	struct country_table *co;

	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		sprintf(filename,"country_%d.tmp", co->countryid);
		f=fopen(filename,"rb");
		if (f) {
			int i,first=1;
			struct item_bin *ib;
			while ((ib=read_item(f))) {
				struct coord *c=(struct coord *)(ib+1);
				co->size+=ib->len*4+4;
				for (i = 0 ; i < ib->clen/2 ; i++) {
					if (first) {
						co->r.l=c[i];
						co->r.h=c[i];
						first=0;
					} else
						bbox_extend(&c[i], &co->r);
				}
			}
			fseek(f, 0, SEEK_END);
			co->size=ftello(f);
			fclose(f);
		}
	}
}

void
remove_countryfiles(void)
{
	int i,j;
	char filename[32];
	struct country_table *co;

	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if (co->size) {
			sprintf(filename,"country_%d.tmp", co->countryid);
			unlink(filename);
		}
		for(j=0; j<=co->nparts;j++) {
			char partsuffix[32];
			sprintf(filename,"country_%d_p", co->countryid);
			sprintf(partsuffix,"%d",j);
			tempfile_unlink(partsuffix,filename);
		}
	}
}

void osm_init(FILE* rule_file)
{
	build_attrmap(rule_file);
	build_countrytable();
}

