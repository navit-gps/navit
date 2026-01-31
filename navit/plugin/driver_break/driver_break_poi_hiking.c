/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "driver_break_poi_hiking.h"
#include "driver_break_poi.h"
#include "driver_break_poi_map.h"
#include "mapset.h"
#include "navit.h"

/* Forward declaration - navit_get_default may not be exported */
struct navit *navit_get_default(void);

/* Compare function for sorting water points by distance */
static int poi_compare_water_distance(const struct water_point *a, 
                                       const struct water_point *b) {
    if (a->distance_from_driver_break_stop < b->distance_from_driver_break_stop) return -1;
    if (a->distance_from_driver_break_stop > b->distance_from_driver_break_stop) return 1;
    return 0;
}

/* Compare function for sorting cabins by distance (prioritize network cabins if DNT priority) */
static int poi_compare_cabin_distance(const struct cabin_hut *a, 
                                       const struct cabin_hut *b) {
    if (!a || !b) return 0;
    
    /* Prioritize network cabins (DNT/STF) */
    if (a->is_network && !b->is_network) return -1;
    if (!a->is_network && b->is_network) return 1;
    
    /* Then sort by distance */
    if (a->distance_from_driver_break_stop < b->distance_from_driver_break_stop) return -1;
    if (a->distance_from_driver_break_stop > b->distance_from_driver_break_stop) return 1;
    return 0;
}

/* Calculate distance between two coordinates in meters */
static double coord_distance_poi(struct coord_geo *c1, struct coord_geo *c2) {
    double lat1 = c1->lat * M_PI / 180.0;
    double lat2 = c2->lat * M_PI / 180.0;
    double dlat = (c2->lat - c1->lat) * M_PI / 180.0;
    double dlng = (c2->lng - c1->lng) * M_PI / 180.0;
    
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlng/2) * sin(dlng/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return 6371000.0 * c; /* Earth radius in meters */
}

/* Search for water points using Overpass API */
GList *poi_search_water_points(struct coord_geo *driver_break_stop, double radius_km) {
    GList *water_points = NULL;
    
    if (!driver_break_stop || radius_km <= 0) {
        return NULL;
    }
    
    /* POI categories for water points */
    const char *water_categories[] = {
        "amenity=drinking_water",
        "amenity=fountain",
        "natural=spring"
    };
    
    /* Use existing POI discovery with water-specific categories */
    GList *pois = driver_break_poi_discover(driver_break_stop, (int)radius_km, 
                                    water_categories, 
                                    sizeof(water_categories)/sizeof(water_categories[0]));
    
    if (pois) {
        GList *l = pois;
        while (l) {
            struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
            if (poi) {
                struct water_point *wp = g_new0(struct water_point, 1);
                wp->coord = poi->coord;
                wp->name = poi->name ? g_strdup(poi->name) : NULL;
                wp->type = poi->category ? g_strdup(poi->category) : NULL;
                wp->distance_from_driver_break_stop = coord_distance_poi(driver_break_stop, &poi->coord);
                wp->is_spring = (poi->category && strstr(poi->category, "spring")) ? 1 : 0;
                
                if (wp->is_spring) {
                    wp->warning = g_strdup("Natural spring - drink at your own risk");
                }
                
                water_points = g_list_append(water_points, wp);
            }
            l = g_list_next(l);
        }
        
        /* Sort by distance */
        water_points = g_list_sort(water_points, (GCompareFunc)poi_compare_water_distance);
        
        driver_break_poi_free_list(pois);
    }
    
    return water_points;
}

/* Search for water points using map data */
GList *poi_search_water_points_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms) {
    GList *water_points = NULL;
    
    if (!driver_break_stop || !ms || radius_km <= 0) {
        return NULL;
    }
    
    /* Use map-based POI search */
    GList *pois = driver_break_poi_map_search_water(driver_break_stop, radius_km, ms);
    if (pois) {
        GList *l = pois;
        while (l) {
            struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
            if (poi) {
                struct water_point *wp = g_new0(struct water_point, 1);
                wp->coord = poi->coord;
                wp->name = poi->name ? g_strdup(poi->name) : NULL;
                wp->type = poi->category ? g_strdup(poi->category) : NULL;
                wp->distance_from_driver_break_stop = coord_distance_poi(driver_break_stop, &poi->coord);
                
                /* Check if it's a spring (may need treatment) */
                if (poi->category && strstr(poi->category, "spring")) {
                    wp->is_spring = 1;
                    wp->warning = g_strdup("Natural spring - may need treatment");
                }
                
                water_points = g_list_append(water_points, wp);
            }
            l = g_list_next(l);
        }
        
        /* Sort by distance */
        water_points = g_list_sort(water_points, (GCompareFunc)poi_compare_water_distance);
        
        driver_break_poi_free_list(pois);
    }
    
    return water_points;
}

/* Search for cabins/huts using map data (preferred) or Overpass API (fallback) */
GList *poi_search_cabins(struct coord_geo *driver_break_stop, double radius_km) {
    GList *cabins = NULL;
    
    if (!driver_break_stop || radius_km <= 0) {
        return NULL;
    }
    
    /* Use Overpass API as fallback (map-based search requires mapset parameter) */
    /* Call poi_search_cabins_map if you have mapset available */
    const char *cabin_categories[] = {
        "tourism=wilderness_hut",
        "tourism=alpine_hut",
        "tourism=hostel",
        "tourism=camp_site"
    };
    
    GList *pois = driver_break_poi_discover(driver_break_stop, (int)radius_km, 
                                    cabin_categories, 
                                    sizeof(cabin_categories)/sizeof(cabin_categories[0]));
    
    if (pois) {
        GList *l = pois;
        while (l) {
            struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
            if (poi) {
                struct cabin_hut *cabin = g_new0(struct cabin_hut, 1);
                cabin->coord = poi->coord;
                cabin->name = poi->name ? g_strdup(poi->name) : NULL;
                cabin->type = poi->category ? g_strdup(poi->category) : NULL;
                cabin->distance_from_driver_break_stop = coord_distance_poi(driver_break_stop, &poi->coord);
                
                /* Check for DNT/network cabins from name (limited detection) */
                if (poi->name && (strstr(poi->name, "DNT") || strstr(poi->name, "dnt"))) {
                    cabin->is_dnt = 1;
                    cabin->is_network = 1;
                    cabin->network = g_strdup("DNT");
                } else if (poi->name && (strstr(poi->name, "STF") || strstr(poi->name, "stf"))) {
                    cabin->is_network = 1;
                    cabin->network = g_strdup("STF");
                }
                
                cabins = g_list_append(cabins, cabin);
            }
            l = g_list_next(l);
        }
        
        /* Sort by distance */
        cabins = g_list_sort(cabins, (GCompareFunc)poi_compare_cabin_distance);
        
        driver_break_poi_free_list(pois);
    }
    
    return cabins;
}

/* Search for cabins/huts using map data with DNT/network detection */
GList *poi_search_cabins_map(struct coord_geo *driver_break_stop, double radius_km, struct mapset *ms, int enable_dnt_priority) {
    GList *cabins = NULL;
    
    if (!driver_break_stop || !ms || radius_km <= 0) {
        return NULL;
    }
    
    /* Adjust search radius for network huts when DNT priority enabled */
    double search_radius = radius_km;
    if (enable_dnt_priority) {
        /* Use 25 km for network huts (matches BRouter) */
        search_radius = 25.0;
    }
    
    GList *pois = driver_break_poi_map_search_cabins(driver_break_stop, search_radius, ms);
    if (pois) {
        GList *l = pois;
        while (l) {
            struct driver_break_poi *poi = (struct driver_break_poi *)l->data;
            if (poi) {
                struct cabin_hut *cabin = g_new0(struct cabin_hut, 1);
                cabin->coord = poi->coord;
                cabin->name = poi->name ? g_strdup(poi->name) : NULL;
                cabin->type = poi->category ? g_strdup(poi->category) : NULL;
                cabin->distance_from_driver_break_stop = coord_distance_poi(driver_break_stop, &poi->coord);
                
                /* Check for DNT/network in name (network detection done during map search) */
                if (poi->name) {
                    char *name_lower = g_ascii_strdown(poi->name, -1);
                    if (strstr(name_lower, "[dnt]") || strstr(name_lower, "dnt")) {
                        cabin->is_dnt = 1;
                        cabin->is_network = 1;
                        cabin->network = g_strdup("DNT");
                    } else if (strstr(name_lower, "[stf]") || strstr(name_lower, "stf")) {
                        cabin->is_network = 1;
                        cabin->network = g_strdup("STF");
                    }
                    g_free(name_lower);
                }
                
                cabins = g_list_append(cabins, cabin);
            }
            l = g_list_next(l);
        }
        
        /* Sort by distance (network cabins prioritized automatically in compare function) */
        cabins = g_list_sort(cabins, (GCompareFunc)poi_compare_cabin_distance);
        
        driver_break_poi_free_list(pois);
    }
    
    return cabins;
}

void poi_free_water_points(GList *water_points) {
    GList *l = water_points;
    while (l) {
        poi_free_water_point((struct water_point *)l->data);
        l = g_list_next(l);
    }
    g_list_free(water_points);
}

void poi_free_water_point(struct water_point *wp) {
    if (wp) {
        g_free(wp->name);
        g_free(wp->type);
        g_free(wp->warning);
        g_free(wp);
    }
}

void poi_free_cabins(GList *cabins) {
    GList *l = cabins;
    while (l) {
        poi_free_cabin((struct cabin_hut *)l->data);
        l = g_list_next(l);
    }
    g_list_free(cabins);
}

void poi_free_cabin(struct cabin_hut *cabin) {
    if (cabin) {
        g_free(cabin->name);
        g_free(cabin->type);
        g_free(cabin->network);
        g_free(cabin);
    }
}
