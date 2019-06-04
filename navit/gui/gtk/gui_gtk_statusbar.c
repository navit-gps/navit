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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include "item.h"
#include "coord.h"
#include "debug.h"
#include "vehicle.h"
#include "callback.h"
#include "route.h"
#include "transform.h"
#include "navit.h"
#include "map.h"
#include "navigation.h"
#include "gui_gtk.h"
#include "navit_nls.h"

struct statusbar_priv {
    struct gui_priv *gui;
    GtkWidget *hbox;
    char gps_text[128];
    GtkWidget *gps;
    char route_text[128];
    GtkWidget *route;
    struct callback *vehicle_cb;
};

#if 0
static void statusbar_destroy(struct statusbar_priv *this) {
    g_free(this);
}

static void statusbar_gps_update(struct statusbar_priv *this, int sats, int qual, double lng, double lat, double height,
                                 double direction, double speed) {
    char *dirs[]= {_("N"),_("NE"),_("E"),_("SE"),_("S"),_("SW"),_("W"),_("NW"),_("N")};
    char *dir;
    int dir_idx;
    char pos_text[36];

    coord_format(lat,lng,DEGREES_MINUTES_SECONDS,pos_text,sizeof(pos_text));
    dir=dirs[dir_idx];
    sprintf(this->gps_text, "GPS %02d/%02d %s %4.0fm %3.0f°%-2s %3.0fkm/h", sats, qual, pos_text, height, direction, dir,
            speed);
    gtk_label_set_text(GTK_LABEL(this->gps), this->gps_text);

}
#endif

static const char *status_fix2str(int type) {
    switch(type) {
    case 0:
        return _("No");
    case 1:
        return _("2D");
    case 3:
        return _("3D");
    default:
        return _("OT");
    }
}

static void statusbar_route_update(struct statusbar_priv *this, struct navit *navit, struct vehicle *v) {
    struct navigation *nav=NULL;
    struct map *map=NULL;
    struct map_rect *mr=NULL;
    struct item *item=NULL;
    struct attr attr;
    double route_len=0;     /* Distance to destination. We get it in kilometers. */
    time_t eta;
    struct tm *eta_tm=NULL;
    char buffer[128];
    double lng, lat, direction=0, height=0, speed=0, hdop=0;
    int sats=0, qual=0;
    int status=0;
    const char *dirs[]= {_("N"),_("NE"),_("E"),_("SE"),_("S"),_("SW"),_("W"),_("NW"),_("N")};
    const char *dir;
    int dir_idx;

    /* Respect the Imperial attribute as we enlighten the user. */
    int imperial = FALSE;  /* default to using metric measures. */
    if (navit_get_attr(navit, attr_imperial, &attr, NULL))
        imperial=attr.u.num;

    if (navit)
        nav=navit_get_navigation(navit);
    if (nav)
        map=navigation_get_map(nav);
    if (map)
        mr=map_rect_new(map, NULL);
    if (mr)
        item=map_rect_get_item(mr);
    if (item) {
        if (item_attr_get(item, attr_destination_length, &attr))
            route_len=attr.u.num;
        if (item_attr_get(item, attr_destination_time, &attr)) {
            eta=time(NULL)+attr.u.num/10;
            eta_tm=localtime(&eta);
        }
    }
    if (mr)
        map_rect_destroy(mr);

    sprintf(buffer,_("Route %4.1f%s    %02d:%02d ETA" ),
            imperial == TRUE ? route_len * (KILOMETERS_TO_MILES/1000.00) : route_len/1000,
            imperial == TRUE ? "mi" : "km",
            eta_tm ? eta_tm->tm_hour : 0,
            eta_tm ? eta_tm->tm_min : 0);

    if (strcmp(buffer, this->route_text)) {
        strcpy(this->route_text, buffer);
        gtk_label_set_text(GTK_LABEL(this->route), this->route_text);
    }
    if (!vehicle_get_attr(v, attr_position_coord_geo, &attr, NULL))
        return;
    lng=attr.u.coord_geo->lng;
    lat=attr.u.coord_geo->lat;
    if (vehicle_get_attr(v, attr_position_fix_type, &attr, NULL))
        status=attr.u.num;
    if (vehicle_get_attr(v, attr_position_direction, &attr, NULL))
        direction=*(attr.u.numd);
    direction=fmod(direction,360);
    if (direction < 0)
        direction+=360;
    dir_idx=(direction+22.5)/45;
    dir=dirs[dir_idx];
    if (vehicle_get_attr(v, attr_position_height, &attr, NULL))
        height=*(attr.u.numd);
    if (vehicle_get_attr(v, attr_position_hdop, &attr, NULL))
        hdop=*(attr.u.numd);
    if (vehicle_get_attr(v, attr_position_speed, &attr, NULL))
        speed=*(attr.u.numd);
    if (vehicle_get_attr(v, attr_position_sats_used, &attr, NULL))
        sats=attr.u.num;
    if (vehicle_get_attr(v, attr_position_qual, &attr, NULL))
        qual=attr.u.num;
    coord_format(lat,lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));

    sprintf(this->gps_text,"GPS:%s %02d/%02d HD:%02.2f %s %4.0f%s %3.0f°%-2s %3.1f%s",
            status_fix2str(status),
            sats, qual, hdop, buffer,
            imperial ? height * FEET_PER_METER : height,
            imperial == TRUE ? "\'" : "m",
            direction, dir,
            imperial == TRUE ? speed * KILOMETERS_TO_MILES : speed,
            imperial == TRUE ? " mph" : "km/h"
           );

    gtk_label_set_text(GTK_LABEL(this->gps), this->gps_text);
}

struct statusbar_priv *
gui_gtk_statusbar_new(struct gui_priv *gui) {
    struct statusbar_priv *this=g_new0(struct statusbar_priv, 1);

    this->gui=gui;
    this->hbox=gtk_hbox_new(FALSE, 1);
    this->gps=gtk_label_new( "GPS 00/0 0000.0000N 00000.0000E 0000m 000°NO 000km/h" );
    gtk_label_set_justify(GTK_LABEL(this->gps),  GTK_JUSTIFY_LEFT);
    this->route=gtk_label_new( _( "Route 0000km  0+00:00 ETA" ) );
    gtk_label_set_justify(GTK_LABEL(this->route),  GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(this->hbox), this->gps, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(this->hbox), gtk_vseparator_new(), TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(this->hbox), this->route, TRUE, TRUE, 2);
    GTK_WIDGET_UNSET_FLAGS (this->hbox, GTK_CAN_FOCUS);

    gtk_box_pack_end(GTK_BOX(gui->vbox), this->hbox, FALSE, FALSE, 0);
    gtk_widget_show_all(this->hbox);
    /* add a callback for position updates */
    this->vehicle_cb=callback_new_attr_1(callback_cast(statusbar_route_update), attr_position_coord_geo, this);
    navit_add_callback(gui->nav, this->vehicle_cb);
    return this;
}

