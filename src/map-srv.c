/*
 * CORBA map test
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Elliot Lee <sopwith@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "map.h"
#include "coord.h"
#include "route.h"
#include "transform.h"
#include "statusbar.h"
#include "map-share.h"
#include "main.h"
#include <math.h>

/**
   This is used by map-server.c and map-local.c
   It uses map-skels.c
**/

static Map the_map_client;
static CORBA_ORB the_orb;
static PortableServer_POA the_poa;
static PortableServer_ObjectId *the_objid;

#if 0
static GtkMap *global_map;
#endif

static CORBA_Object
do_mapString(PortableServer_Servant servant,
	     const CORBA_char * astring,
	     CORBA_double * outnum, CORBA_Environment * ev)
{

	g_message("[server] %s", astring);

	*outnum = rand() % 100;

	return CORBA_Object_duplicate(the_map_client, ev);
}

static void
do_PlaceFlag(PortableServer_Servant servant, CORBA_Environment * ev)
{

	g_message("[server PlaceFlag]");

}


static void
do_doNothing(PortableServer_Servant servant, CORBA_Environment * ev)
{
}

PortableServer_ServantBase__epv base_epv = {
	NULL,
	NULL,
	NULL
};

static PointObj
do_PointFromCoord(PortableServer_Servant _servant, const CORBA_char * coord, CORBA_Environment * ev)
{
	PointObj ret;
	double lat,lng,lat_deg,lng_deg;
	char lat_c,lng_c;
	
	printf("String=%s\n", coord);
	sscanf(coord,"%lf %c %lf %c",&lat, &lat_c, &lng, &lng_c);
	
	printf("lat=%f lng=%f\n", lat, lng);

	lat_deg=floor(lat/100);
	lat-=lat_deg*100;
	lat_deg+=lat/60;

	lng_deg=floor(lng/100);
	lng-=lng_deg*100;
	lng_deg+=lng/60;

	ret.lat=lat_deg;
	ret.lng=lng_deg;
	printf("lat_deg=%f lng_deg=%f\n", lat_deg, lng_deg);
	
	ret.height=0;
	return ret;
}

#if 0
static void
PointObj_to_coor(const PointObj *pnt, struct coord *c)
{
	double lng, lat;
	lng=pnt->lng;
	lat=pnt->lat;
	transform_mercator(&lng, &lat, c);
}
#endif

static void
do_View(PortableServer_Servant _servant, const PointObj *pnt, CORBA_Environment * ev)
{
#if 0
	unsigned long scale;
	struct coord c;
	GtkMap *map=global_map;

	map_get_view(map, NULL, NULL, &scale);
	PointObj_to_coor(pnt, &c);
	map_set_view(map, c.x, c.y, scale);
#endif
}


static void
do_Route(PortableServer_Servant _servant, const PointObj *src, const PointObj *dst, CORBA_Environment * ev)
{
#if 0
	struct coord c_src,c_dst;
	GtkMap *map=global_map;
	unsigned long scale;

	PointObj_to_coor(src, &c_src);
	PointObj_to_coor(dst, &c_dst);
	route_set_position(map->container->route, &c_src);
	route_set_destination(map->container->route, &c_dst);

	map_get_view(map, NULL, NULL, &scale);
	map_set_view(map, c_src.x, c_src.y, scale);

	if (map->container->statusbar && map->container->statusbar->statusbar_route_update)
        	map->container->statusbar->statusbar_route_update(map->container->statusbar, map->container->route);
#endif
}

POA_Mappel__epv mappel_epv = 	{
};

static Mappel do_Get(PortableServer_Servant _servant, CORBA_Environment * ev)
{
	Mappel retval=NULL;
#if 0
	impl_POA_Mappel *newservant;
	PortableServer_ObjectId *objid;

	printf("Do_Get\n");
	newservant = g_new0(impl_POA_Mappel, 1);
	newservant->servant.vepv = &mappel_epv;
	newservant->poa = poa;
	POA_Mappel__init((PortableServer_Servant) newservant, ev);
	objid = PortableServer_POA_activate_object(poa, newservant, ev);
	CORBA_free(objid);
	retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
#endif

	return retval;
}

POA_Map__epv map_epv = 	{
				mapString:	do_mapString, 
				doNothing:	do_doNothing,
				PlaceFlag:	do_PlaceFlag,
				PointFromCoord:	do_PointFromCoord,
				View:		do_View,
				Route:		do_Route,
				Get:		do_Get,
			};
POA_Map__vepv poa_map_vepv = { &base_epv, &map_epv };
POA_Map poa_map_servant = { NULL, &poa_map_vepv };


void map_srv_start_poa(CORBA_ORB orb, CORBA_Environment * ev)
{
	PortableServer_POAManager mgr;

	the_orb = orb;
	the_poa = (PortableServer_POA)
	    CORBA_ORB_resolve_initial_references(orb, "RootPOA", ev);

	mgr = PortableServer_POA__get_the_POAManager(the_poa, ev);
	PortableServer_POAManager_activate(mgr, ev);
	CORBA_Object_release((CORBA_Object) mgr, ev);
}

CORBA_Object map_srv_start_object(CORBA_Environment * ev, struct container *co)
{
	POA_Map__init(&poa_map_servant, ev);
	if (ev->_major) {
		printf("object__init failed: %d\n", ev->_major);
		exit(1);
	}
	the_objid = PortableServer_POA_activate_object(the_poa,
						       &poa_map_servant,
						       ev);
	if (ev->_major) {
		printf("activate_object failed: %d\n", ev->_major);
		exit(1);
	}
	the_map_client = PortableServer_POA_servant_to_reference(the_poa,
								 &poa_map_servant,
								 ev);
	if (ev->_major) {
		printf("servant_to_reference failed: %d\n", ev->_major);
		exit(1);
	}
#if 0
	global_map=map;
#endif
	return the_map_client;
}

#if 0
static void map_srv_finish_object(CORBA_Environment * ev)
{
	CORBA_Object_release(the_map_client, ev);
	if (ev->_major) {
		printf("object_release failed: %d\n", ev->_major);
		exit(1);
	}
	the_map_client = 0;
	PortableServer_POA_deactivate_object(the_poa, the_objid, ev);
	if (ev->_major) {
		printf("deactivate_object failed: %d\n", ev->_major);
		exit(1);
	}
	CORBA_free(the_objid);
	the_objid = 0;
	POA_Map__fini(&poa_map_servant, ev);
	if (ev->_major) {
		printf("object__fini failed: %d\n", ev->_major);
		exit(1);
	}
}


static void map_srv_finish_poa(CORBA_Environment * ev)
{
	CORBA_Object_release((CORBA_Object) the_poa, ev);
	if (ev->_major) {
		printf("POA release failed: %d\n", ev->_major);
		exit(1);
	}
	the_poa = 0;
}
#endif

