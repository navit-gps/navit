#include <glib.h>
#include <stdlib.h>
#include <math.h>
#include "config.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "navit_nls.h"
#include "command.h"
#include "attr.h"
#include "event.h"
#include "config_.h"
#include "map.h"
#include "mapset.h"
#include "transform.h"
#include "search.h"
#include "route.h"
#include "vehicle.h"
#include "layout.h"
#include "util.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"
#include "gui_internal_search.h"
#include "gui_internal_poi.h"
#include "gui_internal_command.h"

extern char *version;

static char *
coordinates_geo(const struct coord_geo *gc, char sep)
{
	char latc='N',lngc='E';
	int lat_deg,lat_min,lat_sec;
	int lng_deg,lng_min,lng_sec;
	struct coord_geo g=*gc;

	if (g.lat < 0) {
		g.lat=-g.lat;
		latc='S';
	}
	if (g.lng < 0) {
		g.lng=-g.lng;
		lngc='W';
	}
	lat_sec=fmod(g.lat*3600+0.5,60);
	lat_min=fmod(g.lat*60-lat_sec/60.0+0.5,60);
	lat_deg=g.lat-lat_min/60.0-lat_sec/3600.0+0.5;
	lng_sec=fmod(g.lng*3600+0.5,60);
	lng_min=fmod(g.lng*60-lng_sec/60.0+0.5,60);
	lng_deg=g.lng-lng_min/60.0-lng_sec/3600.0+0.5;;
	return g_strdup_printf("%d°%d'%d\" %c%c%d°%d'%d\" %c",lat_deg,lat_min,lat_sec,latc,sep,lng_deg,lng_min,lng_sec,lngc);
}

char *
gui_internal_coordinates(struct pcoord *pc, char sep)
{
	struct coord_geo g;
	struct coord c;
	c.x=pc->x;
	c.y=pc->y;
	transform_to_geo(pc->pro, &c, &g);
	return coordinates_geo(&g, sep);

}

static void
gui_internal_cmd_escape(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	if (in && in[0] && ATTR_IS_STRING(in[0]->type) && out) {
		struct attr escaped;
		escaped.type=in[0]->type;
		escaped.u.str=g_strdup_printf("\"%s\"",in[0]->u.str);
		dbg(1,"result %s\n",escaped.u.str);
		*out=attr_generic_add_attr(*out, attr_dup(&escaped));
		g_free(escaped.u.str);
	}
}
static void
gui_internal_cmd2_about(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *menu,*wb,*w;
	char *text;

	graphics_draw_mode(this->gra, draw_mode_begin);
	menu=gui_internal_menu(this, _("About Navit"));
	menu->spx=this->spacing*10;
	wb=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand);
	gui_internal_widget_append(menu, wb);

	//Icon
	gui_internal_widget_append(wb, w=gui_internal_image_new(this, image_new_xs(this, "navit")));
	w->flags=gravity_top_center|orientation_horizontal|flags_fill;

	//app name
	text=g_strdup_printf("%s",PACKAGE_NAME);
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Version
	text=g_strdup_printf("%s",version);
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Site
	text=g_strdup_printf("http://www.navit-project.org/");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Authors
	text=g_strdup_printf("%s:",_("By"));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Martin Schaller");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Michael Farmbauer");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Alexander Atanasov");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Pierre Grandin");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);

	//Contributors
	text=g_strdup_printf("%s",_("And all the Navit Team"));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("%s",_("members and contributors."));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);

	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_cmd2_waypoints(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	gui_internal_select_waypoint(this, _("Waypoints"), NULL, NULL, gui_internal_cmd_position, (void*)2);
}

static void
gui_internal_cmd_enter_coord(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
        struct widget *w, *wb, *wk, *wr, *we, *wnext, *row;
        wb=gui_internal_menu(this, _("Enter Coordinates"));
	w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wr);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(wr, we);

/*
        w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
        gui_internal_widget_append(wb, w);

        we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
        gui_internal_widget_append(w, we);*/
        gui_internal_widget_append(we, wk=gui_internal_label_new(this, _("Longitude Latitude")));
        wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
        wk->background=this->background;
        wk->flags |= flags_expand|flags_fill;
        wk->func = gui_internal_call_linked_on_finish;
        gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
        wnext->state |= STATE_SENSITIVE;
        wnext->func = gui_internal_cmd_enter_coord_clicked; 
        wnext->data=wk;
        wk->data=wnext;
	row=gui_internal_text_new(this, _("Enter coordinates, for example:"), gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(wr,row);
	row=gui_internal_text_new(this, "52.5219N 19.4127E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(wr,row);
	row=gui_internal_text_new(this, "52°31.3167N 19°24.7667E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(wr,row);
	row=gui_internal_text_new(this, "52°31'19N 19°24'46E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(wr,row);

	if (this->keyboard)
                gui_internal_widget_append(w, gui_internal_keyboard(this,56));
       gui_internal_menu_render(this);
}

static void
gui_internal_cmd2_town(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	if (this->sl)
		search_list_select(this->sl, attr_country_all, 0, 0);
	gui_internal_search(this,_("Town"),"Town",1);
}

static void
gui_internal_cmd2_setting_vehicle(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr,attr2,vattr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;
	struct attr active_vehicle;
    
	iter=navit_attr_iter_new();
	if (navit_get_attr(this->nav, attr_vehicle, &attr, iter) && !navit_get_attr(this->nav, attr_vehicle, &attr2, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
		navit_attr_iter_destroy(iter);
		gui_internal_menu_vehicle_settings(this, attr.u.vehicle, vattr.u.str);
		return;
	}
	navit_attr_iter_destroy(iter);

	wb=gui_internal_menu(this, _("Vehicle"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
		active_vehicle.u.vehicle=NULL;
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_vehicle, &attr, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
		wl=gui_internal_button_new_with_callback(this, vattr.u.str,
			image_new_xs(this, attr.u.vehicle == active_vehicle.u.vehicle ? "gui_active" : "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_vehicle_settings, attr.u.vehicle);
		wl->text=g_strdup(vattr.u.str);
		gui_internal_widget_append(w, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd2_setting_rules(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *wb,*w;
	struct attr on,off;
	wb=gui_internal_menu(this, _("Rules"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_tracking;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Lock on road"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=0;
	off.u.num=-1;
	on.type=off.type=attr_orientation;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Northing"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_follow_cursor;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Map follows Vehicle"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_waypoints_flag;
	gui_internal_widget_append(w,
			gui_internal_button_navit_attr_new(this, _("Plan with Waypoints"), gravity_left_center|orientation_horizontal|flags_fill,
					&on, &off));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd2_setting_maps(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr, on, off, description, type, data, url, active;
	struct widget *w,*wb,*row,*wma;
	char *label;
	struct attr_iter *iter;

	wb=gui_internal_menu(this, _("Maps"));
	//w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	//w->spy=this->spacing*3;
	w = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	on.type=off.type=attr_active;
	on.u.num=1;
	off.u.num=0;
	while(navit_get_attr(this->nav, attr_map, &attr, iter)) {
		if (map_get_attr(attr.u.map, attr_description, &description, NULL)) {
			label=g_strdup(description.u.str);
		} else {
			if (!map_get_attr(attr.u.map, attr_type, &type, NULL))
				type.u.str="";
			if (!map_get_attr(attr.u.map, attr_data, &data, NULL))
				data.u.str="";
			label=g_strdup_printf("%s:%s", type.u.str, data.u.str);
		}
		if (map_get_attr(attr.u.map, attr_url, &url, NULL)) {
			if (!map_get_attr(attr.u.map, attr_active, &active, NULL))
				active.u.num=1;
			wma=gui_internal_button_new_with_callback(this, label, image_new_xs(this, active.u.num ? "gui_active" : "gui_inactive"),
			gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_map_download, attr.u.map);
		} else {
			wma=gui_internal_button_map_attr_new(this, label, gravity_left_center|orientation_horizontal|flags_fill,
				attr.u.map, &on, &off, 1);
		}	
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row, wma);
		g_free(label);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);

}

static void
gui_internal_cmd2_setting_layout_new(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr layout,clayout;
	struct attr_iter *iter;

	char *document=g_strdup("<html><a name='Layout' class='clist'><text>Layout</text>\n");
	navit_get_attr(this->nav, attr_layout, &clayout, NULL);
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_layout, &layout, iter)) {
		struct attr name;
		if (!layout_get_attr(layout.u.layout, attr_name, &name, NULL))
			name.u.str="Unknown";
		document=g_strconcat_printf(document, "<img class='centry' src='%s' onclick='set(\"navit.layout=navit.layout[@name==*]\",E(\"%s\"))'>%s</img>\n",layout.u.layout == clayout.u.layout ? "gui_active":"gui_inactive",name.u.str,name.u.str);
	}
	navit_attr_iter_destroy(iter);
	document=g_strconcat_printf(document, "</a></html>\n");
	gui_internal_html_menu(this, document, "Layout");
	g_free(document);
}

static void
gui_internal_cmd2_setting_layout(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr;
	struct widget *w,*wb,*wl,*row;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, _("Layout"));
	w=gui_internal_widget_table_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill,1);
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_layout, &attr, iter)) {
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		wl=gui_internal_button_navit_attr_new(this, attr.u.layout->name, gravity_left_center|orientation_horizontal|flags_fill,
			&attr, NULL);
		gui_internal_widget_append(row, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}
static void
gui_internal_cmd2_route_height_profile(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{


	struct widget * menu, *box;

	struct map * map=NULL;
	struct map_rect * mr=NULL;
	struct route * route;
	struct item * item =NULL;
	struct mapset *ms;
	struct mapset_handle *msh;
	int x,i,first=1,dist=0;
	struct coord c,last,res;
	struct coord_rect rbbox,dbbox;
	struct map_selection sel;
	struct heightline *heightline,*heightlines=NULL;
	struct diagram_point *min,*diagram_point,*diagram_points=NULL;
	sel.next=NULL;
	sel.order=18;
	sel.range.min=type_height_line_1;
	sel.range.max=type_height_line_3;


	menu=gui_internal_menu(this,_("Height Profile"));
	box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);
	gui_internal_widget_append(menu, box);
	route = navit_get_route(this->nav);
	if (route)
		map = route_get_map(route);
	if(map)
		mr = map_rect_new(map,NULL);
	if(mr) {
		while((item = map_rect_get_item(mr))) {
			while (item_coord_get(item, &c, 1)) {
				if (first) {
					first=0;
					sel.u.c_rect.lu=c;
					sel.u.c_rect.rl=c;
				} else
					coord_rect_extend(&sel.u.c_rect, &c);
			}
		}
		map_rect_destroy(mr);
		ms=navit_get_mapset(this->nav);
		if (!first && ms) {
			msh=mapset_open(ms);
			while ((map=mapset_next(msh, 1))) {
				mr=map_rect_new(map, &sel);
				if (mr) {
					while((item = map_rect_get_item(mr))) {
						if (item->type >= sel.range.min && item->type <= sel.range.max) {
							heightline=item_get_heightline(item);
							if (heightline) {
								heightline->next=heightlines;
								heightlines=heightline;
							}
						}
					}
					map_rect_destroy(mr);
				}
			}
			mapset_close(msh);
		}
	}
	map=NULL;
	mr=NULL;
	if (route)
		map = route_get_map(route);
	if(map)
		mr = map_rect_new(map,NULL);
	if(mr && heightlines) {
		while((item = map_rect_get_item(mr))) {
			first=1;
			while (item_coord_get(item, &c, 1)) {
				if (first)
					first=0;
				else {
					heightline=heightlines;
					rbbox.lu=last;
					rbbox.rl=last;
					coord_rect_extend(&rbbox, &c);
					while (heightline) {
						if (coord_rect_overlap(&rbbox, &heightline->bbox)) {
							for (i = 0 ; i < heightline->count - 1; i++) {
								if (heightline->c[i].x != heightline->c[i+1].x || heightline->c[i].y != heightline->c[i+1].y) {
									if (line_intersection(heightline->c+i, heightline->c+i+1, &last, &c, &res)) {
										diagram_point=g_new(struct diagram_point, 1);
										diagram_point->c.x=dist+transform_distance(projection_mg, &last, &res);
										diagram_point->c.y=heightline->height;
										diagram_point->next=diagram_points;
										diagram_points=diagram_point;
										dbg(2,"%d %d\n", diagram_point->c.x, diagram_point->c.y);
									}
								}
							}
						}
						heightline=heightline->next;
					}
					dist+=transform_distance(projection_mg, &last, &c);
				}
				last=c;
			}

		}
	}

	if(mr)
		map_rect_destroy(mr);

	gui_internal_menu_render(this);

	if(!diagram_points) 
		return;

	first=1;
	diagram_point=diagram_points;
	while (diagram_point) {
		if (first) {
			dbbox.lu=diagram_point->c;
			dbbox.rl=diagram_point->c;
			first=0;
		} else
			coord_rect_extend(&dbbox, &diagram_point->c);
		diagram_point=diagram_point->next;
	}
	dbg(2,"%d %d %d %d\n", dbbox.lu.x, dbbox.lu.y, dbbox.rl.x, dbbox.rl.y);
	if (dbbox.rl.x > dbbox.lu.x && dbbox.lu.x*100/(dbbox.rl.x-dbbox.lu.x) <= 25)
		dbbox.lu.x=0;
	if (dbbox.lu.y > dbbox.rl.y && dbbox.rl.y*100/(dbbox.lu.y-dbbox.rl.y) <= 25)
		dbbox.rl.y=0;
	dbg(2,"%d,%d %dx%d\n", box->p.x, box->p.y, box->w, box->h);
	x=dbbox.lu.x;
	first=1;
	for (;;) {
		struct point p[2];
		min=NULL;
		diagram_point=diagram_points;
		while (diagram_point) {
			if (diagram_point->c.x >= x && (!min || min->c.x > diagram_point->c.x))
				min=diagram_point;
			diagram_point=diagram_point->next;
		}
		if (! min)
			break;
		p[1].x=(min->c.x-dbbox.lu.x)*(box->w-10)/(dbbox.rl.x-dbbox.lu.x)+box->p.x+5;
		p[1].y=(min->c.y-dbbox.rl.y)*(box->h-10)/(dbbox.lu.y-dbbox.rl.y)+box->p.y+5;
		dbg(2,"%d,%d=%d,%d\n",min->c.x, min->c.y, p[1].x,p[1].y);
		graphics_draw_circle(this->gra, this->foreground, &p[1], 2);
		if (first)
			first=0;
		else
			graphics_draw_lines(this->gra, this->foreground, p, 2);
		p[0]=p[1];
		x=min->c.x+1;
	}


}

static void
gui_internal_cmd2_route_description(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{


	struct widget * menu;
	struct widget * box;


	if(! this->vehicle_cb)
	{
	  /**
	   * Register the callback on vehicle updates.
	   */
	  this->vehicle_cb = callback_new_attr_1(callback_cast(gui_internal_route_update),
						       attr_position_coord_geo,this);
	  navit_add_callback(this->nav,this->vehicle_cb);
	}

	this->route_data.route_table = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);

	menu=gui_internal_menu(this,_("Route Description"));

	menu->wfree=gui_internal_route_screen_free;
	this->route_data.route_showing=1;
	this->route_data.route_table->spx = this->spacing;


	box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);

	gui_internal_widget_append(box,this->route_data.route_table);
	box->w=menu->w;
	box->spx = this->spacing;
	this->route_data.route_table->w=box->w;
	gui_internal_widget_append(menu,box);
	gui_internal_populate_route_table(this,this->nav);
	gui_internal_menu_render(this);

}

static void
gui_internal_cmd2_pois(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *w;
	struct poi_param *param;
	struct attr pro;
	struct coord c;

	dbg(1,"enter\n");
	if (!in || !in[0])
		return;
	if (!ATTR_IS_COORD_GEO(in[0]->type))
		return;
	if (!navit_get_attr(this->nav, attr_projection, &pro, NULL))
		return;
	w=g_new0(struct widget,1);
	param=g_new0(struct poi_param,1);
	if (in[1] && ATTR_IS_STRING(in[1]->type)) {
		gui_internal_poi_param_set_filter(param, in[1]->u.str);
		if (in[2] && ATTR_IS_INT(in[2]->type))
			param->isAddressFilter=in[2]->u.num;
	}
	
	transform_from_geo(pro.u.projection,in[0]->u.coord_geo,&c);
	w->c.x=c.x;
	w->c.y=c.y;
	w->c.pro=pro.u.projection;
	gui_internal_cmd_pois(this, w, param);
	g_free(w);
	gui_internal_poi_param_free(param);
}

static void
gui_internal_cmd2_locale(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *menu,*wb,*w;
	char *text;

	graphics_draw_mode(this->gra, draw_mode_begin);
	menu=gui_internal_menu(this, _("Show Locale"));
	menu->spx=this->spacing*10;
	wb=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(menu, wb);
	text=g_strdup_printf("LANG=%1$s (1=%3$s 2=%2$s)",getenv("LANG"),"2","1");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_left_center|orientation_horizontal|flags_fill;
	g_free(text);
#ifdef HAVE_API_WIN32_BASE
	{
		char country[32],lang[32];
#ifdef HAVE_API_WIN32_CE
		wchar_t wcountry[32],wlang[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, wlang, sizeof(wlang));
		WideCharToMultiByte(CP_ACP,0,wlang,-1,lang,sizeof(lang),NULL,NULL);
#else
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
#endif
		text=g_strdup_printf("LOCALE_SABBREVLANGNAME=%s",lang);
		gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
		w->flags=gravity_left_center|orientation_horizontal|flags_fill;
		g_free(text);
#ifdef HAVE_API_WIN32_CE
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, wcountry, sizeof(wcountry));
		WideCharToMultiByte(CP_ACP,0,wcountry,-1,country,sizeof(country),NULL,NULL);
#else
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, country, sizeof(country));
#endif
		text=g_strdup_printf("LOCALE_SABBREVCTRYNAME=%s",country);
		gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
		w->flags=gravity_left_center|orientation_horizontal|flags_fill;
		g_free(text);
	}
#endif

	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_cmd_formerdests(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *wb,*w,*wbm,*tbl=NULL;
	struct map *formerdests;
	struct map_rect *mr_formerdests;
	struct item *item;
	struct attr attr;
	char *label_full;
	enum projection projection;

	if(!navit_get_attr(this->nav, attr_former_destination_map, &attr, NULL))
		return;

	formerdests=attr.u.map;
	if(!formerdests) 
		return;

	mr_formerdests=map_rect_new(formerdests, NULL);
	if(!mr_formerdests)
		return;

	projection = map_projection(formerdests);

	gui_internal_prune_menu_count(this, 1, 0);
	wb=gui_internal_menu(this, _("Former Destinations"));
	wb->background=this->background;

	w=gui_internal_box_new(this,
			gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*2;
	gui_internal_widget_append(wb, w);
	while ((item=map_rect_get_item(mr_formerdests))) {
		struct coord c;
		struct widget *row;
		if (item->type!=type_former_destination) continue;
		if (!item_attr_get(item, attr_label, &attr)) continue;
		if(!tbl) {
			tbl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand | orientation_vertical,1);
			gui_internal_widget_append(w,tbl);
		}
		row=gui_internal_widget_table_row_new(this,gravity_left| flags_fill| orientation_vertical);
		gui_internal_widget_prepend(tbl, row);
		label_full=attr.u.str;
		wbm=gui_internal_button_new_with_callback(this, label_full,
				image_new_xs(this, "gui_active"),
				gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_position, NULL);
		gui_internal_widget_append(row,wbm);
		if (item_coord_get(item, &c, 1)) {
			wbm->c.x=c.x;
			wbm->c.y=c.y;
			wbm->c.pro=projection;
			wbm->name=g_strdup(label_full);
			wbm->text=g_strdup(label_full);
			wbm->data=(void*)8; //Mark us as a former destination 
			wbm->prefix=g_strdup(label_full);
		}
	}
	if (!tbl){
        	wbm=gui_internal_text_new(this, _("- No former destinations available -"), 
		    gravity_left_center|orientation_horizontal|flags_fill);
		gui_internal_widget_append(w, wbm);
	}
	gui_internal_menu_render(this);
	map_rect_destroy(mr_formerdests);
}

static void
gui_internal_cmd2_bookmarks(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *str=NULL;
	if (in && in[0] && ATTR_IS_STRING(in[0]->type)) {
		str=in[0]->u.str;
	}

	gui_internal_cmd_bookmarks(this, NULL, str);
}

static void
gui_internal_cmd2_abort_navigation(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	navit_set_destination(this->nav, NULL, NULL, 0);
}

static void
gui_internal_cmd2_back(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_back(this, NULL, NULL);
	graphics_draw_mode(this->gra, draw_mode_end);
	gui_internal_check_exit(this);
}

static void
gui_internal_cmd2_back_to_map(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	gui_internal_prune_menu(this, NULL);
}


static void
gui_internal_get_data(struct gui_priv *priv, char *command, struct attr **in, struct attr ***out)
{
	struct attr private_data = { attr_private_data, {(void *)&priv->data}};
	if (out)
		*out=attr_generic_add_attr(*out, &private_data);
}

static void
gui_internal_cmd_log(struct gui_priv *this)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext;
	gui_internal_enter(this, 1);
	gui_internal_set_click_coord(this, NULL);
	gui_internal_enter_setup(this);
	wb=gui_internal_menu(this, "Log Message");
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, _("Message")));
	wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_call_linked_on_finish;
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_log_clicked;
	wnext->data=wk;
	wk->data=wnext;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
	gui_internal_leave(this);
}

static void
gui_internal_cmd_menu2(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *href=NULL;
	int i=0, ignore=0, replace=0;

	if (in && in[i] && ATTR_IS_INT(in[i]->type))
		ignore=in[i++]->u.num;

	if (in && in[i] && ATTR_IS_STRING(in[i]->type)) {
		href=in[i++]->u.str;
		if (in[i] && ATTR_IS_INT(in[i]->type))
			replace=in[i++]->u.num;
	}

	if (this->root.children) {
		if (!href)
			return;
		gui_internal_html_load_href(this, href, replace);
		return;
	}
	gui_internal_cmd_menu(this, ignore, href);
}

static void
gui_internal_cmd2_position(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *name=_("Position");
	int flags=-1;

	dbg(1,"enter\n");
	if (!in || !in[0])
		return;
	if (!ATTR_IS_COORD_GEO(in[0]->type))
		return;
	if (in[1] && ATTR_IS_STRING(in[1]->type)) {
		name=in[1]->u.str;
		if (in[2] && ATTR_IS_INT(in[2]->type))
			flags=in[2]->u.num;
	}
	dbg(1,"flags=0x%x\n",flags);
	gui_internal_cmd_position_do(this, NULL, in[0]->u.coord_geo, NULL, name, flags);
}

static void
gui_internal_cmd2_refresh(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *href=g_strdup(this->href);
	gui_internal_html_load_href(this, href, 1);
	g_free(href);
}

static void
gui_internal_cmd2_set(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *pattern,*command=NULL;
	if (!in || !in[0] || !ATTR_IS_STRING(in[0]->type)) {
		dbg(0,"first parameter missing or wrong type\n");
		return;
	}
	pattern=in[0]->u.str;
	dbg(1,"pattern %s\n",pattern);
	if (in[1]) {
		command=gui_internal_cmd_match_expand(pattern, in+1);
		dbg(1,"expand %s\n",command);
		gui_internal_set(pattern, command);
		command_evaluate(&this->self, command);
		g_free(command);
	}

}

void
gui_internal_cmd2_quit(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr navit;
	gui_internal_prune_menu(this, NULL);
	navit.type=attr_navit;
	navit.u.navit=this->nav;
	config_remove_attr(config, &navit);
	event_main_loop_quit();
}

static void
gui_internal_cmd_write(struct gui_priv * this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *str=NULL,*str2=NULL;
	dbg(1,"enter %s %p %p %p\n",function,in,out,valid);
	if (!in || !in[0])
		return;
	dbg(1,"%s\n",attr_to_name(in[0]->type));
	if (ATTR_IS_STRING(in[0]->type)) {
		str=in[0]->u.str;
	}
	if (ATTR_IS_COORD_GEO(in[0]->type)) {
		str=str2=coordinates_geo(in[0]->u.coord_geo, '\n');
	}
	if (str) {
		str=g_strdup_printf("<html>%s</html>\n",str);
		gui_internal_html_parse_text(this, str);
	}
	g_free(str);
	g_free(str2);
}

static void
gui_internal_cmd2(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	int entering=0;
	int ignore=1;
	if (in && in[0] && ATTR_IS_INT(in[0]->type)) {
		ignore=in[0]->u.num;
		in++;
	}

	if(!this->root.children) {
		entering=1;
		gui_internal_apply_config(this);
		gui_internal_enter(this, ignore);
		gui_internal_enter_setup(this);
	}

	if(!strcmp(function, "bookmarks"))
		gui_internal_cmd2_bookmarks(this, function, in, out, valid);
	else if(!strcmp(function, "formerdests"))
		gui_internal_cmd_formerdests(this, function, in, out, valid);
	else if(!strcmp(function, "locale"))
		gui_internal_cmd2_locale(this, function, in, out, valid);
	else if(!strcmp(function, "position"))
		gui_internal_cmd2_position(this, function, in, out, valid);
	else if(!strcmp(function, "pois"))
		gui_internal_cmd2_pois(this, function, in, out, valid);
	else if(!strcmp(function, "route_description"))
		gui_internal_cmd2_route_description(this, function, in, out, valid);
	else if(!strcmp(function, "route_height_profile"))
		gui_internal_cmd2_route_height_profile(this, function, in, out, valid);
	else if(!strcmp(function, "setting_layout"))
		gui_internal_cmd2_setting_layout(this, function, in, out, valid);
	else if(!strcmp(function, "setting_maps"))
		gui_internal_cmd2_setting_maps(this, function, in, out, valid);
	else if(!strcmp(function, "setting_rules"))
		gui_internal_cmd2_setting_rules(this, function, in, out, valid);
	else if(!strcmp(function, "setting_vehicle"))
		gui_internal_cmd2_setting_vehicle(this, function, in, out, valid);
	else if(!strcmp(function, "town"))
		gui_internal_cmd2_town(this, function, in, out, valid);
	else if(!strcmp(function, "enter_coord"))
		gui_internal_cmd_enter_coord(this, function, in, out, valid);
	else if(!strcmp(function, "waypoints"))
		gui_internal_cmd2_waypoints(this, function, in, out, valid);
	else if(!strcmp(function, "about"))
		gui_internal_cmd2_about(this, function, in, out, valid);

	if(entering)
		graphics_draw_mode(this->gra, draw_mode_end);
}

static struct command_table commands[] = {
	{"E",command_cast(gui_internal_cmd_escape)},
	{"abort_navigation",command_cast(gui_internal_cmd2_abort_navigation)},
	{"back",command_cast(gui_internal_cmd2_back)},
	{"back_to_map",command_cast(gui_internal_cmd2_back_to_map)},
	{"bookmarks",command_cast(gui_internal_cmd2)},
	{"formerdests",command_cast(gui_internal_cmd2)},
	{"get_data",command_cast(gui_internal_get_data)},
	{"locale",command_cast(gui_internal_cmd2)},
	{"log",command_cast(gui_internal_cmd_log)},
	{"menu",command_cast(gui_internal_cmd_menu2)},
	{"position",command_cast(gui_internal_cmd2_position)},
	{"pois",command_cast(gui_internal_cmd2)},
	{"refresh",command_cast(gui_internal_cmd2_refresh)},
	{"route_description",command_cast(gui_internal_cmd2)},
	{"route_height_profile",command_cast(gui_internal_cmd2)},
	{"set",command_cast(gui_internal_cmd2_set)},
	{"setting_layout",command_cast(gui_internal_cmd2)},
	{"setting_maps",command_cast(gui_internal_cmd2)},
	{"setting_rules",command_cast(gui_internal_cmd2)},
	{"setting_vehicle",command_cast(gui_internal_cmd2)},
	{"town",command_cast(gui_internal_cmd2)},
	{"enter_coord",command_cast(gui_internal_cmd2)},
	{"quit",command_cast(gui_internal_cmd2_quit)},
	{"waypoints",command_cast(gui_internal_cmd2)},
	{"write",command_cast(gui_internal_cmd_write)},
	{"about",command_cast(gui_internal_cmd2)},

};

void
gui_internal_command_init(struct gui_priv *this, struct attr **attrs)
{
	struct attr *attr;
        if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
		command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), this);
        }
}


