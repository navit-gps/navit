#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "maptool.h"
#include "debug.h"
#include "linguistics.h"
#include "file.h"
#ifdef HAVE_POSTGRESQL
#include <libpq-fe.h>
#endif


typedef long int osmid;

static int in_way, in_node, in_relation;
static int nodeid,wayid;
long long current_id;

static GHashTable *attr_hash,*country_table_hash,*attr_hash;

static char *attr_present;
static int attr_present_count;

static struct item_bin item;


int maxspeed_attr_value;

char debug_attr_buffer[BUFFER_SIZE];

int flags[4];

int flags_attr_value;

struct attr_bin osmid_attr;
long int osmid_attr_value;

char is_in_buffer[BUFFER_SIZE];

char attr_strings_buffer[BUFFER_SIZE*16];
int attr_strings_buffer_len;


struct coord coord_buffer[65536];

struct attr_mapping {
	enum item_type type;
	int attr_present_idx_count;
	int attr_present_idx[0];	
};

static struct attr_mapping **attr_mapping_node;
static int attr_mapping_node_count;
static struct attr_mapping **attr_mapping_way;
static int attr_mapping_way_count;

static char *attr_present;
static int attr_present_count;



enum attr_strings {
	attr_string_phone,
	attr_string_fax,
	attr_string_email,
	attr_string_url,
	attr_string_street_name,
	attr_string_street_name_systematic,
	attr_string_house_number,
	attr_string_label,
	attr_string_postal,
	attr_string_population,
	attr_string_last,
};

char *attr_strings[attr_string_last];

char *osm_types[]={"unknown","node","way","relation"};

#define IS_REF(c) ((c).x >= (1 << 30))
#define REF(c) ((c).y)
#define SET_REF(c,ref) do { (c).x = 1 << 30; (c).y = ref ; } while(0)

struct country_table {
	int countryid;
	char *names;
	FILE *file;
	int size;
	struct rect r;
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
	{ 40,"Austria,Ãsterreich,AUT"},
	{ 44,"Bahamas"},
	{ 48,"Bahrain"},
	{ 50,"Bangladesh"},
	{ 51,"Armenia"},
	{ 52,"Barbados"},
	{ 56,"Belgium"},
	{ 60,"Bermuda"},
	{ 64,"Bhutan"},
	{ 68,"Bolivia, Plurinational State of"},
	{ 70,"Bosnia and Herzegovina,Bosna i Hercegovina,ÐÐ¾ÑÐ½Ð° Ð¸ Ð¥ÐµÑÑÐµÐ³Ð¾Ð²Ð¸Ð½Ð°"},
	{ 72,"Botswana"},
	{ 74,"Bouvet Island"},
	{ 76,"Brazil"},
	{ 84,"Belize"},
	{ 86,"British Indian Ocean Territory"},
	{ 90,"Solomon Islands"},
	{ 92,"Virgin Islands, British"},
	{ 96,"Brunei Darussalam"},
	{ 100,"Bulgaria,ÐÑÐ»Ð³Ð°ÑÐ¸Ñ"},
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
	{ 203,"Czech Republic,ÄeskÃ¡ republika,CZ"},
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
	{ 234,"Faroe Islands,FÃ¸royar"},
	{ 238,"Falkland Islands (Malvinas)"},
	{ 239,"South Georgia and the South Sandwich Islands"},
	{ 242,"Fiji"},
 	{ 246,"Finland,Suomi"},
	{ 248,"Åland Islands"},
 	{ 250,"France,RÃ©publique franÃ§aise,FR"},
	{ 254,"French Guiana"},
	{ 258,"French Polynesia"},
	{ 260,"French Southern Territories"},
	{ 262,"Djibouti"},
	{ 266,"Gabon"},
	{ 268,"Georgia"},
	{ 270,"Gambia"},
	{ 275,"Palestinian Territory, Occupied"},
	{ 276,"Germany,Deutschland,Bundesrepublik Deutschland"},
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
	{ 499,"Montenegro,Ð¦ÑÐ½Ð° ÐÐ¾ÑÐ°,Crna Gora"},
	{ 500,"Montserrat"},
	{ 504,"Morocco"},
	{ 508,"Mozambique"},
	{ 512,"Oman"},
	{ 516,"Namibia"},
	{ 520,"Nauru"},
	{ 524,"Nepal"},
	{ 528,"Nederland,The Netherlands,Niederlande,NL"},
	{ 530,"Netherlands Antilles"},
	{ 533,"Aruba"},
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
	{ 642,"RomÃ¢nia,Romania,RO"},
	{ 643,"Ð Ð¾ÑÑÐ¸Ñ,Ð Ð¾ÑÑÐ¸Ð¹ÑÐºÐ°Ñ Ð¤ÐµÐ´ÐµÑÐ°ÑÐ¸Ñ,Russia,Russian Federation"},
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
	{ 688,"Srbija,Ð¡ÑÐ±Ð¸ÑÐ°,Serbia"},
	{ 690,"Seychelles"},
	{ 694,"Sierra Leone"},
	{ 702,"Singapore"},
	{ 703,"Slovakia,Slovensko,SK"},
	{ 704,"Viet Nam"},
	{ 705,"Slovenia,Republika Slovenija,SI"},
	{ 706,"Somalia"},
	{ 710,"South Africa"},
	{ 716,"Zimbabwe"},
 	{ 724,"Spain,Espana,EspaÃ±a,Reino de Espana,Reino de EspaÃ±a"},
	{ 732,"Western Sahara"},
	{ 736,"Sudan"},
	{ 740,"Suriname"},
	{ 744,"Svalbard and Jan Mayen"},
	{ 748,"Swaziland"},
	{ 752,"Sweden,Sverige,Konungariket Sverige,SE"},
	{ 756,"Switzerland,Schweiz"}, 
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
	{ 804,"Ukraine"},
	{ 807,"Macedonia,ÐÐ°ÐºÐµÐ´Ð¾Ð½Ð¸ÑÐ°"},
	{ 818,"Egypt"},
	{ 826,"United Kingdom,UK"},
	{ 831,"Guernsey"},
	{ 832,"Jersey"},
	{ 833,"Isle of Man"},
	{ 834,"Tanzania, United Republic of"},
	{ 840,"USA"},
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

static char *attrmap={
	"n	*=*			point_unkn\n"
	"n	Annehmlichkeit=Hochsitz	poi_hunting_stand\n"
	"n	addr:housenumber=*	house_number\n"
	"n	aeroway=aerodrome	poi_airport\n"
	"n	aeroway=airport		poi_airport\n"
	"n	aeroway=helipad		poi_heliport\n"
	"n	aeroway=terminal	poi_airport\n"
	"n	amenity=atm		poi_bank\n"
	"n	amenity=bank		poi_bank\n"
	"n	amenity=bench		poi_bench\n"
	"n	amenity=biergarten	poi_biergarten\n"
	"n	amenity=bus_station	poi_bus_station\n"
	"n	amenity=cafe		poi_cafe\n"
	"n	amenity=cinema		poi_cinema\n"
	"n	amenity=college		poi_school_college\n"
	"n	amenity=courthouse	poi_justice\n"
	"n	amenity=drinking_water	poi_potable_water\n"
	"n	amenity=fast_food	poi_fastfood\n"
	"n	amenity=fire_station	poi_firebrigade\n"
	"n	amenity=fountain	poi_fountain\n"
	"n	amenity=fuel		poi_fuel\n"
	"n	amenity=grave_yard	poi_cemetery\n"
	"n	amenity=hospital	poi_hospital\n"
	"n	amenity=hunting_stand	poi_hunting_stand\n"
	"n	amenity=kindergarten	poi_kindergarten\n"
	"n	amenity=library		poi_library\n"
	"n	amenity=nightclub	poi_nightclub\n"
	"n	amenity=park_bench	poi_bench\n"
	"n	amenity=parking		poi_car_parking\n"
	"n	amenity=pharmacy	poi_pharmacy\n"
	"n	amenity=place_of_worship,religion=christian	poi_church\n"
	"n	amenity=police		poi_police\n"
	"n	amenity=post_box	poi_post_box\n"
	"n	amenity=post_office	poi_post_office\n"
	"n	amenity=prison		poi_prison\n"
	"n	amenity=pub		poi_bar\n"
	"n	amenity=public_building	poi_public_office\n"
	"n	amenity=recycling	poi_recycling\n"
	"n	amenity=restaurant	poi_restaurant\n"
	"n	amenity=school		poi_school\n"
	"n	amenity=shelter		poi_shelter\n"
	"n	amenity=taxi		poi_taxi\n"
	"n	amenity=tec_common	tec_common\n"
	"n	amenity=telephone	poi_telephone\n"
	"n	amenity=theatre		poi_theater\n"
	"n	amenity=toilets		poi_restroom\n"
	"n	amenity=toilets		poi_toilets\n"
	"n	amenity=townhall	poi_townhall\n"
	"n	amenity=university	poi_school_university\n"
	"n	amenity=vending_machine	poi_vending_machine\n"
	"n	barrier=bollard		barrier_bollard\n"
	"n	barrier=cycle_barrier	barrier_cycle\n"	
	"n	barrier=lift_gate	barrier_lift_gate\n"
	"n	car=car_rental		poi_car_rent\n"
	"n	highway=bus_station	poi_bus_station\n"
	"n	highway=bus_stop	poi_bus_stop\n"
	"n	highway=mini_roundabout	mini_roundabout\n"
	"n	highway=motorway_junction	highway_exit\n"
	"n	highway=stop		traffic_sign_stop\n"
	"n	highway=traffic_signals	traffic_signals\n"
	"n	highway=turning_circle	turning_circle\n"
	"n	historic=boundary_stone	poi_boundary_stone\n"
	"n	historic=castle		poi_castle\n"
	"n	historic=memorial	poi_memorial\n"
	"n	historic=monument	poi_monument\n"
	"n	historic=*		poi_ruins\n"
	"n	landuse=cemetery	poi_cemetery\n"
	"n	leisure=fishing		poi_fish\n"
	"n	leisure=golf_course	poi_golf\n"
	"n	leisure=marina		poi_marine\n"
	"n	leisure=playground	poi_playground\n"
	"n	leisure=slipway		poi_boat_ramp\n"
	"n	leisure=sports_centre	poi_sport\n"
	"n	leisure=stadium		poi_stadium\n"
	"n	man_made=tower		poi_tower\n"
	"n	military=airfield	poi_military\n"
	"n	military=barracks	poi_military\n"
	"n	military=bunker		poi_military\n"
	"n	military=danger_area	poi_danger_area\n"
	"n	military=range		poi_military\n"
	"n	natural=bay		poi_bay\n"
	"n	natural=peak		poi_peak\n"
	"n	natural=tree		poi_tree\n"
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
	"n	railway=station		poi_rail_station\n"
	"n	railway=tram_stop	poi_rail_tram_stop\n"
	"n	shop=baker		poi_shop_baker\n"
	"n	shop=bakery		poi_shop_baker\n"
	"n	shop=beverages		poi_shop_beverages\n"
	"n	shop=bicycle		poi_shop_bicycle\n"
	"n	shop=butcher		poi_shop_butcher\n"
	"n	shop=car		poi_car_dealer_parts\n"
	"n	shop=car_repair		poi_repair_service\n"
	"n	shop=clothes		poi_shop_apparel\n"
	"n	shop=convenience	poi_shop_grocery\n"
	"n	shop=drogist		poi_shop_drugstore\n"
	"n	shop=florist		poi_shop_florist\n"
	"n	shop=fruit		poi_shop_fruit\n"
	"n	shop=furniture		poi_shop_furniture\n"
	"n	shop=garden_centre	poi_shop_handg\n"
	"n	shop=hardware		poi_shop_handg\n"
	"n	shop=hairdresser	poi_hairdresser\n"
	"n	shop=kiosk		poi_shop_kiosk\n"
	"n	shop=optician		poi_shop_optician\n"
	"n	shop=parfum		poi_shop_parfum\n"
	"n	shop=photo		poi_shop_photo\n"
	"n	shop=shoes		poi_shop_shoes\n"
	"n	shop=supermarket	poi_shopping\n"
	"n	sport=baseball		poi_baseball\n"
	"n	sport=basketball	poi_basketball\n"
	"n	sport=climbing		poi_climbing\n"
	"n	sport=golf		poi_golf\n"
	"n	sport=motor_sports	poi_motor_sport\n"
	"n	sport=skiing		poi_skiing\n"
	"n	sport=soccer		poi_soccer\n"
	"n	sport=stadium		poi_stadium\n"
	"n	sport=swimming		poi_swimming\n"
	"n	sport=tennis		poi_tennis\n"
	"n	tourism=attraction	poi_attraction\n"
	"n	tourism=camp_site	poi_camp_rv\n"
	"n	tourism=caravan_site	poi_camp_rv\n"
	"n	tourism=guest_house	poi_guesthouse\n"
	"n	tourism=hostel		poi_hostel\n"
	"n	tourism=hotel		poi_hotel\n"
	"n	tourism=information	poi_information\n"
	"n	tourism=motel		poi_motel\n"
	"n	tourism=museum		poi_museum_history\n"
	"n	tourism=picnic_site	poi_picnic\n"
	"n	tourism=theme_park	poi_resort\n"
	"n	tourism=viewpoint	poi_viewpoint\n"
	"n	tourism=zoo		poi_zoo\n"
	"n	traffic_sign=city_limit	traffic_sign_city_limit\n"
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
	"w	boundary=administrative	border_country\n"
	"w	boundary=civil		border_civil\n"
	"w	boundary=national_park	border_national_park\n"
	"w	boundary=political	border_political\n"
	"w	building=*		poly_building\n"
	"w	contour_ext=elevation_major	height_line_1\n"
	"w	contour_ext=elevation_medium	height_line_2\n"
	"w	contour_ext=elevation_minor	height_line_3\n"
#if 0 /* FIXME: Implement this as attribute */
	"w	cycleway=track	        cycleway\n"
#endif
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
	"w	highway=path,foot=designated		footway\n"
	"w	highway=path,horse=designated		bridleway\n"
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
	"w	highway=trunk				street_4_land\n"
	"w	highway=trunk,name=*,rural=1		street_4_land\n"
	"w	highway=trunk,name=*			street_4_city\n"
	"w	highway=trunk,rural=0			street_4_city\n"
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
	"w	historic=town gate	poly_building\n"
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
	"w	place=village		poly_place3\n"
	"w	place=municipality	poly_place4\n"
	"w	place=town		poly_place5\n"
	"w	place=city		poly_place6\n"
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
	struct attr_mapping ***attr_mapping_curr,*attr_mapping=g_malloc0(sizeof(struct attr_mapping));
	int idx,attr_mapping_count=0,*attr_mapping_curr_count;
	t=line;
	p=strchr(t,'\t');
	if (p) {
		while (*p == '\t')
			*p++='\0';
		kvl=p;
		p=strchr(kvl,'\t');
	}
	if (p) {
		while (*p == '\t')
			*p++='\0';
		i=p;
	}
	if (t[0] == 'w') {
		if (! i)
			i="street_unkn";
		attr_mapping_curr=&attr_mapping_way;
		attr_mapping_curr_count=&attr_mapping_way_count;
	} else {
		if (! i)
			i="point_unkn";
		attr_mapping_curr=&attr_mapping_node;
		attr_mapping_curr_count=&attr_mapping_node_count;
	}
	attr_mapping->type=item_from_name(i);
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
	*attr_mapping_curr=g_realloc(*attr_mapping_curr, sizeof(**attr_mapping_curr)*(*attr_mapping_curr_count+1));
	(*attr_mapping_curr)[(*attr_mapping_curr_count)++]=attr_mapping;
}

static void
build_attrmap(void)
{
	char *p,*map=g_strdup(attrmap);
	attr_hash=g_hash_table_new(g_str_hash, g_str_equal);
	attr_present_count=1;
	while (map) {
		p=strchr(map,'\n');
		if (p)
			*p++='\0';
		if (strlen(map))
			build_attrmap_line(map);
		map=p;
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
static void
osm_warning(char *type, long long id, int cont, char *fmt, ...)
{
	char str[4096];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	fprintf(stderr,"%shttp://www.openstreetmap.org/browse/%s/"LONGLONG_FMT" %s",cont ? "":"OSM Warning:",type,id,str);
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

static long long
item_bin_get_nodeid(struct item_bin *ib)
{
	long long *ret=item_bin_get_attr(ib, attr_osm_nodeid, NULL);
	if (ret)
		return *ret;
	return 0;
}

static long long
item_bin_get_wayid(struct item_bin *ib)
{
	long long *ret=item_bin_get_attr(ib, attr_osm_wayid, NULL);
	if (ret)
		return *ret;
	return 0;
}

static long long
item_bin_get_relationid(struct item_bin *ib)
{
	long long *ret=item_bin_get_attr(ib, attr_osm_relationid, NULL);
	if (ret)
		return *ret;
	return 0;
}

long long
item_bin_get_id(struct item_bin *ib)
{
	long long ret;
	if (ib->type < 0x80000000)
		return item_bin_get_nodeid(ib);
	ret=item_bin_get_wayid(ib);
	if (!ret)
		ret=item_bin_get_relationid(ib);
	return ret;
}

static int
xml_get_attribute(char *xml, char *attribute, char *buffer, int buffer_size)
{
	int len=strlen(attribute);
	char *pos,*i,s,attr[len+2];
	strcpy(attr, attribute);
	strcpy(attr+len, "=");
	pos=strstr(xml, attr);
	if (! pos)
		return 0;
	pos+=len+1;
	s=*pos++;
	if (! s)
		return 0;
	i=strchr(pos, s);
	if (! i)
		return 0;
	if (i - pos > buffer_size) {
		fprintf(stderr,"Buffer overflow %ld vs %d\n", (long)(i-pos), buffer_size);
		return 0;
	}
	strncpy(buffer, pos, i-pos);
	buffer[i-pos]='\0';
	return 1;
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
add_tag(char *k, char *v)
{
	int idx,level=2;
	char buffer[BUFFER_SIZE*2+2];
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
	if (! strcmp(k,"access")) {
		flags[access_value(v)] |= AF_DANGEROUS_GOODS|AF_EMERGENCY_VEHICLES|AF_TRANSPORT_TRUCK|AF_DELIVERY_TRUCK|AF_PUBLIC_BUS|AF_TAXI|AF_HIGH_OCCUPANCY_CAR|AF_CAR|AF_MOTORCYCLE|AF_MOPED|AF_HORSE|AF_BIKE|AF_PEDESTRIAN;
		level=5;
	}
	if (! strcmp(k,"vehicle")) {
		flags[access_value(v)] |= AF_DANGEROUS_GOODS|AF_EMERGENCY_VEHICLES|AF_TRANSPORT_TRUCK|AF_DELIVERY_TRUCK|AF_PUBLIC_BUS|AF_TAXI|AF_HIGH_OCCUPANCY_CAR|AF_CAR|AF_MOTORCYCLE|AF_MOPED|AF_BIKE;
		level=5;
	}
	if (! strcmp(k,"motorvehicle")) {
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
	if (! strcmp(k,"is_in")) {
		strcpy(is_in_buffer, v);
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

	strcpy(buffer,"*=*");
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=1;

	sprintf(buffer,"%s=*", k);
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=2;

	sprintf(buffer,"*=%s", v);
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=2;

	sprintf(buffer,"%s=%s", k, v);
	if ((idx=(int)(long)g_hash_table_lookup(attr_hash, buffer)))
		attr_present[idx]=4;
}

struct entity {
	char *entity;
	char c;
} entities[]= {
	{"&quot;",'"'},
	{"&apos;",'\''},
	{"&amp;",'&'},
	{"&lt;",'<'},
	{"&gt;",'>'},
	{"&#34;",'"'},
	{"&#39;",'\''},
	{"&#38;",'&'},
	{"&#60;",'<'},
	{"&#62;",'>'},
};

static void
decode_entities(char *buffer)
{
	char *pos=buffer;
	int i,len,found;

	while ((pos=strchr(pos, '&'))) {
		found=0;
		for (i = 0 ; i < sizeof(entities)/sizeof(struct entity); i++) {
			len=strlen(entities[i].entity);
			if (!strncmp(pos, entities[i].entity, len)) {
				*pos=entities[i].c;
			 	memmove(pos+1, pos+len, strlen(pos+len)+1);
				found=1;
				break;
			}
		}
		pos++;
	}
}

static int
parse_tag(char *p)
{
	char k_buffer[BUFFER_SIZE];
	char v_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "k", k_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "v", v_buffer, BUFFER_SIZE))
		return 0;
	decode_entities(v_buffer);
	if (in_relation)
		relation_add_tag(k_buffer, v_buffer);
	else
		add_tag(k_buffer, v_buffer);
	return 1;
}


int coord_count;

struct node_item {
	int id;
	char ref_node;
	char ref_way;
	char ref_ref;
	char dummy;
	struct coord c;
};

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

int nodeid_last;
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

static void
add_node(int id, double lat, double lon)
{
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
			fprintf(stderr,"INFO: Nodes out of sequence (new %d vs old %d), adding hash\n", ni->id, nodeid_last);
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

static int
parse_node(char *p)
{
	char id_buffer[BUFFER_SIZE];
	char lat_buffer[BUFFER_SIZE];
	char lon_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "lat", lat_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "lon", lon_buffer, BUFFER_SIZE))
		return 0;
	add_node(atoi(id_buffer), atof(lat_buffer), atof(lon_buffer));
	return 1;
}


static struct node_item *
node_item_get(int id)
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
		i=(int)(long)(g_hash_table_lookup(node_hash, (gpointer)(long)id));
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

static int
node_item_get_from_file(FILE *coords, int id, struct node_item *ret)
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
	count=ftell(coords)/sizeof(struct node_item);
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

static void
add_way(int id)
{
	static int wayid_last;
	wayid=id;
	coord_count=0;
	attr_strings_clear();
	item.type=type_street_unkn;
	debug_attr_buffer[0]='\0';
	maxspeed_attr_value=0;
	flags_attr_value = 0;
	memset(flags, 0, sizeof(flags));
	debug_attr_buffer[0]='\0';
	osmid_attr_value=id;
	if (wayid < wayid_last && !way_hash) {
		fprintf(stderr,"INFO: Ways out of sequence (new %d vs old %d), adding hash\n", wayid, wayid_last);
		way_hash=g_hash_table_new(NULL, NULL);
	}
	wayid_last=wayid;
}

static int
parse_way(char *p)
{
	char id_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	add_way(atoi(id_buffer));
	return 1;
}

static int
add_id_attr(char *p, enum attr_type attr_type)
{
	struct attr idattr = { attr_type };
	char id_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	current_id=atoll(id_buffer);
	idattr.u.num64=&current_id;
	item_bin_add_attr(item_bin, &idattr);
	return 1;
}

char relation_type[BUFFER_SIZE];
char iso_code[BUFFER_SIZE];
int admin_level;
int boundary;


static int
parse_relation(char *p)
{
	debug_attr_buffer[0]='\0';
	relation_type[0]='\0';
	iso_code[0]='\0';
	admin_level=-1;
	boundary=0;
	item_bin_init(item_bin, type_none);
	if (!add_id_attr(p, attr_osm_relationid))
		return 0;
	return 1;
}

static void
end_relation(FILE *turn_restrictions, FILE *boundaries)
{
	if ((!strcmp(relation_type, "multipolygon") || !strcmp(relation_type, "boundary")) && boundary) {
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
		item_bin_write(item_bin, boundaries);
	}

	if (!strcmp(relation_type, "restriction") && (item_bin->type == type_street_turn_restriction_no || item_bin->type == type_street_turn_restriction_only))
		item_bin_write(item_bin, turn_restrictions);
}

static int
parse_member(char *p)
{
	char type_buffer[BUFFER_SIZE];
	char ref_buffer[BUFFER_SIZE];
	char role_buffer[BUFFER_SIZE];
	char member_buffer[BUFFER_SIZE*3+3];
	int type;
	struct attr memberattr = { attr_osm_member };
	if (!xml_get_attribute(p, "type", type_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "ref", ref_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "role", role_buffer, BUFFER_SIZE))
		return 0;
	if (!strcmp(type_buffer,"node")) 
		type=1;
	else if (!strcmp(type_buffer,"way")) 
		type=2;
	else if (!strcmp(type_buffer,"relation")) 
		type=3;
	else {
		fprintf(stderr,"Unknown type %s\n",type_buffer);
		type=0;
	}
	sprintf(member_buffer,"%d:%s:%s", type, ref_buffer, role_buffer);
	memberattr.u.str=member_buffer;
	item_bin_add_attr(item_bin, &memberattr);
	
	return 1;
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
			osm_warning("relation", current_id, 0, "Unknown restriction %s\n",v);
		}
	} else if (!strcmp(k,"admin_level")) {
		admin_level=atoi(v);
	} else if (!strcmp(k,"boundary")) {
		if (!strcmp(v,"administrative")) {
			boundary=1;
		}
	} else if (!strcmp(k,"ISO3166-1")) {
		strcpy(iso_code, v);
	}
#if 0
	if (add_tag) {
		char tag[strlen(k)+strlen(v)+2];
		sprintf(tag,"%s=%s",k,v);
		item_bin_add_attr_string(item_bin, attr_osm_tag, tag);
	}
#endif
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
	memset(attr_present, 0, sizeof(*attr_present)*attr_present_count);
	return ret;
}

static void
end_way(FILE *out)
{
	int i,count;
	int *def_flags,add_flags;
	enum item_type types[10];
	struct item_bin *item_bin;

	if (! out)
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
		fprintf(stderr,"way id %ld\n",osmid_attr_value);	
		dbg_assert(count < 10);
	}
	for (i = 0 ; i < count ; i++) {
		add_flags=0;
		if (types[i] == type_none)
			continue;
		item_bin=init_item(types[i]);
		item_bin_add_coord(item_bin, coord_buffer, coord_count);
		def_flags=item_get_default_flags(types[i]);
		if (def_flags) {
			flags_attr_value=(*def_flags | flags[0] | flags[1]) & ~flags[2];
			if (flags_attr_value != *def_flags)
				add_flags=1;
		}
		item_bin_add_attr_string(item_bin, def_flags ? attr_street_name : attr_label, attr_strings[attr_string_label]);
		item_bin_add_attr_string(item_bin, attr_street_name_systematic, attr_strings[attr_string_street_name_systematic]);
		item_bin_add_attr_longlong(item_bin, attr_osm_wayid, osmid_attr_value);
		if (debug_attr_buffer[0])
			item_bin_add_attr_string(item_bin, attr_debug, debug_attr_buffer);
		if (add_flags) 
			item_bin_add_attr_int(item_bin, attr_flags, flags_attr_value);
		if (maxspeed_attr_value)
			item_bin_add_attr_int(item_bin, attr_maxspeed, maxspeed_attr_value);
		item_bin_write(item_bin,out);
	}
}

static void
end_node(FILE *out)
{
	int conflict,count,i;
	char *postal;
	enum item_type types[10];
	struct country_table *result=NULL, *lookup;
	struct item_bin *item_bin;

	if (!out || ! node_is_tagged || ! nodeid)
		return;
	count=attr_longest_match(attr_mapping_node, attr_mapping_node_count, types, sizeof(types)/sizeof(enum item_type));
	if (!count) {
		types[0]=type_point_unkn;
		count=1;
	}
	dbg_assert(count < 10);
	for (i = 0 ; i < count ; i++) {
		conflict=0;
		if (types[i] == type_none)
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
		item_bin_write(item_bin,out);
		if (item_is_town(*item_bin) && attr_strings[attr_string_label]) {
			char *tok,*buf=is_in_buffer;
			if (!buf[0])
				strcpy(is_in_buffer, "Unknown");
			while ((tok=strtok(buf, ",;"))) {
				while (*tok==' ')
					tok++;
				lookup=g_hash_table_lookup(country_table_hash,tok);
				if (lookup) {
					if (result && result->countryid != lookup->countryid) {
						osm_warning("node",nodeid,0,"conflict for %s %s country %d vs %d\n", attr_strings[attr_string_label], debug_attr_buffer, lookup->countryid, result->countryid);
						conflict=1;
					} else
						result=lookup;
				}
				buf=NULL;
			}
			if (result && !conflict) {
				if (!result->file) {
					char *name=g_strdup_printf("country_%d.bin.unsorted", result->countryid);
					result->file=fopen(name,"wb");
					g_free(name);
				}
				if (result->file) {
					item_bin=init_item(item_bin->type);
					item_bin_add_coord(item_bin, &ni->c, 1);
					item_bin_add_attr_string(item_bin, attr_town_postal, postal);
					item_bin_add_attr_string(item_bin, attr_town_name, attr_strings[attr_string_label]);
					item_bin_write_match(item_bin, attr_town_name, attr_town_name_match, result->file);
				}
			
			}
		}
	}
	processed_nodes_out++;
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
		name_in=g_strdup_printf("country_%d.bin.unsorted", co->countryid);
		name_out=g_strdup_printf("country_%d.bin", co->countryid);
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


static int
seek_to_way(FILE *way, FILE *ways_index, long long wayid)
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
	count=ftell(ways_index)/sizeof(idx);
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

void
process_turn_restrictions(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out)
{
	struct relation_member fromm,tom,viam,tmpm;
	struct node_item ni;
	long long relid;
	char from_buffer[65536],to_buffer[65536],via_buffer[65536];
	struct item_bin *ib,*from=(struct item_bin *)from_buffer,*to=(struct item_bin *)to_buffer,*via=(struct item_bin *)via_buffer;
	struct coord *fromc,*toc,*viafrom,*viato,*tmp;
	fseek(in, 0, SEEK_SET);
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

static void
node_ref_way(osmid node)
{
	struct node_item *ni;
	ni=node_item_get(node);
	if (ni) 
		ni->ref_way++; 
}

int
resolve_ways(FILE *in, FILE *out)
{
	struct item_bin *ib;
	struct coord *c;
	int i;

	fseek(in, 0, SEEK_SET);
	while ((ib=read_item(in))) {
		c=(struct coord *)(ib+1);
		for (i = 0 ; i < ib->clen/2 ; i++) {
			node_ref_way(REF(c[i]));
		}
	}
	return 0;
}

static void
add_nd(char *p, osmid ref)
{
	SET_REF(coord_buffer[coord_count], ref);
	node_ref_way(ref);
	coord_count++;
	if (coord_count > 65536) {
		fprintf(stderr,"ERROR: Overflow\n");
		exit(1);
	}
}

static int
parse_nd(char *p)
{
	char ref_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "ref", ref_buffer, BUFFER_SIZE))
		return 0;
	add_nd(p, atoi(ref_buffer));
	return 1;
}

int
map_collect_data_osm(FILE *in, FILE *out_ways, FILE *out_nodes, FILE *out_turn_restrictions, FILE *out_boundaries)
{
	int size=BUFFER_SIZE;
	char buffer[size];
	char *p;
	sig_alrm(0);
	while (fgets(buffer, size, in)) {
		p=strchr(buffer,'<');
		if (! p) {
			fprintf(stderr,"WARNING: wrong line %s\n", buffer);
			continue;
		}
		if (!strncmp(p, "<?xml ",6)) {
		} else if (!strncmp(p, "<osm ",5)) {
		} else if (!strncmp(p, "<bound ",7)) {
		} else if (!strncmp(p, "<node ",6)) {
			if (!parse_node(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			in_node=1;
			processed_nodes++;
		} else if (!strncmp(p, "<tag ",5)) {
			if (!parse_tag(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<way ",5)) {
			in_way=1;
			if (!parse_way(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_ways++;
		} else if (!strncmp(p, "<nd ",4)) {
			if (!parse_nd(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<relation ",10)) {
			in_relation=1;
			if (!parse_relation(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_relations++;
		} else if (!strncmp(p, "<member ",8)) {
			if (!parse_member(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "</node>",7)) {
			in_node=0;
			end_node(out_nodes);
		} else if (!strncmp(p, "</way>",6)) {
			in_way=0;
			end_way(out_ways);
		} else if (!strncmp(p, "</relation>",11)) {
			in_relation=0;
			end_relation(out_turn_restrictions, out_boundaries);
		} else if (!strncmp(p, "</osm>",6)) {
		} else {
			fprintf(stderr,"WARNING: unknown tag in %s\n", buffer);
		}
	}
	sig_alrm(0);
	sig_alrm_end();
	return 1;
}

#ifdef HAVE_POSTGRESQL
int
map_collect_data_osm_db(char *dbstr, FILE *out_ways, FILE *out_nodes)
{
	PGconn *conn;
	PGresult *res,*node,*way,*tag;
	int count,tagged,i,j,k;
	long min, max, id, tag_id, node_id;
	char query[256];
	
	sig_alrm(0);
	conn=PQconnectdb(dbstr);
	if (! conn) {
		fprintf(stderr,"Failed to connect to database with '%s'\n",dbstr);
		exit(1);
	}
	res=PQexec(conn, "begin");
	if (! res) {
		fprintf(stderr, "Cannot begin transaction: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	res=PQexec(conn, "set transaction isolation level serializable");
	if (! res) {
		fprintf(stderr, "Cannot set isolation level: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	res=PQexec(conn, "declare node cursor for select id,x(coordinate),y(coordinate) from node order by id");
	if (! res) {
		fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	res=PQexec(conn, "declare way cursor for select id from way order by id");
	if (! res) {
		fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	res=PQexec(conn, "declare relation cursor for select id from relation order by id");
	if (! res) {
		fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	for (;;) {
		node=PQexec(conn, "fetch 100000 from node");
		if (! node) {
			fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
			PQclear(node);
			exit(1);
		}
		count=PQntuples(node);
		if (! count)
			break;
		min=atol(PQgetvalue(node, 0, 0));
		max=atol(PQgetvalue(node, count-1, 0));
		sprintf(query,"select node_id,name,value from node_tag where node_id >= %ld and node_id <= %ld order by node_id", min, max);
		tag=PQexec(conn, query);
		if (! tag) {
			fprintf(stderr, "Cannot query node_tag: %s\n", PQerrorMessage(conn));
			exit(1);
		}
		j=0;
		for (i = 0 ; i < count ; i++) {
			id=atol(PQgetvalue(node, i, 0));
			add_node(id, atof(PQgetvalue(node, i, 1)), atof(PQgetvalue(node, i, 2)));
			tagged=0;
			in_node=1;
			processed_nodes++;
			while (j < PQntuples(tag)) {
				tag_id=atol(PQgetvalue(tag, j, 0));
				if (tag_id == id) {
					add_tag(PQgetvalue(tag, j, 1), PQgetvalue(tag, j, 2));
					tagged=1;
					j++;
				}
				if (tag_id < id)
					j++;
				if (tag_id > id)
					break;
			}
			if (tagged)
				end_node(out_nodes);
			in_node=0;
		}
		PQclear(tag);
		PQclear(node);
	}
	for (;;) {
		way=PQexec(conn, "fetch 100000 from way");
		if (! way) {
			fprintf(stderr, "Cannot setup cursor for ways: %s\n", PQerrorMessage(conn));
			PQclear(node);
			exit(1);
		}
		count=PQntuples(way);
		if (! count)
			break;
		min=atol(PQgetvalue(way, 0, 0));
		max=atol(PQgetvalue(way, count-1, 0));
		sprintf(query,"select way_id,node_id from way_node where way_id >= %ld and way_id <= %ld order by way_id,sequence_id", min, max);
		node=PQexec(conn, query);
		if (! node) {
			fprintf(stderr, "Cannot query way_node: %s\n", PQerrorMessage(conn));
			exit(1);
		}
		sprintf(query,"select way_id,name,value from way_tag where way_id >= %ld and way_id <= %ld order by way_id", min, max);
		tag=PQexec(conn, query);
		if (! tag) {
			fprintf(stderr, "Cannot query way_tag: %s\n", PQerrorMessage(conn));
			exit(1);
		}
		j=0;
		k=0;
		for (i = 0 ; i < count ; i++) {
			id=atol(PQgetvalue(way, i, 0));
			add_way(id);
			tagged=0;
			in_way=1;
			processed_ways++;
			while (k < PQntuples(node)) {
				node_id=atol(PQgetvalue(node, k, 0));
				if (node_id == id) {
					add_nd("",atol(PQgetvalue(node, k, 1)));
					tagged=1;
					k++;
				}
				if (node_id < id)
					k++;
				if (node_id > id)
					break;
			}
			while (j < PQntuples(tag)) {
				tag_id=atol(PQgetvalue(tag, j, 0));
				if (tag_id == id) {
					add_tag(PQgetvalue(tag, j, 1), PQgetvalue(tag, j, 2));
					tagged=1;
					j++;
				}
				if (tag_id < id)
					j++;
				if (tag_id > id)
					break;
			}
			if (tagged)
				end_way(out_ways);
			in_way=0;
		}
		PQclear(tag);
		PQclear(node);
		PQclear(way);
	}

	res=PQexec(conn, "commit");
	if (! res) {
		fprintf(stderr, "Cannot commit transaction: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit(1);
	}
	sig_alrm(0);
	sig_alrm_end();
	return 1;
}
#endif


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
		long long idx[2];
		idx[0]=item_bin_get_wayid(orig);
		idx[1]=ftell(out);
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
index_country_add(struct zip_info *info, int country_id, int zipnum)
{
	struct item_bin *item_bin=init_item(type_countryindex);
	item_bin_add_attr_int(item_bin, attr_country_id, country_id);
	item_bin_add_attr_int(item_bin, attr_zipfile_ref, zipnum);
	item_bin_write(item_bin, info->index);
}

void
write_countrydir(struct zip_info *zip_info)
{
	int i,zipnum,num;
	int max=11;
	char tilename[32];
	char filename[32];
	char suffix[32];
	struct country_table *co;
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if (co->size) {
			num=0;
			do {
				tilename[0]='\0';
				sprintf(suffix,"s%d", num);
				num++;
				tile(&co->r, suffix, tilename, max, overlap, NULL);
				sprintf(filename,"country_%d.bin", co->countryid);
				zipnum=add_aux_tile(zip_info, tilename, filename, co->size);
			} while (zipnum == -1);
			index_country_add(zip_info,co->countryid,zipnum);
		}
	}
}

void
remove_countryfiles(void)
{
	int i;
	char filename[32];
	struct country_table *co;

	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if (co->size) {
			sprintf(filename,"country_%d.bin", co->countryid);
			unlink(filename);
		}
	}
}

void osm_init(void)
{
        build_attrmap();
        build_countrytable();
}

