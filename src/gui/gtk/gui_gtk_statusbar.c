#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "route.h"
#include "transform.h"
#include "gui_gtk.h"

#include "statusbar.h"

struct statusbar_gui {
	GtkWidget *hbox;
	char mouse_text[128];
	GtkWidget *mouse;
	char gps_text[128];
	GtkWidget *gps;	
	char route_text[128];
	GtkWidget *route;
};


void
statusbar_destroy(struct statusbar *this)
{
	g_free(this->gui);
	g_free(this);
}

void
statusbar_mouse_update(struct statusbar *this, struct transformation *tr, struct point *p)
{
	struct coord c;
	struct coord_geo g;
	char buffer[128];

	transform_reverse(tr, p, &c);
	transform_lng_lat(&c, &g);
	transform_geo_text(&g, buffer);
	sprintf(this->gui->mouse_text,"M: %s", buffer);
	gtk_label_set_text(GTK_LABEL(this->gui->mouse), this->gui->mouse_text);
}

void
statusbar_gps_update(struct statusbar *this, int sats, int qual, double lng, double lat, double height, double direction, double speed)
{
	char lat_c='N';
	char lng_c='E';
	char *dirs[]={"N","NO","O","SO","S","SW","W","NW","N"};
	char *dir;
	char *utf8;
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
	sprintf(this->gui->gps_text,"GPS %2d/%1d %02.0f%07.4f%c %03.0f%07.4f%c %4.0fm %3.0f°%-2s %3.0fkm/h", sats, qual, floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c, height, direction, dir, speed);
	utf8=g_locale_to_utf8(this->gui->gps_text,-1,NULL,NULL,NULL);
	gtk_label_set_text(GTK_LABEL(this->gui->gps), utf8);
	g_free(utf8);
	
}

void
statusbar_route_update(struct statusbar *this, struct route *route)
{
	struct tm *eta_tm;
	double route_len;

	eta_tm=route_get_eta(route);
	route_len=route_get_len(route);
	
	sprintf(this->gui->route_text,"Route %4.0fkm    %02d:%02d ETA",route_len/1000, eta_tm->tm_hour, eta_tm->tm_min);
	gtk_label_set_text(GTK_LABEL(this->gui->route), this->gui->route_text);
}

struct statusbar *
gui_gtk_statusbar_new(GtkWidget **widget)
{
	struct statusbar *this=g_new0(struct statusbar, 1);
	char *utf8;

	this->gui=g_new0(struct statusbar_gui,1);
	this->statusbar_destroy=statusbar_destroy;
	this->statusbar_mouse_update=statusbar_mouse_update;
	this->statusbar_route_update=statusbar_route_update;
	this->statusbar_gps_update=statusbar_gps_update;

	this->gui->hbox=gtk_hbox_new(FALSE, 1);
	this->gui->mouse=gtk_label_new("M: 0000.0000N 00000.0000E");
	gtk_label_set_justify(GTK_LABEL(this->gui->mouse),  GTK_JUSTIFY_LEFT);
	utf8=g_locale_to_utf8("GPS 00/0 0000.0000N 00000.0000E 0000m 000°NO 000km/h",-1,NULL,NULL,NULL);
	this->gui->gps=gtk_label_new(utf8);
	g_free(utf8);
	gtk_label_set_justify(GTK_LABEL(this->gui->gps),  GTK_JUSTIFY_LEFT);
	this->gui->route=gtk_label_new("Route 0000km  0+00:00 ETA");
	gtk_label_set_justify(GTK_LABEL(this->gui->route),  GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(this->gui->hbox), this->gui->mouse, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(this->gui->hbox), gtk_vseparator_new(), TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(this->gui->hbox), this->gui->gps, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(this->gui->hbox), gtk_vseparator_new(), TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(this->gui->hbox), this->gui->route, TRUE, TRUE, 2);

	*widget=this->gui->hbox;
	return this;
}

