#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libintl.h>
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

#include "statusbar.h"


#define _(STRING) gettext(STRING)

struct statusbar_priv {
	struct gui_priv *gui;
	GtkWidget *hbox;
	char gps_text[128];
	GtkWidget *gps;
	char route_text[128];
	GtkWidget *route;
	struct callback *vehicle_cb;
};




static void
statusbar_destroy(struct statusbar_priv *this)
{
	g_free(this);
}

static void
statusbar_gps_update(struct statusbar_priv *this, int sats, int qual, double lng, double lat, double height, double direction, double speed)
{
	char lat_c='N';
	char lng_c='E';
    char *dirs[]={_("N"),_("NE"),_("E"),_("SE"),_("S"),_("SW"),_("W"),_("NW"),_("N")};
	char *dir;
	int dir_idx;

	if (lng < 0) {
		lng=-lng;
		lng_c='W';
	}
	if (lat < 0) {
		lat=-lat;
		lat_c='S';
	}
	dir_idx=(direction+22.5)/45;
	dir=dirs[dir_idx];
	sprintf(this->gps_text, "GPS %2d/%1d %02.0f%07.4f%c %03.0f%07.4f%c %4.0fm %3.0f°%-2s %3.0fkm/h", sats, qual, floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c, height, direction, dir, speed);
	gtk_label_set_text(GTK_LABEL(this->gps), this->gps_text);

}

static void
statusbar_route_update(struct statusbar_priv *this, struct navit *navit, struct vehicle *v)
{
	struct navigation *nav=NULL;
	struct map *map=NULL;
	struct map_rect *mr=NULL;
	struct item *item=NULL;
	struct attr attr;
	double route_len=0;
	time_t eta;
	struct tm *eta_tm=NULL;
	char buffer[128];
	double lng, lat, direction=0, height=0, speed=0;
	int sats=0, qual=0;
	char lat_c='N';
	char lng_c='E';
    char *dirs[]={_("N"),_("NE"),_("E"),_("SE"),_("S"),_("SW"),_("W"),_("NW"),_("N")};
	char *dir;
	int dir_idx;

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
	sprintf(buffer,_("Route %4.0fkm    %02d:%02d ETA" ),route_len/1000, eta_tm ? eta_tm->tm_hour : 0 , eta_tm ? eta_tm->tm_min : 0);
	if (strcmp(buffer, this->route_text)) {
		strcpy(this->route_text, buffer);
		gtk_label_set_text(GTK_LABEL(this->route), this->route_text);
	}
	if (!vehicle_position_attr_get(v, attr_position_coord_geo, &attr))
		return;
	lng=attr.u.coord_geo->lng;
	lat=attr.u.coord_geo->lat;
	if (lng < 0) {
		lng=-lng;
		lng_c='W';
	}
	if (lat < 0) {
		lat=-lat;
		lat_c='S';
	}
	if (vehicle_position_attr_get(v, attr_position_direction, &attr))
		direction=*(attr.u.numd);
	dir_idx=(direction+22.5)/45;
	dir=dirs[dir_idx];
	if (vehicle_position_attr_get(v, attr_position_height, &attr))
		height=*(attr.u.numd);
	if (vehicle_position_attr_get(v, attr_position_speed, &attr))
		speed=*(attr.u.numd);
	if (vehicle_position_attr_get(v, attr_position_sats_used, &attr))
		sats=attr.u.num;
	if (vehicle_position_attr_get(v, attr_position_qual, &attr))
		qual=attr.u.num;
	sprintf(this->gps_text,"GPS %2d/%1d %02.0f%07.4f%c %03.0f%07.4f%c %4.0fm %3.0f°%-2s %3.0fkm/h", sats, qual, floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c, height, direction, dir, speed);
	gtk_label_set_text(GTK_LABEL(this->gps), this->gps_text);
}

static struct statusbar_methods methods = {
	statusbar_destroy,
};

struct statusbar_priv *
gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth)
{
	struct statusbar_priv *this=g_new0(struct statusbar_priv, 1);

	this->gui=gui;
	*meth=methods;

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
	this->vehicle_cb=callback_new_1(callback_cast(statusbar_route_update), this);
	navit_add_vehicle_cb(gui->nav, this->vehicle_cb);
	return this;
}

