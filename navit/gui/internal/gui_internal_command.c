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
#include "xmlconfig.h"
#include "event.h"
#include "config_.h"
#include "map.h"
#include "mapset.h"
#include "transform.h"
#include "search.h"
#include "route.h"
#include "vehicle.h"
#include "vehicleprofile.h"
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
#if HAS_IFADDRS
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

extern char *version;

/**
 * @brief Converts a WGS84 coordinate pair to its string representation.
 *
 * This function takes a coordinate pair with latitude and longitude in degrees and converts them to a
 * string of the form {@code 45°28'0" N 9°11'26" E}.
 *
 * @param gc A WGS84 coordinate pair
 * @param sep The separator character to insert between latitude and longitude
 *
 * @return The coordinates as a formatted string
 */
static char *coordinates_geo(const struct coord_geo *gc, char sep) {
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

/**
 * @brief Converts a coordinate pair to its WGS84 string representation.
 *
 * This function takes a coordinate pair, transforms it to WGS84 and converts it to a string of the form
 * {@code 45°28'0" N 9°11'26" E}.
 *
 * @param gc A coordinate pair
 * @param sep The separator character to insert between latitude and longitude
 *
 * @return The coordinates as a formatted string
 */
char *gui_internal_coordinates(struct pcoord *pc, char sep) {
    struct coord_geo g;
    struct coord c;
    c.x=pc->x;
    c.y=pc->y;
    transform_to_geo(pc->pro, &c, &g);
    return coordinates_geo(&g, sep);

}

enum escape_mode {
    escape_mode_none=0,
    escape_mode_string=1,
    escape_mode_quote=2,
    escape_mode_html=4,
    escape_mode_html_quote=8,
    escape_mode_html_apos=16,
};

/* todo &=&amp;, < = &lt; */

static char *gui_internal_escape(enum escape_mode mode, char *in) {
    int len=mode & escape_mode_string ? 3:1;
    char *dst,*out,*src=in;
    char *quot="&quot;";
    char *apos="&apos;";
    while (*src) {
        if ((*src == '"' || *src == '\\') && (mode & (escape_mode_string | escape_mode_quote)))
            len++;
        if (*src == '"' && mode == escape_mode_html_quote)
            len+=strlen(quot);
        else if (*src == '\'' && mode == escape_mode_html_apos)
            len+=strlen(apos);
        else
            len++;
        src++;
    }
    src=in;
    out=dst=g_malloc(len);
    if (mode & escape_mode_string)
        *dst++='"';
    while (*src) {
        if ((*src == '"' || *src == '\\') && (mode & (escape_mode_string | escape_mode_quote)))
            *dst++='\\';
        if (*src == '"' && mode == escape_mode_html_quote) {
            strcpy(dst,quot);
            src++;
            dst+=strlen(quot);
        } else if (*src == '\'' && mode == escape_mode_html_apos) {
            strcpy(dst,apos);
            src++;
            dst+=strlen(apos);
        } else
            *dst++=*src++;
    }
    if (mode & escape_mode_string)
        *dst++='"';
    *dst++='\0';
    return out;
}

static void gui_internal_cmd_escape(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                    int *valid) {
    struct attr escaped;
    if (!in || !in[0]) {
        dbg(lvl_error,"first parameter missing or wrong type");
        return;
    }
    if (!out) {
        dbg(lvl_error,"output missing");
        return;
    }
    if (ATTR_IS_STRING(in[0]->type)) {
        escaped.type=in[0]->type;
        escaped.u.str=gui_internal_escape(escape_mode_string,in[0]->u.str);
    } else if (ATTR_IS_INT(in[0]->type)) {
        escaped.type=attr_type_string_begin;
        escaped.u.str=g_strdup_printf("%ld",in[0]->u.num);
    } else {
        dbg(lvl_error,"first parameter wrong type");
        return;
    }
    dbg(lvl_debug,"in %s result %s",in[0]->u.str,escaped.u.str);
    *out=attr_generic_add_attr(*out, attr_dup(&escaped));
    g_free(escaped.u.str);
}

static void gui_internal_cmd2_about(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                    int *valid) {
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

static void gui_internal_cmd2_waypoints(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                        int *valid) {
    gui_internal_select_waypoint(this, _("Waypoints"), NULL, NULL, gui_internal_cmd_position, (void*)2);
}

static void gui_internal_cmd_enter_coord(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
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
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, _("Latitude Longitude")));
    wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_call_linked_on_finish;
    gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wnext->state |= STATE_SENSITIVE;
    wnext->func = gui_internal_cmd_enter_coord_clicked;
    wnext->data=wk;
    wk->data=wnext;
    row=gui_internal_text_new(this, _("Enter coordinates, for example:"),
                              gravity_top_center|flags_fill|orientation_vertical);
    gui_internal_widget_append(wr,row);
    row=gui_internal_text_new(this, "52.5219N 19.4127E", gravity_top_center|flags_fill|orientation_vertical);
    gui_internal_widget_append(wr,row);
    row=gui_internal_text_new(this, "52°31.3167N 19°24.7667E", gravity_top_center|flags_fill|orientation_vertical);
    gui_internal_widget_append(wr,row);
    row=gui_internal_text_new(this, "52°31'19N 19°24'46E", gravity_top_center|flags_fill|orientation_vertical);
    gui_internal_widget_append(wr,row);

    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this, VKBD_DEGREE));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_DEGREE, NULL);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd2_town(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    if (this->sl)
        search_list_select(this->sl, attr_country_all, 0, 0);
    gui_internal_search(this,_("Town"),"Town",1);
}

static void gui_internal_cmd2_setting_vehicle(struct gui_priv *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
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
                image_new_xs(this, attr.u.vehicle == active_vehicle.u.vehicle ? "gui_active" : "gui_inactive"),
                gravity_left_center|orientation_horizontal|flags_fill,
                gui_internal_cmd_vehicle_settings, attr.u.vehicle);
        wl->text=g_strdup(vattr.u.str);
        gui_internal_widget_append(w, wl);
    }
    navit_attr_iter_destroy(iter);
    gui_internal_menu_render(this);
}

static void gui_internal_cmd2_setting_rules(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
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
                               gui_internal_button_navit_attr_new(this, _("Map follows Vehicle"),
                                       gravity_left_center|orientation_horizontal|flags_fill,
                                       &on, &off));
    on.u.num=1;
    off.u.num=0;
    on.type=off.type=attr_waypoints_flag;
    gui_internal_widget_append(w,
                               gui_internal_button_navit_attr_new(this, _("Plan with Waypoints"),
                                       gravity_left_center|orientation_horizontal|flags_fill,
                                       &on, &off));
    gui_internal_menu_render(this);
}

static void gui_internal_cmd2_setting_maps(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
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
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        gui_internal_widget_append(row, wma);
        g_free(label);
    }
    navit_attr_iter_destroy(iter);
    gui_internal_menu_render(this);

}

static void gui_internal_cmd2_setting_layout(struct gui_priv *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
    struct attr attr;
    struct widget *w,*wb,*wl,*row;
    struct attr_iter *iter;


    wb=gui_internal_menu(this, _("Layout"));
    w=gui_internal_widget_table_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill,1);
    gui_internal_widget_append(wb, w);
    iter=navit_attr_iter_new();
    while(navit_get_attr(this->nav, attr_layout, &attr, iter)) {
        gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,
                                          gravity_left|orientation_horizontal|flags_fill));
        wl=gui_internal_button_navit_attr_new(this, attr.u.layout->name, gravity_left_center|orientation_horizontal|flags_fill,
                                              &attr, NULL);
        gui_internal_widget_append(row, wl);
    }
    navit_attr_iter_destroy(iter);
    gui_internal_menu_render(this);
}

/*
 * @brief Displays Route Height Profile
 *
 * displays a heightprofile if a route is active and
 * some heightinfo is provided by means of a map
 *
 * the name of the file providing the heightlines must
 * comply with *.heightlines.bin
 *
 */
static void gui_internal_cmd2_route_height_profile(struct gui_priv *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
    struct widget * menu, *box;
    struct map * map=NULL;
    struct map_rect * mr=NULL;
    struct route * route;
    struct item * item =NULL;
    struct mapset *ms;
    struct mapset_handle *msh;
    int x,i,first=1,dist=0;
    int diagram_points_count = 0;
    struct coord c,last,res;
    struct coord_rect rbbox,dbbox;
    struct map_selection sel;
    struct heightline *heightline,*heightlines=NULL;
    struct diagram_point *min,*diagram_point,*diagram_points=NULL;
    struct point p[2];
    int min_ele=INT_MAX;
    int max_ele=INT_MIN;
    int distance=0;
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
        mr = NULL;
        ms=navit_get_mapset(this->nav);
        if (!first && ms) {
            int heightmap_installed = FALSE;
            msh=mapset_open(ms);
            while ((map=mapset_next(msh, 1))) {
                struct attr data_attr;
                if (map_get_attr(map, attr_data, &data_attr, NULL)) {
                    dbg(lvl_debug,"map name = %s",data_attr.u.str);
                    if (strstr(data_attr.u.str,".heightlines.bin")) {
                        dbg(lvl_info,"reading heightlines from map %s",data_attr.u.str);
                        mr=map_rect_new(map, &sel);
                        heightmap_installed = TRUE;
                    } else {
                        dbg(lvl_debug,"ignoring map %s",data_attr.u.str);
                    }
                }
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
                    mr = NULL;
                }
            }
            mapset_close(msh);
            if (!heightmap_installed) {
                char *text;
                struct widget *w;
                text=g_strdup_printf("%s",_("please install a map *.heightlines.bin to provide elevationdata"));
                gui_internal_widget_append(box, w=gui_internal_label_new(this, text));
                w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
                g_free(text);
                gui_internal_menu_render(this);
                return;
            }
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
                                        diagram_points_count ++;
                                        dbg(lvl_debug,"%d %d", diagram_point->c.x, diagram_point->c.y);
                                        max_ele=MAX(max_ele, diagram_point->c.y);
                                        min_ele=MIN(min_ele, diagram_point->c.y);
                                        distance=diagram_point->c.x;
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
    while (heightlines) {
        heightline=heightlines;
        heightlines=heightlines->next;
        g_free(heightline);
    }
    if(mr)
        map_rect_destroy(mr);

    if(diagram_points_count < 2) {
        char *text;
        struct widget *w;
        text=g_strdup_printf("%s",_("The route must cross at least 2 heightlines"));
        gui_internal_widget_append(box, w=gui_internal_label_new(this, text));
        w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
        g_free(text);
        gui_internal_menu_render(this);
        if(diagram_points)
            g_free(diagram_points);
        return;
    }

    gui_internal_menu_render(this);
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
    dbg(lvl_debug,"%d %d %d %d", dbbox.lu.x, dbbox.lu.y, dbbox.rl.x, dbbox.rl.y);
    if (dbbox.rl.x > dbbox.lu.x && dbbox.lu.x*100/(dbbox.rl.x-dbbox.lu.x) <= 25)
        dbbox.lu.x=0;
    if (dbbox.lu.y > dbbox.rl.y && dbbox.rl.y*100/(dbbox.lu.y-dbbox.rl.y) <= 25)
        dbbox.rl.y=0;
    dbg(lvl_debug,"%d,%d %dx%d", box->p.x, box->p.y, box->w, box->h);
    x=dbbox.lu.x;
    first=1;
    if (diagram_points_count > 1 && dbbox.rl.x != dbbox.lu.x && dbbox.lu.y != dbbox.rl.y) {
        for (;;) {
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
            p[1].y=(box->h)-5-(min->c.y-dbbox.rl.y)*(box->h-10)/(dbbox.lu.y-dbbox.rl.y)+box->p.y;
            dbg(lvl_debug,"%d,%d=%d,%d",min->c.x, min->c.y, p[1].x,p[1].y);
            graphics_draw_circle(this->gra, this->foreground, &p[1], 2);
            if (first)
                first=0;
            else
                graphics_draw_lines(this->gra, this->foreground, p, 2);
            p[0]=p[1];
            x=min->c.x+1;
        }
    }

    struct point pTopLeft= {0, box->p.y + 10};
    struct point pBottomLeft= {0, box->h + box->p.y - 2};
    struct point pBottomRight= {box->w - 100, box->h + box->p.y - 2};
    char* minele_text=g_strdup_printf("%d m", min_ele);
    char* maxele_text=g_strdup_printf("%d m", max_ele);
    char* distance_text=g_strdup_printf("%.3f km", distance/1000.0);
    graphics_draw_text_std(this->gra, 10, maxele_text, &pTopLeft);
    graphics_draw_text_std(this->gra, 10, minele_text, &pBottomLeft);
    graphics_draw_text_std(this->gra, 10, distance_text, &pBottomRight);

    while (diagram_points) {
        diagram_point=diagram_points;
        diagram_points=diagram_points->next;
        g_free(diagram_point);
    }
}

static void gui_internal_cmd2_route_description(struct gui_priv *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {


    struct widget * menu;
    struct widget * box;


    if(! this->vehicle_cb) {
        /**
         * Register the callback on vehicle updates.
         */
        this->vehicle_cb = callback_new_attr_1(callback_cast(gui_internal_route_update),
                                               attr_position_coord_geo,this);
        navit_add_callback(this->nav,this->vehicle_cb);
    }

    this->route_data.route_table = gui_internal_widget_table_new(this,
                                   gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);

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

static void gui_internal_cmd2_pois(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    struct widget *w;
    struct poi_param *param;
    struct attr pro;
    struct coord c;

    dbg(lvl_debug,"enter");
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
            param->AddressFilterType=in[2]->u.num;
    }

    transform_from_geo(pro.u.projection,in[0]->u.coord_geo,&c);
    w->c.x=c.x;
    w->c.y=c.y;
    w->c.pro=pro.u.projection;
    gui_internal_cmd_pois(this, w, param);
    g_free(w);
    gui_internal_poi_param_free(param);
}

static void gui_internal_cmd2_locale(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                     int *valid) {
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

/**
 * @brief display basic networking information
 *
 * @return nothing
 *
 * This function displays basic networking information, currently
 * only the interface name and the associated IP address(es).
 * Currently only works on non Windows systems.
 *
 */
static void gui_internal_cmd2_network_info(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
#if HAS_IFADDRS
    struct widget *menu,*wb,*w;
    char *text;

    graphics_draw_mode(this->gra, draw_mode_begin);
    menu=gui_internal_menu(this, _("Network info"));
    menu->spx=this->spacing*10;
    wb=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(menu, wb);

    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            if(g_ascii_strncasecmp(tmp->ifa_name,"lo",2 ) ) {
                text=g_strdup_printf("%s: %s", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
                gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
                w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
                g_free(text);
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);

    gui_internal_menu_render(this);
    graphics_draw_mode(this->gra, draw_mode_end);
#else
    dbg(lvl_error, "Cannot show network info: ifaddr.h not found");
#endif
}

static void gui_internal_cmd_formerdests(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
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
    if (!tbl) {
        wbm=gui_internal_text_new(this, _("- No former destinations available -"),
                                  gravity_left_center|orientation_horizontal|flags_fill);
        gui_internal_widget_append(w, wbm);
    }
    gui_internal_menu_render(this);
    map_rect_destroy(mr_formerdests);
}

static void gui_internal_cmd2_bookmarks(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                        int *valid) {
    char *str=NULL;
    if (in && in[0] && ATTR_IS_STRING(in[0]->type)) {
        str=in[0]->u.str;
    }

    gui_internal_cmd_bookmarks(this, NULL, str);
}

static void gui_internal_cmd2_abort_navigation(struct gui_priv *this, char *function, struct attr **in,
        struct attr ***out,
        int *valid) {
    navit_set_destination(this->nav, NULL, NULL, 0);
}

static void gui_internal_cmd2_back(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    graphics_draw_mode(this->gra, draw_mode_begin);
    gui_internal_back(this, NULL, NULL);
    graphics_draw_mode(this->gra, draw_mode_end);
    gui_internal_check_exit(this);
}

static void gui_internal_cmd2_back_to_map(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
        int *valid) {
    gui_internal_prune_menu(this, NULL);
    gui_internal_check_exit(this);
}


static void gui_internal_get_data(struct gui_priv *priv, char *command, struct attr **in, struct attr ***out) {
    struct attr private_data = { attr_private_data, {(void *)&priv->data}};
    if (out)
        *out=attr_generic_add_attr(*out, &private_data);
}

static void gui_internal_cmd_log(struct gui_priv *this) {
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
        gui_internal_widget_append(w, gui_internal_keyboard(this,
                                   VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"))));
    else
        gui_internal_keyboard_show_native(this, w, VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG")),
                                          getenv("LANG"));
    gui_internal_menu_render(this);
    gui_internal_leave(this);
}

static void gui_internal_cmd_menu2(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
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

static void gui_internal_cmd2_position(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                       int *valid) {
    const char *name=_("Position");
    int flags=-1;

    dbg(lvl_debug,"enter");
    if (!in || !in[0])
        return;
    if (!ATTR_IS_COORD_GEO(in[0]->type))
        return;
    if (in[1] && ATTR_IS_STRING(in[1]->type)) {
        name=in[1]->u.str;
        if (in[2] && ATTR_IS_INT(in[2]->type))
            flags=in[2]->u.num;
    }
    dbg(lvl_debug,"flags=0x%x",flags);
    gui_internal_cmd_position_do(this, NULL, in[0]->u.coord_geo, NULL, name, flags);
}

static void gui_internal_cmd_redraw_map(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                        int *valid) {
    this->redraw=1;
}

static void gui_internal_cmd2_refresh(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                      int *valid) {
    char *href=g_strdup(this->href);
    gui_internal_html_load_href(this, href, 1);
    g_free(href);
}

static void gui_internal_cmd2_set(struct gui_priv *this, char *function, struct attr **in, struct attr ***out,
                                  int *valid) {
    char *pattern,*command=NULL;
    if (!in || !in[0] || !ATTR_IS_STRING(in[0]->type)) {
        dbg(lvl_error,"first parameter missing or wrong type");
        return;
    }
    pattern=in[0]->u.str;
    dbg(lvl_debug,"pattern %s",pattern);
    if (in[1]) {
        command=gui_internal_cmd_match_expand(pattern, in+1);
        dbg(lvl_debug,"expand %s",command);
        gui_internal_set(pattern, command);
        command_evaluate(&this->self, command);
        g_free(command);
    } else {
        gui_internal_set(pattern, NULL);
    }

}

void gui_internal_cmd2_quit(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid) {
    struct attr navit;
    gui_internal_prune_menu(this, NULL);
    navit.type=attr_navit;
    navit.u.navit=this->nav;
    config_remove_attr(config, &navit);
    event_main_loop_quit();
}

static char *gui_internal_append_attr(char *str, enum escape_mode mode, char *pre, struct attr *attr, char *post) {
    char *astr=NULL;
    if (ATTR_IS_STRING(attr->type))
        astr=gui_internal_escape(mode, attr->u.str);
    else if (ATTR_IS_COORD_GEO(attr->type)) {
        char *str2=coordinates_geo(attr->u.coord_geo, '\n');
        astr=gui_internal_escape(mode, str2);
        g_free(str2);
    } else if (ATTR_IS_INT(attr->type))
        astr=g_strdup_printf("%ld",attr->u.num);
    else
        astr=g_strdup_printf("Unsupported type %s",attr_to_name(attr->type));
    str=g_strconcat_printf(str,"%s%s%s",pre,astr,post);
    g_free(astr);
    return str;
}

static void gui_internal_cmd_write(struct gui_priv * this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    char *str=NULL;
    dbg(lvl_debug,"enter %s %p %p %p",function,in,out,valid);
    if (!in)
        return;
    while (*in) {
        str=gui_internal_append_attr(str, escape_mode_none, "", *in, "");
        in++;
    }
    if (str) {
        str=g_strdup_printf("<html>%s</html>\n",str);
#if 0
        dbg(lvl_debug,"%s",str);
#endif
        gui_internal_html_parse_text(this, str);
    }
    g_free(str);
}

static void gui_internal_onclick(struct attr ***in, char **onclick, char *set) {
    struct attr **i=*in;
    char *c,*str=NULL,*args=NULL,*sep="";

    if (!*i || !ATTR_IS_STRING((*i)->type) || !(*i)->u.str)
        goto error;
    str=g_strdup((*i)->u.str);
    i++;
    c=str;
    while (*c) {
        if (c[0] == '%' && c[1] == '{') {
            char format[4],*end=strchr(c+2,'}'),*replacement=NULL,*new_str;
            int is_arg;
            if (!end) {
                dbg(lvl_error,"Missing closing brace in format string %s",c);
                goto error;
            }
            if (end-c > sizeof(format)) {
                dbg(lvl_error,"Invalid format string %s",c);
                goto error;
            }
            strncpy(format, c+2, end-c-2);
            format[end-c-2]='\0';
            is_arg=end[1] == '*';
            c[0]='\0';
            if (!strcmp(format,"d")) {
                replacement=gui_internal_append_attr(NULL, escape_mode_string, "", *i++, "");
                if (is_arg) {
                    args=g_strconcat_printf(args, "%s%s", args ? "," : "", replacement);
                    g_free(replacement);
                    replacement=g_strdup("");
                }

            }
            if (!strcmp(format,"se")) {
                replacement=gui_internal_append_attr(NULL, escape_mode_string, "", *i++, "");
                if (is_arg) {
                    char *arg=gui_internal_escape(escape_mode_string, replacement);
                    args=g_strconcat_printf(args, "%s%s", args ? "," : "", arg);
                    g_free(replacement);
                    g_free(arg);
                    replacement=g_strdup("");
                }
            }
            if (!replacement) {
                dbg(lvl_error,"Unsupported format string %s",format);
                goto error;
            }
            new_str=g_strconcat(str, replacement, end+1, NULL);
            c=new_str+strlen(str)+strlen(replacement);
            g_free(str);
            g_free(replacement);
            str=new_str;
        }
        c++;
    }
    *in=i;
    if (*onclick && strlen(*onclick))
        sep=";";
    if (str && strlen(str)) {
        char *old=*onclick;
        if (set) {
            char *setstr=gui_internal_escape(escape_mode_string,str);
            char *argssep="";
            if (args && strlen(args))
                argssep=",";
            *onclick=g_strconcat(old,sep,set,"(",setstr,argssep,args,")",NULL);
        } else {
            *onclick=g_strconcat(old,sep,str,NULL);
        }
        g_free(old);
    }
error:
    g_free(str);
    g_free(args);
    return;
}

static void gui_internal_cmd_img(struct gui_priv * this, char *function, struct attr **in, struct attr ***out,
                                 int *valid) {
    char *str=g_strdup("<img"),*suffix=NULL,*onclick=g_strdup(""),*html;

    if (ATTR_IS_STRING((*in)->type)) {
        if ((*in)->u.str && strlen((*in)->u.str))
            str=gui_internal_append_attr(str, escape_mode_string|escape_mode_html, " class=", *in, "");
        in++;
    } else {
        dbg(lvl_error,"argument error: class argument not string");
        goto error;
    }
    if (ATTR_IS_STRING((*in)->type) && (*in)->u.str) {
        if ((*in)->u.str && strlen((*in)->u.str)) {
            str=gui_internal_append_attr(str, escape_mode_string|escape_mode_html, " src=", *in, "");
        }
        in++;
    } else {
        dbg(lvl_error,"argument error: image argument not string");
        goto error;
    }
    if (ATTR_IS_STRING((*in)->type) && (*in)->u.str) {
        if ((*in)->u.str && strlen((*in)->u.str)) {
            suffix=gui_internal_append_attr(NULL, escape_mode_html, ">", *in, "</img>");
        } else {
            suffix=g_strdup("/>");
        }
        in++;
    } else {
        dbg(lvl_error,"argument error: text argument not string");
        goto error;
    }
    gui_internal_onclick(&in,&onclick,NULL);
    gui_internal_onclick(&in,&onclick,"set");
    gui_internal_onclick(&in,&onclick,NULL);
    if (strlen(onclick)) {
        char *tmp=gui_internal_escape(escape_mode_html_apos, onclick);
        str=g_strconcat_printf(str," onclick='%s'",tmp);
        g_free(tmp);
    }
    g_free(onclick);
    html=g_strdup_printf("<html>%s%s</html>\n",str,suffix);
    dbg(lvl_debug,"return %s",html);
    gui_internal_html_parse_text(this, html);
    g_free(html);
error:
    g_free(suffix);
    g_free(str);
    return;
}

static void gui_internal_cmd_debug(struct gui_priv * this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    char *str;
    dbg(lvl_debug,"begin");
    if (in) {
        while (*in) {
            str=attr_to_text(*in, NULL, 0);
            dbg(lvl_debug,"%s:%s",attr_to_name((*in)->type),str);
            in++;
            g_free(str);
        }
    }
    dbg(lvl_debug,"done");
}

static void gui_internal_cmd2(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid) {
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
    else if(!strcmp(function, "network_info"))
        gui_internal_cmd2_network_info(this, function, in, out, valid);
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
    {"debug",command_cast(gui_internal_cmd_debug)},
    {"formerdests",command_cast(gui_internal_cmd2)},
    {"get_data",command_cast(gui_internal_get_data)},
    {"img",command_cast(gui_internal_cmd_img)},
    {"locale",command_cast(gui_internal_cmd2)},
    {"log",command_cast(gui_internal_cmd_log)},
    {"menu",command_cast(gui_internal_cmd_menu2)},
    {"position",command_cast(gui_internal_cmd2_position)},
    {"pois",command_cast(gui_internal_cmd2)},
    {"redraw_map",command_cast(gui_internal_cmd_redraw_map)},
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
#if HAS_IFADDRS
    {"network_info",command_cast(gui_internal_cmd2)},
#endif
};

void gui_internal_command_init(struct gui_priv *this, struct attr **attrs) {
    struct attr *attr;
    if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
        command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), this);
    }
}


