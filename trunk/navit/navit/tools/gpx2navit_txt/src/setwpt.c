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

#include "gpx2navit_txt.h"

void setWpt(parsedata * pdata);

/**
 * save point objects in *_wpt or *_pnt file
 */
void setWpt( parsedata * pdata)
{
    double x[1], y[1], z[1];
    x[0] = pdata->attr->lon;
    y[0] = pdata->attr->lat;
    z[0] = pdata->attr->ele;
    if (pdata->prop->is3d) {
	//nothing at the moment		    
    } else {
	//do not know if i will change something here
    }
    // Write attributes to file first line waypoint-info, second line coords
    char poi_type[20]="poi_attraction";
    if (!strcmp(pdata->attr->type,"Geocache|Traditional Cache"))
	strcpy(poi_type,"poi_gc_tradi");
    if (!strcmp(pdata->attr->type,"Geocache|Multi-cache"))
	strcpy(poi_type,"poi_gc_multi");
    if (!strcmp(pdata->attr->type,"Geocache|Unknown Cache"))
	strcpy(poi_type,"poi_gc_mystery");
    if (!strcmp(pdata->attr->type,"Geocache|Event Cache"))
	strcpy(poi_type,"poi_gc_event");
    if (!strcmp(pdata->attr->type,"Geocache")) //for OC
	strcpy(poi_type,"poi_gc_tradi");
    if (!strcmp(pdata->attr->type,"Waypoint|Parking Area")) 
	strcpy(poi_type,"poi_car_parking");
    if (!strcmp(pdata->attr->type,"Waypoint|Question to Answer")) 
	strcpy(poi_type,"poi_gc_question");
    if (!strcmp(pdata->attr->type,"Waypoint|Reference Point")) 
	strcpy(poi_type,"poi_gc_reference");
    if (!strcmp(pdata->attr->type,"Waypoint|Stages of a Multicache")) 
	strcpy(poi_type,"poi_gc_stages");

    //generate the file
    fprintf(pdata->fp,"type=%s label=\"%s\" description=\"%s\" gc_type=\"%s\"\n",
	poi_type,pdata->attr->name,pdata->attr->desc,pdata->attr->type);
    fprintf(pdata->fp,"%3.6f %4.6f\n",x[0],y[0]);

    //writeWptAttribute(hDBF, pdata, iShape);
    if (!strcmp(pdata->current->name, "wpt")) {
	pdata->prop->stats->wptpoints++;
    }
    return;
}


