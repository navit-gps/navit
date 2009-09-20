/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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


#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"
#include "item.h"
#include "map.h"
#include "zipfile.h"
#include "main.h"
#include "config.h"
#include "linguistics.h"
#include "plugin.h"
#ifdef HAVE_POSTGRESQL
#include <libpq-fe.h>
#endif

#define BUFFER_SIZE 1280

typedef long int osmid;


#if 1
#define debug_tile(x) 0
#else
#define debug_tile(x) (!strcmp(x,"bcdbd") || !strcmp(x,"bcdbd") || !strcmp(x,"bcdbda") || !strcmp(x,"bcdbdb") || !strcmp(x,"bcdbdba") || !strcmp(x,"bcdbdbb") || !strcmp(x,"bcdbdbba") || !strcmp(x,"bcdbdbaa") || !strcmp(x,"bcdbdbacaa") || !strcmp(x,"bcdbdbacab") || !strcmp(x,"bcdbdbacaba") || !strcmp(x,"bcdbdbacabaa") || !strcmp(x,"bcdbdbacabab") || !strcmp(x,"bcdbdbacababb") || !strcmp(x,"bcdbdbacababba") || !strcmp(x,"bcdbdbacababbb") || !strcmp(x,"bcdbdbacababbd") || !strcmp(x,"bcdbdbacababaa") || !strcmp(x,"bcdbdbacababab") || !strcmp(x,"bcdbdbacababac") || !strcmp(x,"bcdbdbacababad") || !strcmp(x,"bcdbdbacabaaa") || !strcmp(x,"bcdbdbacabaaba") || !strcmp(x,"bcdbdbacabaabb") || !strcmp(x,"bcdbdbacabaabc") || !strcmp(x,"bcdbdbacabaabd") || !strcmp(x,"bcdbdbacabaaaa") || !strcmp(x,"bcdbdbacabaaab") || !strcmp(x,"bcdbdbacabaaac") || !strcmp(x,"bcdbdbacabaaad") || 0)
#endif


static GHashTable *dedupe_ways_hash;

static int attr_debug_level=1;
static int nodeid,wayid;
static int phase;
static int ignore_unkown = 0, coverage=0;

long long slice_size=1024*1024*1024;
long long current_id;
int slices;

char *osm_types[]={"unknown","node","way","relation"};

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
	"n	car=car_rental		poi_car_rent\n"
	"n	highway=bus_station	poi_bus_station\n"
	"n	highway=bus_stop	poi_bus_stop\n"
	"n	highway=mini_roundabout	mini_roundabout\n"
	"n	highway=motorway_junction	highway_exit\n"
	"n	highway=traffic_signals	traffic_signals\n"
	"n	highway=turning_circle	turning_circle\n"
	"n	historic=boundary_stone	poi_boundary_stone\n"
	"n	historic=castle		poi_castle\n"
	"n	historic=memorial	poi_memorial\n"
	"n	historic=monument	poi_monument\n"
	"n	historic=ruins		poi_ruins\n"
	"n	landuse=cemetery	poi_cemetery\n"
	"n	leisure=fishing		poi_fish\n"
	"n	leisure=golf_course	poi_golf\n"
	"n	leisure=marina		poi_marine\n"
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
	"n	place=city		town_label_2e5\n"
	"n	place=hamlet		town_label_2e2\n"
	"n	place=locality		town_label_2e0\n"
	"n	place=suburb		district_label\n"
	"n	place=town		town_label_2e4\n"
	"n	place=village		town_label_2e3\n"
	"n	power=tower		power_tower\n"
	"n	railway=halt		poi_rail_halt\n"
	"n	railway=level_crossing	poi_level_crossing\n"
	"n	railway=station		poi_rail_station\n"
	"n	railway=tram_stop	poi_rail_tram_stop\n"
	"n	shop=baker		poi_shop_baker\n"
	"n	shop=butcher		poi_shop_butcher\n"
	"n	shop=car_repair		poi_repair_service\n"
	"n	shop=convenience	poi_shop_grocery\n"
	"n	shop=kiosk		poi_shop_kiosk\n"
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


static char buffer[400000];
static struct item_bin *item_bin=(struct item_bin *)(void *)buffer;

struct coord coord_buffer[65536];

char *suffix="";

#define IS_REF(c) ((c).x >= (1 << 30))
#define REF(c) ((c).y)
#define SET_REF(c,ref) do { (c).x = 1 << 30; (c).y = ref ; } while(0)

struct rect {
	struct coord l,h;
};

static void bbox_extend(struct coord *c, struct rect *r);

GList *aux_tile_list;

struct country_table {
	int countryid;
	char *names;
	FILE *file;
	int size;
	struct rect r;
} country_table[] = {
	{ 36,"Australia,AUS"},
	{ 40,"Austria,Österreich,AUT"},
	{ 56,"Belgium"},
	{124,"Canada"},
	{152,"Chile"},
	{191,"Croatia,Republika Hrvatska,HR"},
	{203,"Czech Republic,Česká republika,CZ"},
 	{208,"Denmark,Danmark,DK"},
 	{246,"Finland,Suomi"},
 	{250,"France,République française,FR"},
	{276,"Germany,Deutschland,Bundesrepublik Deutschland"},
	{348,"Hungary"},
 	{380,"Italy,Italia"},
	{528,"Nederland,The Netherlands,Niederlande,NL"},
	{578,"Norway,Norge,Noreg,NO"},
	{616,"Poland,Polska,PL"},
	{642,"România,Romania,RO"},
	{703,"Slovakia,Slovensko,SK"},
	{705,"Slovenia,Republika Slovenija,SI"},
 	{724,"Spain,Espana,España,Reino de Espana"},
	{752,"Sweden,Sverige,Konungariket Sverige,SE"},
	{756,"Schweiz"}, 
	{826,"United Kingdom,UK"},
	{840,"USA"},
	{999,"Unknown"},
};

static GHashTable *country_table_hash;

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
static GHashTable *attr_hash;


static GHashTable *strings_hash = NULL;

static void
osm_warning(char *type, long long id, int cont, char *fmt, ...)
{
	char str[4096];
	va_list ap;
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
	fprintf(stderr,"%shttp://www.openstreetmap.org/browse/%s/%Ld %s",cont ? "":"OSM Warning:",type,id,str);
}

static char* string_hash_lookup( const char* key )
{
	char* key_ptr = NULL;

	if ( strings_hash == NULL ) {
		strings_hash = g_hash_table_new(g_str_hash, g_str_equal);
	}

	if ( ( key_ptr = g_hash_table_lookup(strings_hash, key )) == NULL ) {
		key_ptr = g_strdup( key );
		g_hash_table_insert(strings_hash, key_ptr,  (gpointer)key_ptr );

	}
	return key_ptr;
}

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
build_attrmap(char *map)
{
	char *p;
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



static int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;
static int in_way, in_node, in_relation;

static void
sig_alrm(int sig)
{
#ifndef _WIN32
	signal(SIGALRM, sig_alrm);
	alarm(30);
#endif
	fprintf(stderr,"PROGRESS%d: Processed %d nodes (%d out) %d ways %d relations %d tiles\n", phase, processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles);
}

struct item_bin {
	int len;
	enum item_type type;
	int clen;
} item;

struct attr_bin {
	int len;
	enum attr_type type;
};

int maxspeed_attr_value;

char debug_attr_buffer[BUFFER_SIZE];

int flags[4];

int flags_attr_value;

struct attr_bin osmid_attr;
long int osmid_attr_value;

char is_in_buffer[BUFFER_SIZE];

char attr_strings_buffer[BUFFER_SIZE*16];
int attr_strings_buffer_len;

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

static void
item_bin_set_type(struct item_bin *ib, enum item_type type)
{
	ib->type=type;
}

static void
item_bin_init(struct item_bin *ib, enum item_type type)
{
	ib->clen=0;
	ib->len=2;
	item_bin_set_type(ib, type);
}


static void
item_bin_add_coord(struct item_bin *ib, struct coord *c, int count)
{
	struct coord *c2=(struct coord *)(ib+1);
	c2+=ib->clen/2;
	memcpy(c2, c, count*sizeof(struct coord));
	ib->clen+=count*2;
	ib->len+=count*2;
}

static void
item_bin_add_coord_rect(struct item_bin *ib, struct rect *r)
{
	item_bin_add_coord(ib, &r->l, 1);
	item_bin_add_coord(ib, &r->h, 1);
}

static void
item_bin_add_attr(struct item_bin *ib, struct attr *attr)
{
	struct attr_bin *ab=(struct attr_bin *)((int *)ib+ib->len+1);
	int size=attr_data_size(attr);
	int pad=(4-(size%4))%4;
	ab->type=attr->type;
	memcpy(ab+1, attr_data_get(attr), size);
	memset((unsigned char *)(ab+1)+size, 0, pad);
	ab->len=(size+pad)/4+1;
	ib->len+=ab->len+1;
}

static void
item_bin_add_attr_int(struct item_bin *ib, enum attr_type type, int val)
{
	struct attr attr;
	attr.type=type;
	attr.u.num=val;
	item_bin_add_attr(ib, &attr);
}

static void *
item_bin_get_attr(struct item_bin *ib, enum attr_type type, void *last)
{
	unsigned char *s=(unsigned char *)ib;
	unsigned char *e=s+(ib->len+1)*4;
	s+=sizeof(struct item_bin)+ib->clen*4;
	while (s < e) {
		struct attr_bin *ab=(struct attr_bin *)s;
		s+=(ab->len+1)*4;
		if (ab->type == type && (void *)(ab+1) > last) {
			return (ab+1);
		}
	}
	return NULL;
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

static void
item_bin_add_attr_longlong(struct item_bin *ib, enum attr_type type, long long val)
{
	struct attr attr;
	attr.type=type;
	attr.u.num64=&val;
	item_bin_add_attr(ib, &attr);
}

static void
item_bin_add_attr_string(struct item_bin *ib, enum attr_type type, char *str)
{
	struct attr attr;
	if (! str)
		return;
	attr.type=type;
	attr.u.str=str;
	item_bin_add_attr(ib, &attr);
}

static void
item_bin_add_attr_range(struct item_bin *ib, enum attr_type type, short min, short max)
{
	struct attr attr;
	attr.type=type;
	attr.u.range.min=min;
	attr.u.range.max=max;
	item_bin_add_attr(ib, &attr);
}

static void
item_bin_write(struct item_bin *ib, FILE *out)
{
	fwrite(buffer, (ib->len+1)*4, 1, out);
}

static int
item_bin_read(struct item_bin *ib, FILE *in)
{
	if (fread(ib, 4, 1, in) == 0)
		return 0;
	if (!ib->len)
		return 1;
	if (fread((unsigned char *)ib+4, ib->len*4, 1, in))
		return 2;
	return 0;
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
	if (!strcmp(v,"yes"))
		return 1;
	if (!strcmp(v,"designated"))
		return 1;
	if (!strcmp(v,"permissive"))
		return 1;
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
	if (! strcmp(k,"postal")) {
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


struct buffer {
	int malloced_step;
	long long malloced;
	unsigned char *base;
	long long size;
};

static void save_buffer(char *filename, struct buffer *b, long long offset);

static struct tile_head {
	int num_subtiles;
	int total_size;
	char *name;
	char *zip_data;
	int total_size_used;
	int zipnum;
	int process;
	struct tile_head *next;
	// char subtiles[0];
} *tile_head_root;


int coord_count;

struct node_item {
	int id;
	char ref_node;
	char ref_way;
	char ref_ref;
	char dummy;
	struct coord c;
};

static struct buffer node_buffer = {
	64*1024*1024,
};


static char** th_get_subtile( const struct tile_head* th, int idx )
{
	char* subtile_ptr = NULL;
	subtile_ptr = (char*)th + sizeof( struct tile_head ) + idx * sizeof( char *);
	return (char**)subtile_ptr;
}

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

static void
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
	for (;;) {
		fseek(coords, p*sizeof(struct node_item), SEEK_SET);
		if (fread(ret, sizeof(*ret), 1, coords) != 1) {
			fprintf(stderr,"read failed\n");
			return 0;
		}
		if (ret->id == id)
			return 1;
		if (ret->id < id) {
			p+=interval;
			if (interval == 1) {
				if (p >= count)
					return 0;
				if (ret->id > id)
					return 0;
			} else {
				if (p >= count)
					p=count-1;
			}
		} else {
			p-=interval;
			if (interval == 1) {
				if (p < 0)
					return 0;
				if (ret->id < id)
					return 0;
			} else {
				if (p < 0)
					p=0;
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


static int
parse_relation(char *p)
{
	debug_attr_buffer[0]='\0';
	relation_type[0]='\0';
	item_bin_init(item_bin, type_none);
	if (!add_id_attr(p, attr_osm_relationid))
		return 0;
	return 1;
}

static void
end_relation(FILE *turn_restrictions)
{
	struct item_bin *ib=(struct item_bin *)buffer;
	if (!strcmp(relation_type, "restriction") && (ib->type == type_street_turn_restriction_no || ib->type == type_street_turn_restriction_only))
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
	if (!strcmp(k,"type")) 
		strcpy(relation_type, v);
	else if (!strcmp(k,"restriction")) {
		if (!strncmp(v,"no_",3)) {
			item_bin->type=type_street_turn_restriction_no;
		} else if (!strncmp(v,"only_",5)) {
			item_bin->type=type_street_turn_restriction_only;
		} else {
			item_bin->type=type_none;
			osm_warning("relation", current_id, 0, "Unknown restriction %s\n",v);
		}
	}
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
		assert(count < 10);
	}
	for (i = 0 ; i < count ; i++) {
		add_flags=0;
		if (types[i] == type_none)
			continue;
		item_bin_init(item_bin,types[i]);
		item_bin_add_coord(item_bin, coord_buffer, coord_count);
		def_flags=item_get_default_flags(types[i]);
		if (def_flags) {
			if (coverage) {
				item.type=type_coverage;
			} else {
				flags_attr_value=(*def_flags | flags[0] | flags[1]) & ~flags[2];
				if (flags_attr_value != *def_flags)
					add_flags=1;
			}
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

struct population_table {
	enum item_type type;
	int population;
};

static struct population_table town_population[] = {
	{type_town_label_0e0,0},
	{type_town_label_1e0,1},
	{type_town_label_2e0,2},
	{type_town_label_5e0,5},
	{type_town_label_1e1,10},
	{type_town_label_2e1,20},
	{type_town_label_5e1,50},
	{type_town_label_1e2,100},
	{type_town_label_2e2,200},
	{type_town_label_5e2,500},
	{type_town_label_1e3,1000},
	{type_town_label_2e3,2000},
	{type_town_label_5e3,5000},
	{type_town_label_1e4,10000},
	{type_town_label_2e4,20000},
	{type_town_label_5e4,50000},
	{type_town_label_1e5,100000},
	{type_town_label_2e5,200000},
	{type_town_label_5e5,500000},
	{type_town_label_1e6,1000000},
	{type_town_label_2e6,2000000},
	{type_town_label_5e6,5000000},
	{type_town_label_1e7,1000000},
};

static struct population_table district_population[] = {
	{type_district_label_0e0,0},
	{type_district_label_1e0,1},
	{type_district_label_2e0,2},
	{type_district_label_5e0,5},
	{type_district_label_1e1,10},
	{type_district_label_2e1,20},
	{type_district_label_5e1,50},
	{type_district_label_1e2,100},
	{type_district_label_2e2,200},
	{type_district_label_5e2,500},
	{type_district_label_1e3,1000},
	{type_district_label_2e3,2000},
	{type_district_label_5e3,5000},
	{type_district_label_1e4,10000},
	{type_district_label_2e4,20000},
	{type_district_label_5e4,50000},
	{type_district_label_1e5,100000},
	{type_district_label_2e5,200000},
	{type_district_label_5e5,500000},
	{type_district_label_1e6,1000000},
	{type_district_label_2e6,2000000},
	{type_district_label_5e6,5000000},
	{type_district_label_1e7,1000000},
};

static void
end_node(FILE *out)
{
	int conflict,count,i;
	char *postal;
	enum item_type types[10];
	struct country_table *result=NULL, *lookup;
	if (!out || ! node_is_tagged || ! nodeid)
		return;
	count=attr_longest_match(attr_mapping_node, attr_mapping_node_count, types, sizeof(types)/sizeof(enum item_type));
	if (!count) {
		types[0]=type_point_unkn;
		count=1;
	}
	assert(count < 10);
	for (i = 0 ; i < count ; i++) {
		conflict=0;
		if (types[i] == type_none)
			continue;
		item_bin_init(item_bin, types[i]);
		if (item_is_town(*item_bin) && attr_strings[attr_string_population]) {
			int i,count,population=atoi(attr_strings[attr_string_population]);
			struct population_table *table;
			if (population < 0)
				population=0;
			if (item_is_district(*item_bin)) {
				table=district_population;
				count=sizeof(district_population)/sizeof(district_population[0]);
			} else {
				table=town_population;
				count=sizeof(town_population)/sizeof(town_population[0]);
			}
			for (i = 0 ; i < count ; i++) {
				if (population < table[i].population)
					break;
			}
			item_bin_set_type(item_bin, table[i-1].type);
		}
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
			while ((tok=strtok(buf, ","))) {
				while (*tok==' ')
					tok++;
				lookup=g_hash_table_lookup(country_table_hash,tok);
				if (lookup) {
					if (result && result->countryid != lookup->countryid) {
						fprintf(stderr,"conflict for %s %s country %d vs %d\n", attr_strings[attr_string_label], debug_attr_buffer, lookup->countryid, result->countryid);
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
					int i,words=0;
					char *town_name=attr_strings[attr_string_label];
					char *word=town_name;
					do  {
						for (i = 0 ; i < 3 ; i++) {
							char *str=linguistics_expand_special(word, i);
							if (str) {
								item_bin_init(item_bin, item_bin->type);
								item_bin_add_coord(item_bin, &ni->c, 1);
								if (i || words)
									item_bin_add_attr_string(item_bin, attr_town_name_match, str);
								item_bin_add_attr_string(item_bin, attr_town_name, town_name);
								item_bin_add_attr_string(item_bin, attr_town_postal, postal);
								item_bin_write(item_bin, result->file);
								g_free(str);
							}
						}
						word=linguistics_next_word(word);
						words++;
					} while (word);
				}
			
			}
		}
	}
	processed_nodes_out++;
}

static int
sort_countries_compare(const void *p1, const void *p2)
{
	struct item_bin *ib1=*((struct item_bin **)p1),*ib2=*((struct item_bin **)p2);
	struct attr_bin *attr1,*attr2;
	char *s1,*s2;
	assert(ib1->clen==2);
	assert(ib2->clen==2);
	attr1=(struct attr_bin *)((int *)(ib1+1)+ib1->clen);
	attr2=(struct attr_bin *)((int *)(ib2+1)+ib1->clen);
	assert(attr1->type == attr_town_name || attr1->type == attr_town_name_match);
	assert(attr2->type == attr_town_name || attr2->type == attr_town_name_match);
	s1=(char *)(attr1+1);
	s2=(char *)(attr2+1);
	return strcmp(s1, s2);
#if 0
	fprintf(stderr,"sort_countries_compare p1=%p p2=%p %s %s\n",p1,p2,s1,s2);
#endif
	return 0;
}

static void
sort_countries(int keep_tmpfiles)
{
	int i,j,count;
	struct country_table *co;
	struct coord *c;
	struct item_bin *ib;
	FILE *f;
	char *name;
	unsigned char *p,**idx,*buffer;
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		co=&country_table[i];
		if (co->file) {
			fclose(co->file);
			co->file=NULL;
		}
		name=g_strdup_printf("country_%d.bin.unsorted", co->countryid);
		if (file_get_contents(name, &buffer, &co->size)) {
			if(!keep_tmpfiles) 
				unlink(name);
			g_free(name);
			ib=(struct item_bin *)buffer;
			p=buffer;
			count=0;
			while (p < buffer+co->size) {
				count++;
				p+=(*((int *)p)+1)*4;
			}
			idx=malloc(count*sizeof(void *));
			assert(idx != NULL);
			p=buffer;
			for (j = 0 ; j < count ; j++) {
				idx[j]=p;
				p+=(*((int *)p)+1)*4;
			}
			qsort(idx, count, sizeof(void *), sort_countries_compare);
			name=g_strdup_printf("country_%d.bin", co->countryid);
			f=fopen(name,"wb");
			for (j = 0 ; j < count ; j++) {
				ib=(struct item_bin *)(idx[j]);
				c=(struct coord *)(ib+1);
				fwrite(ib, (ib->len+1)*4, 1, f);
				if (j) 
					bbox_extend(c, &co->r);
				else {
					co->r.l=*c;
					co->r.h=*c;
				}
			}
			fclose(f);
		}
		g_free(name);
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
	sscanf(str,"%d:%Ld:%n",&memb->type,&memb->id,&len);
	memb->role=str+len;
	return 1;
}

static int
search_relation_member(struct item_bin *ib, char *role, struct relation_member *memb)
{
	char *str=NULL;
	while ((str=item_bin_get_attr(ib, attr_osm_member, str))) {
		fprintf(stderr,"str=%s\n",str);
		if (!get_relation_member(str, memb))
			return 0;
		if (!strcmp(memb->role, role))
			return 1;
	}
	return 0;
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
	for (;;) {
		fseek(ways_index, p*sizeof(idx), SEEK_SET);
		if (fread(idx, sizeof(idx), 1, ways_index) != 1) {
			fprintf(stderr,"read failed\n");
			return 0;
		}
		if (idx[0] == wayid) {
			fseek(way, idx[1], SEEK_SET);
			return 1;
		}
		if (idx[0] < wayid) {
			p+=interval;
			if (interval == 1) {
				if (p >= count)
					return 0;
				if (idx[0] > wayid)
					return 0;
			} else {
				if (p >= count)
					p=count-1;
			}
		} else {
			p-=interval;
			if (interval == 1) {
				if (p < 0)
					return 0;
				if (idx[0] < wayid)
					return 0;
			} else {
				if (p < 0)
					p=0;
			}
		}
		if (interval > 1)
			interval/=2;
	}
}

static struct coord *
get_way(FILE *way, FILE *ways_index, struct coord *c, long long wayid, struct item_bin *ret)
{
	long long currid;
	int last;
	struct coord *ic;
	if (!seek_to_way(way, ways_index, wayid))
		return NULL;
	while (item_bin_read(ret, way)) {
		currid=item_bin_get_wayid(ret);
		if (currid != wayid) 
			return NULL;
		ic=(struct coord *)(ret+1);
		last=ret->clen/2-1;
		if (!c) 
			return &ic[0];
		if (ic[0].x == c->x && ic[0].y == c->y) 
			return &ic[last];
		if (ic[last].x == c->x && ic[last].y == c->y) 
			return &ic[0];
	}
	return NULL;
}



static void
process_turn_restrictions(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out)
{
	struct relation_member fromm,tom,viam;
	struct node_item ni;
	long long relid;
	char from_buffer[65536],to_buffer[65536],via_buffer[65536];
	struct item_bin *ib=(struct item_bin *)buffer,*from=(struct item_bin *)from_buffer,*to=(struct item_bin *)to_buffer,*via=(struct item_bin *)via_buffer;
	struct coord *fromc,*toc,*viafrom,*viato,*tmp;
	fseek(in, 0, SEEK_SET);
	while (item_bin_read(ib, in)) {
		relid=item_bin_get_relationid(ib);
		fprintf(stderr,"Relation %Ld\n",relid);
		if (!search_relation_member(ib, "from",&fromm)) {
			osm_warning("relation",relid,0,"turn restriction: from member missing\n");
			continue;
		}
		if (!search_relation_member(ib, "to",&tom)) {
			osm_warning("relation",relid,0,"turn restriction: to member missing\n");
			continue;
		}
		if (!search_relation_member(ib, "via",&viam)) {
			osm_warning("relation",relid,0,"turn restriction: via member missing\n");
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
			if (!(viafrom=get_way(ways, ways_index, NULL, viam.id, via))) {
				osm_warning("relation",relid,0,"turn restriction: failed to get first via coordinate from ");
				osm_warning(osm_types[viam.type],viam.id,1,"\n");
				continue;
			}
			if (!(viato=get_way(ways, ways_index, viafrom, viam.id, via))) {
				osm_warning("relation",relid,0,"turn restriction: failed to get last via coordinate from ");
				osm_warning(osm_types[viam.type],viam.id,1,"\n");
				continue;
			}
				
		}
#if 0
		fprintf(stderr,"via %Ld vs %d\n",viam.id, ni.id);
		fprintf(stderr,"coord 0x%x,0x%x\n",ni.c.x,ni.c.y);	
		fprintf(stderr,"Lookup %Ld\n",fromm.id);
#endif
		if (!(fromc=get_way(ways, ways_index, viafrom, fromm.id, from))) {
			if (viam.type == 1 || !(fromc=get_way(ways, ways_index, viato, fromm.id, from))) {
				osm_warning("relation",relid,0,"turn restriction: failed to connect via ");
				osm_warning(osm_types[viam.type],viam.id,1," to from member ");
				osm_warning(osm_types[fromm.type],fromm.id,1,"\n");
				continue;
			} else {
				tmp=viato;
				viato=viafrom;
				viafrom=tmp;
			}
		}
		if (!(toc=get_way(ways, ways_index, viato, tom.id, to))) {
			osm_warning("relation",relid,0,"turn restriction: failed to connect via ");
			osm_warning(osm_types[viam.type],viam.id,1," to to member ");
			osm_warning(osm_types[tom.type],tom.id,1,"\n");
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
node_ref_way(osmid node)
{
	struct node_item *ni;
	ni=node_item_get(node);
	if (ni) 
		ni->ref_way++; 
}

static int
resolve_ways(FILE *in, FILE *out)
{
	struct item_bin *ib=(struct item_bin *)buffer;
	struct coord *c;
	int i;

	fseek(in, 0, SEEK_SET);
	for (;;) {
		switch (item_bin_read(ib, in)) {
		case 0:
			return 0;
		case 2:
			c=(struct coord *)(ib+1);
			for (i = 0 ; i < ib->clen/2 ; i++) {
				node_ref_way(REF(c[i]));
			}
		default:
			continue;
		}
	}
	
	
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


static void
save_buffer(char *filename, struct buffer *b, long long offset)
{
	FILE *f;
	f=fopen(filename,"rb+");
	if (! f)
		f=fopen(filename,"wb+");
		
	assert(f != NULL);
	fseek(f, offset, SEEK_SET);
	fwrite(b->base, b->size, 1, f);
	fclose(f);
}

static void
load_buffer(char *filename, struct buffer *b, long long offset, long long size)
{
	FILE *f;
	long long len;
	int ret;
	if (b->base)
		free(b->base);
	b->malloced=0;
	f=fopen(filename,"rb");
	fseek(f, 0, SEEK_END);
	len=ftell(f);
	if (offset+size > len) {
		size=len-offset;
		ret=1;
	}
	b->size=b->malloced=size;
	fprintf(stderr,"reading %Ld bytes from %s at %Ld\n", b->size, filename, offset);
	fseek(f, offset, SEEK_SET);
	b->base=malloc(b->size);
	assert(b->base != NULL);
	fread(b->base, b->size, 1, f);
	fclose(f);
}

static int
phase1(FILE *in, FILE *out_ways, FILE *out_nodes, FILE *out_turn_restrictions)
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
			end_relation(out_turn_restrictions);
		} else if (!strncmp(p, "</osm>",6)) {
		} else {
			fprintf(stderr,"WARNING: unknown tag in %s\n", buffer);
		}
	}
	sig_alrm(0);
#ifndef _WIN32
	alarm(0);
#endif
	return 1;
}

#ifdef HAVE_POSTGRESQL
static int
phase1_db(char *dbstr, FILE *out_ways, FILE *out_nodes)
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
#ifndef _WIN32
	alarm(0);
#endif
	return 1;
}
#endif


static void
phase1_map(struct map *map, FILE *out_ways, FILE *out_nodes)
{
	struct map_rect *mr=map_rect_new(map, NULL);
	struct item *item;
	int count,max=16384;
	struct coord ca[max];
	struct attr attr;

	while ((item = map_rect_get_item(mr))) {
		count=item_coord_get(item, ca, item->type < type_line ? 1: max);
		item_bin_init(item_bin, item->type);
		item_bin_add_coord(item_bin, ca, count);
		while (item_attr_get(item, attr_any, &attr)) {
			item_bin_add_attr(item_bin, &attr);
		}
                if (item->type >= type_line) 
			item_bin_write(item_bin, out_ways);
		else
			item_bin_write(item_bin, out_nodes);
	}
	map_rect_destroy(mr);
}


int bytes_read=0;

static struct item_bin *
read_item(FILE *in)
{
	struct item_bin *ib=(struct item_bin *) buffer;
	int r,s;
	r=fread(ib, sizeof(*ib), 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	assert((ib->len+1)*4 < sizeof(buffer));
	s=(ib->len+1)*4-sizeof(*ib);
	r=fread(ib+1, s, 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	return ib;
}

static void
bbox_extend(struct coord *c, struct rect *r)
{
	if (c->x < r->l.x)
		r->l.x=c->x;
	if (c->y < r->l.y)
		r->l.y=c->y;
	if (c->x > r->h.x)
		r->h.x=c->x;
	if (c->y > r->h.y)
		r->h.y=c->y;
}

static void
bbox(struct coord *c, int count, struct rect *r)
{
	if (! count)
		return;
	r->l=*c;
	r->h=*c;
	while (--count) {
		c++;
		bbox_extend(c, r);
	}
}

static int
contains_bbox(int xl, int yl, int xh, int yh, struct rect *r)
{
        if (r->h.x < xl || r->h.x > xh) {
                return 0;
        }
        if (r->l.x > xh || r->l.x < xl) {
                return 0;
        }
        if (r->h.y < yl || r->h.y > yh) {
                return 0;
        }
        if (r->l.y > yh || r->l.y < yl) {
                return 0;
        }
        return 1;

}
struct rect world_bbox = {
	{ -20000000, -20000000},
	{  20000000,  20000000},
};

int overlap=1;

static void
tile(struct rect *r, char *suffix, char *ret, int max)
{
	int x0,x2,x4;
	int y0,y2,y4;
	int xo,yo;
	int i;
	x0=world_bbox.l.x;
	y0=world_bbox.l.y;
	x4=world_bbox.h.x;
	y4=world_bbox.h.y;
	for (i = 0 ; i < max ; i++) {
		x2=(x0+x4)/2;
                y2=(y0+y4)/2;
		xo=(x4-x0)*overlap/100;
		yo=(y4-y0)*overlap/100;
     		if (     contains_bbox(x0,y0,x2+xo,y2+yo,r)) {
			strcat(ret,"d");
                        x4=x2+xo;
                        y4=y2+yo;
                } else if (contains_bbox(x2-xo,y0,x4,y2+yo,r)) {
			strcat(ret,"c");
                        x0=x2-xo;
                        y4=y2+yo;
                } else if (contains_bbox(x0,y2-yo,x2+xo,y4,r)) {
			strcat(ret,"b");
                        x4=x2+xo;
                        y0=y2-yo;
                } else if (contains_bbox(x2-xo,y2-yo,x4,y4,r)) {
			strcat(ret,"a");
                        x0=x2-xo;
                        y0=y2-yo;
		} else 
			break;
	}
	if (suffix)
		strcat(ret,suffix);
}

static void
tile_bbox(char *tile, struct rect *r)
{
	struct coord c;
	int xo,yo;
	*r=world_bbox;
	while (*tile) {
		c.x=(r->l.x+r->h.x)/2;
		c.y=(r->l.y+r->h.y)/2;
		xo=(r->h.x-r->l.x)*overlap/100;
		yo=(r->h.y-r->l.y)*overlap/100;
		switch (*tile) {
		case 'a':
			r->l.x=c.x-xo;
			r->l.y=c.y-yo;
			break;
		case 'b':
			r->h.x=c.x+xo;
			r->l.y=c.y-yo;
			break;
		case 'c':
			r->l.x=c.x-xo;
			r->h.y=c.y+yo;
			break;
		case 'd':
			r->h.x=c.x+xo;
			r->h.y=c.y+yo;
			break;
		}
		tile++;
	}
}

static int
tile_len(char *tile)
{
	int ret=0;
	while (tile[0] >= 'a' && tile[0] <= 'd') {
		tile++;
		ret++;
	}
	return ret;
}

GHashTable *tile_hash;
GHashTable *tile_hash2;

static void
tile_extend(char *tile, struct item_bin *ib, GList **tiles_list)
{
	struct tile_head *th=NULL;
	if (debug_tile(tile))
		fprintf(stderr,"Tile:Writing %d bytes to '%s' (%p,%p)\n", (ib->len+1)*4, tile, g_hash_table_lookup(tile_hash, tile), tile_hash2 ? g_hash_table_lookup(tile_hash2, tile) : NULL);
	if (tile_hash2)
		th=g_hash_table_lookup(tile_hash2, tile);
	if (!th)
		th=g_hash_table_lookup(tile_hash, tile);
	if (! th) {
		th=malloc(sizeof(struct tile_head)+ sizeof( char* ) );
		assert(th != NULL);
		// strcpy(th->subtiles, tile);
		th->num_subtiles=1;
		th->total_size=0;
		th->total_size_used=0;
		th->zipnum=0;
		th->zip_data=NULL;
		th->name=string_hash_lookup(tile);
		*th_get_subtile( th, 0 ) = th->name;

		if (tile_hash2)
			g_hash_table_insert(tile_hash2, string_hash_lookup( th->name ), th);
		if (tiles_list)
			*tiles_list=g_list_append(*tiles_list, string_hash_lookup( th->name ) );
		processed_tiles++;
		if (debug_tile(tile))
			fprintf(stderr,"new '%s'\n", tile);
	}
	th->total_size+=ib->len*4+4;
	if (debug_tile(tile))
		fprintf(stderr,"New total size of %s(%p):%d\n", th->name, th, th->total_size);
	g_hash_table_insert(tile_hash, string_hash_lookup( th->name ), th);
}

static int
tile_data_size(char *tile)
{
	struct tile_head *th;
	th=g_hash_table_lookup(tile_hash, tile);
	if (! th)
		return 0;
	return th->total_size;
}

static int
merge_tile(char *base, char *sub)
{
	struct tile_head *thb, *ths;
	thb=g_hash_table_lookup(tile_hash, base);
	ths=g_hash_table_lookup(tile_hash, sub);
	if (! ths)
		return 0;
	if (debug_tile(base) || debug_tile(sub))
		fprintf(stderr,"merging '%s'(%p) (%d) with '%s'(%p) (%d)\n", base, thb, thb ? thb->total_size : 0, sub, ths, ths->total_size);
	if (! thb) {
		thb=ths;
		g_hash_table_remove(tile_hash, sub);
		thb->name=string_hash_lookup(base);
		g_hash_table_insert(tile_hash, string_hash_lookup( thb->name ), thb);

	} else {
		thb=realloc(thb, sizeof(struct tile_head)+( ths->num_subtiles+thb->num_subtiles ) * sizeof( char*) );
		assert(thb != NULL);
		memcpy( th_get_subtile( thb, thb->num_subtiles ), th_get_subtile( ths, 0 ), ths->num_subtiles * sizeof( char*) );
		thb->num_subtiles+=ths->num_subtiles;
		thb->total_size+=ths->total_size;
		g_hash_table_insert(tile_hash, string_hash_lookup( thb->name ), thb);
		g_hash_table_remove(tile_hash, sub);
		g_free(ths);
	}
	return 1;
}


static void
get_tiles_list_func(char *key, struct tile_head *th, GList **list)
{
	*list=g_list_prepend(*list, key);
}

static GList *
get_tiles_list(void)
{
	GList *ret=NULL;
	g_hash_table_foreach(tile_hash, (GHFunc)get_tiles_list_func, &ret);
	return ret;
}

#if 0
static void
write_tile(char *key, struct tile_head *th, gpointer dummy)
{
	FILE *f;
	char buffer[1024];
	fprintf(stderr,"DEBUG: Writing %s\n", key);
	strcpy(buffer,"tiles/");
	strcat(buffer,key);
#if 0
	strcat(buffer,".bin");
#endif
	f=fopen(buffer, "wb+");
	while (th) {
		fwrite(th->data, th->size, 1, f);
		th=th->next;
	}
	fclose(f);
}
#endif

static void
write_item(char *tile, struct item_bin *ib)
{
	struct tile_head *th;
	int size;

	th=g_hash_table_lookup(tile_hash2, tile);
	if (! th)
		th=g_hash_table_lookup(tile_hash, tile);
	if (th) {
		if (th->process != 0 && th->process != 1) {
			fprintf(stderr,"error with tile '%s' of length %d\n", tile, (int)strlen(tile));
			abort();
		}
		if (! th->process)
			return;
		if (debug_tile(tile))
			fprintf(stderr,"Data:Writing %d bytes to '%s' (%p,%p)\n", (ib->len+1)*4, tile, g_hash_table_lookup(tile_hash, tile), tile_hash2 ? g_hash_table_lookup(tile_hash2, tile) : NULL);
		size=(ib->len+1)*4;
		if (th->total_size_used+size > th->total_size) {
			fprintf(stderr,"Overflow in tile %s (used %d max %d item %d)\n", tile, th->total_size_used, th->total_size, size);
			exit(1);
			return;
		}
		memcpy(th->zip_data+th->total_size_used, ib, size);
		th->total_size_used+=size;
	} else {
		fprintf(stderr,"no tile hash found for %s\n", tile);
		exit(1);
	}
}

static void
write_item_part(FILE *out, FILE *out_index, FILE *out_graph, struct item_bin *orig, int first, int last)
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
			fwrite(idx, sizeof(idx), 1, out_index);
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

static int
phase2(FILE *in, FILE *out, FILE *out_index, FILE *out_graph, FILE *out_coastline, int final)
{
	struct coord *c;
	int i,ccount,last,remaining;
	osmid ndref;
	struct item_bin *ib;
	struct node_item *ni;
	FILE *out_index_tmp;

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	while ((ib=read_item(in))) {
#if 0
		fprintf(stderr,"type 0x%x len %d clen %d\n", ib->type, ib->len, ib->clen);
#endif
		out_index_tmp=out_index;
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
						write_item_part(out, out_index_tmp, out_graph, ib, last, i);
						out_index_tmp=NULL;
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
			write_item_part(out, out_index_tmp, out_graph, ib, last, ccount-1);
			if (final && ib->type == type_water_line && out_coastline) {
				write_item_part(out_coastline, NULL, NULL, ib, last, ccount-1);
			}
		}
	}
	sig_alrm(0);
#ifndef _WIN32
	alarm(0);
#endif
	return 0;
}

struct tile_info {
	int write;
	int maxlen;
	char *suffix;
	GList **tiles_list;
	FILE *tilesdir_out;
};

struct zip_info {
	int zipnum;
	int dir_size;
	long long offset;
	int compression_level;
	int maxnamelen;
	FILE *res;
	FILE *index;
	FILE *dir;
};

static void write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size);

static void
tile_write_item_to_tile(struct tile_info *info, struct item_bin *ib, char *name)
{
	if (info->write)
		write_item(name, ib);
	else
		tile_extend(name, ib, info->tiles_list);
}

static void
tile_write_item_minmax(struct tile_info *info, struct item_bin *ib, int min, int max)
{
	struct rect r;
	char buffer[1024];
	bbox((struct coord *)(ib+1), ib->clen/2, &r);
	buffer[0]='\0';
	tile(&r, info->suffix, buffer, max);
	tile_write_item_to_tile(info, ib, buffer);
}

static void
phase34_process_file(struct tile_info *info, FILE *in)
{
	struct item_bin *ib;
	int max;

	while ((ib=read_item(in))) {
		if (ib->type < 0x80000000)
			processed_nodes++;
		else
			processed_ways++;
		max=14;
		switch (ib->type) {
		case type_town_label_1e7:
		case type_town_label_5e6:
		case type_town_label_2e6:
		case type_town_label_1e6:
		case type_town_label_5e5:
		case type_district_label_1e7:
		case type_district_label_5e6:
		case type_district_label_2e6:
		case type_district_label_1e6:
		case type_district_label_5e5:
			max=6;
			break;
		case type_town_label_2e5:
		case type_town_label_1e5:
		case type_district_label_2e5:
		case type_district_label_1e5:
		case type_street_n_lanes:
		case type_highway_city:
		case type_highway_land:
		case type_ramp:
			max=8;
			break;
		case type_town_label_5e4:
		case type_town_label_2e4:
		case type_town_label_1e4:
		case type_district_label_5e4:
		case type_district_label_2e4:
		case type_district_label_1e4:
			max=9;
			break;
		case type_town_label_5e3:
		case type_town_label_2e3:
		case type_town_label_1e3:
		case type_district_label_5e3:
		case type_district_label_2e3:
		case type_district_label_1e3:
		case type_street_3_city:
		case type_street_4_city:
		case type_street_3_land:
		case type_street_4_land:
			max=12;
			break;
		default:
			break;
		}
		tile_write_item_minmax(info, ib, 0, max);
	}
}

static void
index_init(struct zip_info *info, int version)
{
	item_bin_init(item_bin, type_map_information);
	item_bin_add_attr_int(item_bin, attr_version, version);
	item_bin_write(item_bin, info->index);
}

static void
index_submap_add(struct tile_info *info, struct tile_head *th)
{
	int tlen=tile_len(th->name);
	int len=tlen;
	char index_tile[len+1+strlen(suffix)];
	struct rect r;

	strcpy(index_tile, th->name);
	if (len > 6)
		len=6;
	else
		len=0;
	index_tile[len]=0;
	if (tlen)
		strcat(index_tile, suffix);
	tile_bbox(th->name, &r);

	item_bin_init(item_bin, type_submap);
	item_bin_add_coord_rect(item_bin, &r);
	item_bin_add_attr_range(item_bin, attr_order, (tlen > 4)?tlen-4 : 0, 255);
	item_bin_add_attr_int(item_bin, attr_zipfile_ref, th->zipnum);
	tile_write_item_to_tile(info, item_bin, index_tile);
}

static int
add_tile_hash(struct tile_head *th)
{
	int idx,len,maxnamelen=0;
	char **data;

#if 0
	g_hash_table_insert(tile_hash2, string_hash_lookup( th->name ), th);
#endif
	for( idx = 0; idx < th->num_subtiles; idx++ ) {

        data = th_get_subtile( th, idx );

		if (debug_tile(data) || debug_tile(th->name)) {
			fprintf(stderr,"Parent for '%s' is '%s'\n", *data, th->name);
		}

		g_hash_table_insert(tile_hash2, *data, th);

		len = strlen( *data );

		if (len > maxnamelen) {
			maxnamelen=len;
		}
	}
	return maxnamelen;
}


static int
create_tile_hash(void)
{
	struct tile_head *th;
	int len,maxnamelen=0;

	tile_hash2=g_hash_table_new(g_str_hash, g_str_equal);
	th=tile_head_root;
	while (th) {
		len=add_tile_hash(th);
		if (len > maxnamelen)
			maxnamelen=len;
		th=th->next;
	}
	return maxnamelen;
}

static void
create_tile_hash_list(GList *list)
{
	GList *next;
	struct tile_head *th;

	tile_hash2=g_hash_table_new(g_str_hash, g_str_equal);

	next=g_list_first(list);
	while (next) {
		th=g_hash_table_lookup(tile_hash, next->data);
		if (!th) {
			fprintf(stderr,"No tile found for '%s'\n", (char *)(next->data));
		}
		add_tile_hash(th);
		next=g_list_next(next);
	}
}

#if 0
static void
destroy_tile_hash(void)
{
	g_hash_table_destroy(tile_hash2);
	tile_hash2=NULL;
}
#endif

static void write_countrydir(struct zip_info *zip_info);

static void
write_tilesdir(struct tile_info *info, struct zip_info *zip_info, FILE *out)
{
	int idx,len,maxlen;
	GList *next,*tiles_list;
	char **data;
	struct tile_head *th,**last=NULL;

	tiles_list=get_tiles_list();
	info->tiles_list=&tiles_list;
	if (phase == 3)
		create_tile_hash_list(tiles_list);
	next=g_list_first(tiles_list);
	last=&tile_head_root;
	maxlen=info->maxlen;
	if (! maxlen) {
		while (next) {
			if (strlen(next->data) > maxlen)
				maxlen=strlen(next->data);
			next=g_list_next(next);
		}
	}
	len=maxlen;
	while (len >= 0) {
#if 0
		fprintf(stderr,"PROGRESS: collecting tiles with len=%d\n", len);
#endif
		next=g_list_first(tiles_list);
		while (next) {
			if (strlen(next->data) == len) {
				th=g_hash_table_lookup(tile_hash, next->data);
				if (phase == 3) {
					*last=th;
					last=&th->next;
					th->next=NULL;
					th->zipnum=zip_info->zipnum;
					fprintf(out,"%s:%d",(char *)next->data,th->total_size);

					for ( idx = 0; idx< th->num_subtiles; idx++ ){
                        data= th_get_subtile( th, idx );
						fprintf(out,":%s", *data);
					}

					fprintf(out,"\n");
				}
				if (th->name[0])
					index_submap_add(info, th);
				zip_info->zipnum++;
				processed_tiles++;
			}
			next=g_list_next(next);
		}
		len--;
	}
}

static void
merge_tiles(struct tile_info *info)
{
	struct tile_head *th;
	char basetile[1024];
	char subtile[1024];
	GList *tiles_list_sorted,*last;
	int i,i_min,len,size_all,size[5],size_min,work_done;
	long long zip_size;

	do {
		tiles_list_sorted=get_tiles_list();
		fprintf(stderr,"PROGRESS: sorting %d tiles\n", g_list_length(tiles_list_sorted));
		tiles_list_sorted=g_list_sort(tiles_list_sorted, (GCompareFunc)strcmp);
		fprintf(stderr,"PROGRESS: sorting %d tiles done\n", g_list_length(tiles_list_sorted));
		last=g_list_last(tiles_list_sorted);
		zip_size=0;
		while (last) {
			th=g_hash_table_lookup(tile_hash, last->data);
			zip_size+=th->total_size;
			last=g_list_previous(last);
		}
		last=g_list_last(tiles_list_sorted);
		work_done=0;
		while (last) {
			processed_tiles++;
			len=tile_len(last->data);
			if (len >= 1) {
				strcpy(basetile,last->data);
				basetile[len-1]='\0';
				strcat(basetile, info->suffix);
				strcpy(subtile,last->data);
				for (i = 0 ; i < 4 ; i++) {
					subtile[len-1]='a'+i;
					size[i]=tile_data_size(subtile);
				}
				size[4]=tile_data_size(basetile);
				size_all=size[0]+size[1]+size[2]+size[3]+size[4];
				if (size_all < 65536 && size_all > 0 && size_all != size[4]) {
					for (i = 0 ; i < 4 ; i++) {
						subtile[len-1]='a'+i;
						work_done+=merge_tile(basetile, subtile);
					}
				} else {
					for (;;) {
						size_min=size_all;
						i_min=-1;
						for (i = 0 ; i < 4 ; i++) {
							if (size[i] && size[i] < size_min) {
								size_min=size[i];
								i_min=i;
							}
						}
						if (i_min == -1)
							break;
						if (size[4]+size_min >= 65536)
							break;
						subtile[len-1]='a'+i_min;
						work_done+=merge_tile(basetile, subtile);
						size[4]+=size[i_min];
						size[i_min]=0;
					}
				}
			}
			last=g_list_previous(last);
		}
		g_list_free(tiles_list_sorted);
		fprintf(stderr,"PROGRESS: merged %d tiles\n", work_done);
	} while (work_done);
}

static void
index_country_add(struct zip_info *info, int country_id, int zipnum)
{
	item_bin_init(item_bin, type_countryindex);
	item_bin_add_attr_int(item_bin, attr_country_id, country_id);
	item_bin_add_attr_int(item_bin, attr_zipfile_ref, zipnum);
	item_bin_write(item_bin, info->index);
}

struct aux_tile {
	char *name;
	char *filename;
	int size;
};

static int
add_aux_tile(struct zip_info *zip_info, char *name, char *filename, int size)
{
	struct aux_tile *at;
	GList *l;
	l=aux_tile_list;
	while (l) {
		at=l->data;
		if (!strcmp(at->name, name)) {
			fprintf(stderr,"exists %s vs %s\n",at->name, name);
			return -1;
		}
		l=g_list_next(l);
	}
	at=g_new0(struct aux_tile, 1);
	at->name=g_strdup(name);
	at->filename=g_strdup(filename);
	at->size=size;
	aux_tile_list=g_list_append(aux_tile_list, at);
	return zip_info->zipnum++;
}

static int
write_aux_tiles(struct zip_info *zip_info)
{
	GList *l=aux_tile_list;
	struct aux_tile *at;
	char *buffer;
	FILE *f;
	int count=0;
	
	while (l) {
		at=l->data;
		buffer=malloc(at->size);
		assert(buffer != NULL);
		f=fopen(at->filename,"rb");
		assert(f != NULL);
		fread(buffer, at->size, 1, f);
		fclose(f);
		write_zipmember(zip_info, at->name, zip_info->maxnamelen, buffer, at->size);
		free(buffer);
		count++;
		l=g_list_next(l);
		zip_info->zipnum++;
	}
	return count;
}

static void
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
				tile(&co->r, suffix, tilename, max);
				sprintf(filename,"country_%d.bin", co->countryid);
				zipnum=add_aux_tile(zip_info, tilename, filename, co->size);
			} while (zipnum == -1);
			index_country_add(zip_info,co->countryid,zipnum);
		}
	}
}

static void
write_index(struct zip_info *info)
{
        int size=ftell(info->index);
	char buffer[size];

	fseek(info->index, 0, SEEK_SET);
	fread(buffer, size, 1, info->index);
	write_zipmember(info, "index", strlen("index"), buffer, size);
	info->zipnum++;
}

static void
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

static int
phase34(struct tile_info *info, struct zip_info *zip_info, FILE *relations_in, FILE *ways_in, FILE *nodes_in)
{

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	bytes_read=0;
	sig_alrm(0);
	if (! info->write)
		tile_hash=g_hash_table_new(g_str_hash, g_str_equal);
	if (relations_in)
		phase34_process_file(info, relations_in);
	if (ways_in)
		phase34_process_file(info, ways_in);
	if (nodes_in)
		phase34_process_file(info, nodes_in);
	if (! info->write)
		merge_tiles(info);
	sig_alrm(0);
#ifndef _WIN32
	alarm(0);
#endif
	write_tilesdir(info, zip_info, info->tilesdir_out);

	return 0;

}

static void
dump_coord(struct coord *c)
{
	printf("0x%x 0x%x",c->x, c->y);
}

static void
dump(FILE *in)
{
	struct item_bin *ib;
	struct coord *c;
	struct attr_bin *a;
	struct attr attr;
	int *attr_start;
	int *attr_end;
	int i;
	char *str;
	fprintf(stderr,"enter\n");
	while ((ib=read_item(in))) {
		fprintf(stderr,"read item\n");
		c=(struct coord *)(ib+1);
		if (ib->type < type_line) {
			dump_coord(c);
			printf(" ");
		}
		attr_start=(int *)(ib+1)+ib->clen;
		attr_end=(int *)ib+ib->len+1;
		printf("type=%s", item_to_name(ib->type));
		while (attr_start < attr_end) {
			a=(struct attr_bin *)(attr_start);
			attr_start+=a->len+1;
			attr.type=a->type;
			attr_data_set(&attr, (a+1));
			str=attr_to_text(&attr, NULL, 1);
			printf(" %s=\"%s\"", attr_to_name(a->type), str);
			g_free(str);
		}
		printf(" debug=\"length=%d\"", ib->len);
		printf("\n");
		if (ib->type >= type_line) {
			for (i = 0 ; i < ib->clen/2 ; i++) {
				dump_coord(c+i);
				printf("\n");
			}
			
		}
	}
}

static int
phase4(FILE *relations_in, FILE *ways_in, FILE *nodes_in, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info)
{
	struct tile_info info;
	info.write=0;
	info.maxlen=0;
	info.suffix=suffix;
	info.tiles_list=NULL;
	info.tilesdir_out=tilesdir_out;
	return phase34(&info, zip_info, relations_in, ways_in, nodes_in);
}

static int
compress2_int(Byte *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	err = deflateInit2(&stream, level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) return err;

	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*destLen = stream.total_out;

	err = deflateEnd(&stream);
	return err;
}

static void
write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size)
{
	struct zip_lfh lfh = {
		0x04034b50,
		0x0a,
		0x0,
		0x0,
		0xbe2a,
		0x5d37,
		0x0,
		0x0,
		0x0,
		filelen,
		0x0,
	};
	struct zip_cd cd = {
		0x02014b50,
		0x17,
		0x00,
		0x0a,
		0x00,
		0x0000,
		0x0,
		0xbe2a,
		0x5d37,
		0x0,
		0x0,
		0x0,
		filelen,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0,
		zip_info->offset,
	};
	char filename[filelen+1];
	int error,crc,len,comp_size=data_size;
	uLongf destlen=data_size+data_size/500+12;
	char *compbuffer;

	compbuffer = malloc(destlen);
	if (!compbuffer) {
	  fprintf(stderr, "No more memory.\n");
	  exit (1);
	}

	crc=crc32(0, NULL, 0);
	crc=crc32(crc, (unsigned char *)data, data_size);
#ifdef HAVE_ZLIB
	if (zip_info->compression_level) {
		error=compress2_int((Byte *)compbuffer, &destlen, (Bytef *)data, data_size, zip_info->compression_level);
		if (error == Z_OK) {
			if (destlen < data_size) {
				data=compbuffer;
				comp_size=destlen;
			}
		} else {
			fprintf(stderr,"compress2 returned %d\n", error);
		}
	}
#endif
	lfh.zipcrc=crc;
	lfh.zipsize=comp_size;
	lfh.zipuncmp=data_size;
	lfh.zipmthd=zip_info->compression_level ? 8:0;
	cd.zipccrc=crc;
	cd.zipcsiz=comp_size;
	cd.zipcunc=data_size;
	cd.zipcmthd=zip_info->compression_level ? 8:0;
	strcpy(filename, name);
	len=strlen(filename);
	while (len < filelen) {
		filename[len++]='_';
	}
	filename[filelen]='\0';
	fwrite(&lfh, sizeof(lfh), 1, zip_info->res);
	fwrite(filename, filelen, 1, zip_info->res);
	fwrite(data, comp_size, 1, zip_info->res);
	zip_info->offset+=sizeof(lfh)+filelen+comp_size;
	fwrite(&cd, sizeof(cd), 1, zip_info->dir);
	fwrite(filename, filelen, 1, zip_info->dir);
	zip_info->dir_size+=sizeof(cd)+filelen;
	
	free(compbuffer);
}

static int
process_slice(FILE *relations_in, FILE *ways_in, FILE *nodes_in, long long size, char *suffix, struct zip_info *zip_info)
{
	struct tile_head *th;
	char *slice_data,*zip_data;
	int zipfiles=0;
	struct tile_info info;

	slice_data=malloc(size);
	assert(slice_data != NULL);
	zip_data=slice_data;
	th=tile_head_root;
	while (th) {
		if (th->process) {
			th->zip_data=zip_data;
			zip_data+=th->total_size;
		}
		th=th->next;
	}
	if (ways_in)
		fseek(ways_in, 0, SEEK_SET);
	if (nodes_in)
		fseek(nodes_in, 0, SEEK_SET);
	info.write=1;
	info.maxlen=zip_info->maxnamelen;
	info.suffix=suffix;
	info.tiles_list=NULL;
	info.tilesdir_out=NULL;
	phase34(&info, zip_info, relations_in, ways_in, nodes_in);

	th=tile_head_root;
	while (th) {
		if (th->process) {
			if (th->name[0]) {
				if (th->total_size != th->total_size_used) {
					fprintf(stderr,"Size error '%s': %d vs %d\n", th->name, th->total_size, th->total_size_used);
					exit(1);
				}
				write_zipmember(zip_info, th->name, zip_info->maxnamelen, th->zip_data, th->total_size);
				zipfiles++;
			} else 
				fwrite(th->zip_data, th->total_size, 1, zip_info->index);
		}
		th=th->next;
	}
	free(slice_data);

	return zipfiles;
}

static void
cat(FILE *in, FILE *out)
{
	size_t size;
	char buffer[4096];
	while ((size=fread(buffer, 1, 4096, in)))
		fwrite(buffer, 1, size, out);
}

static int
phase5(FILE *relations_in, FILE *ways_in, FILE *nodes_in, char *suffix, struct zip_info *zip_info)
{
	long long size;
	int slices;
	int zipnum,written_tiles;
	struct tile_head *th,*th2;
	create_tile_hash();

	th=tile_head_root;
	size=0;
	slices=0;
	fprintf(stderr, "Maximum slice size %Ld\n", slice_size);
	while (th) {
		if (size + th->total_size > slice_size) {
			fprintf(stderr,"Slice %d is of size %Ld\n", slices, size);
			size=0;
			slices++;
		}
		size+=th->total_size;
		th=th->next;
	}
	if (size)
		fprintf(stderr,"Slice %d is of size %Ld\n", slices, size);
	th=tile_head_root;
	size=0;
	slices=0;
	while (th) {
		th2=tile_head_root;
		while (th2) {
			th2->process=0;
			th2=th2->next;
		}
		size=0;
		while (th && size+th->total_size < slice_size) {
			size+=th->total_size;
			th->process=1;
			th=th->next;
		}
		/* process_slice() modifies zip_info, but need to retain old info */
		zipnum=zip_info->zipnum;
		written_tiles=process_slice(relations_in, ways_in, nodes_in, size, suffix, zip_info);
		zip_info->zipnum=zipnum+written_tiles;
		slices++;
	}
	return 0;
}

static int
phase5_write_directory(struct zip_info *info)
{
	struct zip_eoc eoc = {
		0x06054b50,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0,
		0x0,
		0x0,
	};

	fseek(info->dir, 0, SEEK_SET);
	cat(info->dir, info->res);
	eoc.zipenum=info->zipnum;
	eoc.zipecenn=info->zipnum;
	eoc.zipecsz=info->dir_size;
	eoc.zipeofst=info->offset;
	fwrite(&eoc, sizeof(eoc), 1, info->res);
	sig_alrm(0);
#ifndef _WIN32
	alarm(0);
#endif
	return 0;
}



static void
usage(FILE *f)
{
	/* DEVELOPPERS : don't forget to update the manpage if you modify theses options */
	fprintf(f,"\n");
	fprintf(f,"osm2navit - parse osm textfile and converts to NavIt binfile format\n\n");
	fprintf(f,"Usage :\n");
	fprintf(f,"bzcat planet.osm.bz2 | osm2navit mymap.bin\n");
	fprintf(f,"Available switches:\n");
	fprintf(f,"-h (--help)              : this screen\n");
	fprintf(f,"-N (--nodes-only)        : process only nodes\n");
	fprintf(f,"-W (--ways-only)         : process only ways\n");
	fprintf(f,"-a (--attr-debug-level)  : control which data is included in the debug attribute\n");
	fprintf(f,"-c (--dump-coordinates)  : dump coordinates after phase 1\n");
#ifdef HAVE_POSTGRESQL
	fprintf(f,"-d (--db)                : get osm data out of a postgresql database with osm simple scheme and given connect string\n");
#endif
	fprintf(f,"-e (--end)               : end at specified phase\n");
	fprintf(f,"-k (--keep-tmpfiles)     : do not delete tmp files after processing. useful to reuse them\n\n");
	fprintf(f,"-o (--coverage)          : map every street to item coverage\n");
	fprintf(f,"-s (--start)             : start at specified phase\n");
	fprintf(f,"-i (--input-file)        : specify the input file name (OSM), overrules default stdin\n");
	fprintf(f,"-w (--dedupe-ways)       : ensure no duplicate ways or nodes. useful when using several input files\n");
	fprintf(f,"-z (--compression-level) : set the compression level\n");
	exit(1);
}

static void
process_binfile(FILE *in, FILE *out)
{
	struct item_bin *ib;
	while ((ib=read_item(in))) {
		fwrite(ib, (ib->len+1)*4, 1, out);
	}
}

static struct plugins *plugins;

static void add_plugin(char *path)
{
	struct attr **attrs;

	if (! plugins)
		plugins=plugins_new();
	attrs=(struct attr*[]){&(struct attr){attr_path,{path}},NULL};
	plugin_new(&(struct attr){attr_plugins,.u.plugins=plugins}, attrs);	
}

static FILE *
tempfile(char *suffix, char *name, int write)
{
	char buffer[4096];
	sprintf(buffer,"%s_%s.tmp",name, suffix);
	return fopen(buffer,write ? "wb+": "rb");
}

static void
tempfile_unlink(char *suffix, char *name)
{
	char buffer[4096];
	sprintf(buffer,"%s_%s.tmp",name, suffix);
	unlink(buffer);
}

static void
tempfile_rename(char *suffix, char *from, char *to)
{
	char buffer_from[4096],buffer_to[4096];
	sprintf(buffer_from,"%s_%s.tmp",from,suffix);
	sprintf(buffer_to,"%s_%s.tmp",to,suffix);
	assert(rename(buffer_from, buffer_to) == 0);
	
}

int main(int argc, char **argv)
{
	FILE *ways=NULL,*ways_split=NULL,*ways_split_index=NULL,*nodes=NULL,*turn_restrictions=NULL,*graph=NULL,*coastline=NULL,*tilesdir,*coords,*relations=NULL;
	char *map=g_strdup(attrmap);
	int zipnum,c,start=1,end=99,dump_coordinates=0;
	int keep_tmpfiles=0;
	int process_nodes=1, process_ways=1, process_relations=0;
#ifdef HAVE_ZLIB
	int compression_level=9;
#else
	int compression_level=0;
#endif
	int output=0;
	int input=0;
	char *result;
#ifdef HAVE_POSTGRESQL
	char *dbstr=NULL;
#endif
	FILE* input_file = stdin;
	struct attr **attrs;
	struct map *map_handle=NULL;
#if 0
	char *suffixes[]={"m0l0", "m0l1","m0l2","m0l3","m0l4","m0l5","m0l6"};
#else
	char *suffixes[]={""};
#endif
	int suffix_count=sizeof(suffixes)/sizeof(char *);
	int i;
	main_init(argv[0]);
	struct zip_info zip_info;

	while (1) {
#if 0
		int this_option_optind = optind ? optind : 1;
#endif
		int option_index = 0;
		static struct option long_options[] = {
			{"attr-debug-level", 1, 0, 'a'},
			{"binfile", 0, 0, 'b'},
			{"compression-level", 1, 0, 'z'},
			{"coverage", 0, 0, 'o'},
#ifdef HAVE_POSTGRESQL
			{"db", 1, 0, 'd'},
#endif
			{"dedupe-ways", 0, 0, 'w'},
			{"dump", 0, 0, 'D'},
			{"dump-coordinates", 0, 0, 'c'},
			{"end", 1, 0, 'e'},
			{"help", 0, 0, 'h'},
			{"keep-tmpfiles", 0, 0, 'k'},
			{"nodes-only", 0, 0, 'N'},
			{"map", 1, 0, 'm'},
			{"plugin", 1, 0, 'p'},
			{"start", 1, 0, 's'},
			{"input-file", 1, 0, 'i'},
			{"ignore-unknown", 0, 0, 'n'},
			{"ways-only", 0, 0, 'W'},
			{"slice-size", 1, 0, 'S'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "DNWS:a:bc"
#ifdef HAVE_POSTGRESQL
					      "d:"
#endif
					      "e:hi:knm:p:s:wz:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'D':
			output=1;
			break;
		case 'N':
			process_ways=0;
			break;
		case 'R':
			process_relations=0;
			break;
		case 'S':
			slice_size=atoll(optarg);
			break;
		case 'W':
			process_nodes=0;
			break;
		case 'a':
			attr_debug_level=atoi(optarg);
			break;
		case 'b':
			input=1;
			break;
		case 'c':
			dump_coordinates=1;
			break;
#ifdef HAVE_POSTGRESQL
		case 'd':
			dbstr=optarg;
			break;
#endif
		case 'e':
			end=atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			break;
		case 'm':
			attrs=(struct attr*[]){
				&(struct attr){attr_type,{"textfile"}},
				&(struct attr){attr_data,{optarg}},
				NULL};
			add_plugin("$NAVIT_LIBDIR/*/${NAVIT_LIBPREFIX}libmap_textfile.so");
			map_handle=map_new(NULL, attrs);
			break;	
		case 'n':
			fprintf(stderr,"I will IGNORE unknown types\n");
			ignore_unkown=1;
			break;
		case 'k':
			fprintf(stderr,"I will KEEP tmp files\n");
			keep_tmpfiles=1;
			break;
		case 'o':
			coverage=1;
			break;
		case 'p':
			add_plugin(optarg);
			break;
		case 's':
			start=atoi(optarg);
			break;
		case 'w':
			dedupe_ways_hash=g_hash_table_new(NULL, NULL);
			break;
		case 'i':
			input_file = fopen( optarg, "r" );
			if ( input_file ==  NULL )
			{
			    fprintf( stderr, "\nInput file (%s) not found\n", optarg );
			    exit( -1 );
			}
			break;
#ifdef HAVE_ZLIB
		case 'z':
			compression_level=atoi(optarg);
			break;
#endif
		case '?':
			usage(stderr);
			break;
		default:
			fprintf(stderr,"c=%d\n", c);
		}

	}
	if (optind != argc-(output == 1 ? 0:1))
		usage(stderr);
	if (plugins)
		plugins_init(plugins);
	result=argv[optind];
	build_attrmap(map);
	build_countrytable();


	if (input == 0) {
	if (start == 1) {
		unlink("coords.tmp");
		if (process_ways)
			ways=tempfile(suffix,"ways",1);
		if (process_nodes)
			nodes=tempfile(suffix,"nodes",1);
		if (process_ways && process_nodes) 
			turn_restrictions=tempfile(suffix,"turn_restrictions",1);
		phase=1;
		fprintf(stderr,"PROGRESS: Phase 1: collecting data\n");
#ifdef HAVE_POSTGRESQL
		if (dbstr) 
			phase1_db(dbstr,ways,nodes);
		else
#endif
		if (map_handle) {
			phase1_map(map_handle,ways,nodes);
			map_destroy(map_handle);
		}
		else
			phase1(input_file,ways,nodes,turn_restrictions);
		if (slices) {
			fprintf(stderr,"%d slices\n",slices);
			flush_nodes(1);
			for (i = slices-2 ; i>=0 ; i--) {
				fprintf(stderr, "slice %d of %d\n",slices-i-1,slices-1);
				load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
				resolve_ways(ways, NULL);
				save_buffer("coords.tmp",&node_buffer, i*slice_size);
			}
		} else
			save_buffer("coords.tmp",&node_buffer, 0);
		if (ways)
			fclose(ways);
		if (nodes)
			fclose(nodes);
		if (turn_restrictions)
			fclose(turn_restrictions);
	}
	if (!slices) {
		if (end == 1 || dump_coordinates)
			flush_nodes(1);
		else
			slices++;
	}
	if (end == 1)
		exit(0);
	if (start == 2) {
		load_buffer("coords.tmp",&node_buffer,0, slice_size);
	}
	if (start <= 2) {
		if (process_ways) {
			ways=tempfile(suffix,"ways",0);
			phase=2;
			fprintf(stderr,"PROGRESS: Phase 2: finding intersections\n");
			for (i = 0 ; i < slices ; i++) {
				int final=(i >= slices-1);
				ways_split=tempfile(suffix,"ways_split",1);
				ways_split_index=final ? tempfile(suffix,"ways_split_index",1) : NULL;
				graph=tempfile(suffix,"graph",1);
				/* coastline=tempfile(suffix,"coastline",1); */
				if (i) 
					load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
				phase2(ways,ways_split,ways_split_index,graph,coastline,final);
				fclose(ways_split);
				if (ways_split_index)
					fclose(ways_split_index);
				fclose(ways);
				fclose(graph);
				if (! final) {
					tempfile_rename(suffix,"ways_split","ways_to_resolve");
					ways=tempfile(suffix,"ways_to_resolve",0);
				}
			}
			if(!keep_tmpfiles)
				tempfile_unlink(suffix,"ways");
			tempfile_unlink(suffix,"ways_to_resolve");
		} else
			fprintf(stderr,"PROGRESS: Skipping Phase 2\n");
	}
	free(node_buffer.base);
	node_buffer.base=NULL;
	node_buffer.malloced=0;
	node_buffer.size=0;
	if (end == 2)
		exit(0);
	} else {
		ways_split=tempfile(suffix,"ways_split",0);
		process_binfile(stdin, ways_split);
		fclose(ways_split);
	}
	if (start <= 3) {
		fprintf(stderr,"PROGRESS: Phase 3: sorting countries, generating turn restrictions\n");
		sort_countries(keep_tmpfiles);
		if (process_relations) {
			turn_restrictions=tempfile(suffix,"turn_restrictions",0);
			relations=tempfile(suffix,"relations",1);
			coords=fopen("coords.tmp","rb");
			ways_split=tempfile(suffix,"ways_split",0);
			ways_split_index=tempfile(suffix,"ways_split_index",0);
			process_turn_restrictions(turn_restrictions,coords,ways_split,ways_split_index,relations);
			fclose(ways_split_index);
			fclose(ways_split);
			fclose(coords);
			fclose(relations);
			fclose(turn_restrictions);
			if(!keep_tmpfiles)
				tempfile_unlink(suffix,"turn_restrictions");
		}
		if(!keep_tmpfiles)
			tempfile_unlink(suffix,"ways_split_index");
	}
	if (end == 3)
		exit(0);
	if (output == 1) {
		fprintf(stderr,"PROGRESS: Phase 4: dumping\n");
		if (process_nodes) {
			nodes=tempfile(suffix,"nodes",0);
			if (nodes) {
				dump(nodes);
				fclose(nodes);
			}
		}
		if (process_ways) {
			ways_split=tempfile(suffix,"ways_split",0);
			if (ways_split) {
				dump(ways_split);
				fclose(ways_split);
			}
		}
		if (process_relations) {
			relations=tempfile(suffix,"relations",0);
			fprintf(stderr,"Relations=%p\n",relations);
			if (relations) {
				dump(relations);
				fclose(relations);
			}
		}
		exit(0);
	}
	for (i = 0 ; i < suffix_count ; i++) {
		suffix=suffixes[i];
	if (start <= 4) {
		phase=3;
		if (i == 0) {
			memset(&zip_info, 0, sizeof(zip_info));
		}
		zipnum=zip_info.zipnum;
		fprintf(stderr,"PROGRESS: Phase 4: generating tiles %s\n",suffix);
		if (process_relations)
			relations=tempfile(suffix,"relations",0);
		if (process_ways)
			ways_split=tempfile(suffix,"ways_split",0);
		if (process_nodes)
			nodes=tempfile(suffix,"nodes",0);
		tilesdir=tempfile(suffix,"tilesdir",1);
		phase4(relations,ways_split,nodes,suffix,tilesdir,&zip_info);
		fclose(tilesdir);
		if (nodes)
			fclose(nodes);
		if (ways_split)
			fclose(ways_split);
		zip_info.zipnum=zipnum;
	}
	if (end == 4)
		exit(0);
	if (start <= 5) {
		phase=4;
		fprintf(stderr,"PROGRESS: Phase 5: assembling map %s\n",suffix);
		if (process_relations)
			relations=tempfile(suffix,"relations",0);
		if (process_ways)
			ways_split=tempfile(suffix,"ways_split",0);
		if (process_nodes)
			nodes=tempfile(suffix,"nodes",0);
		if (i == 0) {
			zip_info.dir_size=0;
			zip_info.offset=0;
			zip_info.maxnamelen=14+strlen(suffixes[0]);
			zip_info.compression_level=compression_level;
			zip_info.zipnum=0;
			zip_info.dir=tempfile("zipdir","",1);
			zip_info.index=tempfile("index","",1);
			zip_info.res=fopen(result,"wb+");
			index_init(&zip_info, 1);
		}
		phase5(relations,ways_split,nodes,suffix,&zip_info);
		if (nodes)
			fclose(nodes);
		if (ways_split)
			fclose(ways_split);
		if(!keep_tmpfiles) {
			tempfile_unlink(suffix,"relations");
			tempfile_unlink(suffix,"nodes");
			tempfile_unlink(suffix,"ways_split");
			tempfile_unlink(suffix,"turn_restrictions");
			tempfile_unlink(suffix,"graph");
			tempfile_unlink(suffix,"tilesdir");
			tempfile_unlink("zipdir","");
			unlink("coords.tmp");
		}
		if (i == suffix_count-1) {
	zipnum=zip_info.zipnum;
	write_countrydir(&zip_info);
	zip_info.zipnum=zipnum;
	write_aux_tiles(&zip_info);
	write_index(&zip_info);
	phase5_write_directory(&zip_info);
	fclose(zip_info.index);
	fclose(zip_info.dir);
	fclose(zip_info.res);
	if (!keep_tmpfiles) {
		remove_countryfiles();
		tempfile_unlink("index","");
	}
		}
	}
	}
	return 0;
}
