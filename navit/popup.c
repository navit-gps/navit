/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008, 2010 Navit Team
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

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include "popup.h"
#include "debug.h"
#include "navit.h"
#include "coord.h"
#include "gui.h"
#include "menu.h"
#include "point.h"
#include "transform.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "graphics.h"
#include "callback.h"
#include "route.h"
#include "navit_nls.h"
#include "bookmarks.h"

#if 0
static void popup_set_no_passing(struct popup_item *item, void *param) {
#if 0
    struct display_list *l=param;
    struct segment *seg=(struct segment *)(l->data);
    struct street_str *str=(struct street_str *)(seg->data[0]);
    char log[256];
    int segid=str->segid;
    if (segid < 0)
        segid=-segid;

    sprintf(log,"Attributes Street 0x%x updated: limit=0x%x(0x%x)", segid, 0x33, str->limit);
    str->limit=0x33;
    log_write(log, seg->blk_inf.file, str, sizeof(*str));
#endif
}

#endif

static void popup_traffic_distortion(struct item *item, char *attr) {
    /* add the configuration directory to the name of the file to use */
    char *dist_filename = g_strjoin(NULL, navit_get_user_data_directory(TRUE),
                                    "/distortion.txt", NULL);
    if (dist_filename) {				/* if we built the filename */
        FILE *map=fopen(dist_filename,"a");
        if (map) {					/* if the file was opened */
            struct coord c;
            struct map_rect *mr;
            fprintf(map,"type=traffic_distortion %s\n",attr);
            mr=map_rect_new(item->map,NULL);
            item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
            while (item_coord_get(item, &c, 1)) {
                fprintf(map,"0x%x 0x%x\n",c.x,c.y);
            }
            fclose(map);
        } else {
            dbg(lvl_error,"could not open file for distortions !!");

        } /* else - if (map) */
        g_free(dist_filename);			/* free the file name */
    } /* if (dist_filename) */

} /* end: popup_traffic_distortion(..) */


static void popup_traffic_distortion_blocked(struct item *item) {
    dbg(lvl_debug,"item=%p",item);
    popup_traffic_distortion(item, "maxspeed=0");
}

static void popup_traffic_distortion_speed(struct item *item, int maxspeed) {
    char buffer[256];
    sprintf(buffer,"maxspeed=%d",maxspeed);
    popup_traffic_distortion(item,buffer);
}

static void popup_traffic_distortion_delay(struct item *item, int delay) {
    char buffer[256];
    sprintf(buffer,"delay=%d",delay*600);
    popup_traffic_distortion(item,buffer);
}

static void popup_set_destination(struct navit *nav, struct pcoord *pc) {
    struct coord c;
    struct coord_geo g;
    char buffer[1024];
    char buffer_geo[1024];
    c.x = pc->x;
    c.y = pc->y;
    transform_to_geo(transform_get_projection(navit_get_trans(nav)), &c, &g);
    coord_format(g.lat,g.lng,DEGREES_MINUTES_SECONDS,buffer_geo,sizeof(buffer_geo));
    sprintf(buffer,"Map Point %s", buffer_geo);
    navit_set_destination(nav, pc, buffer, 1);
}


void popup_set_visitbefore(struct navit *nav, struct pcoord *pc,int visitbefore) {
    struct pcoord *dst;
    char buffer[1024];
    int i, dstcount_new;
    sprintf(buffer, _("Waypoint %d"), visitbefore+1);
    dstcount_new=navit_get_destination_count(nav)+1;
    dst=g_alloca(dstcount_new*sizeof(struct pcoord));
    navit_get_destinations(nav,dst,dstcount_new);
    for (i=dstcount_new-1; i>visitbefore; i--) {
        dst[i]=dst[i-1];
    }
    dst[visitbefore]=*pc;
    navit_add_destination_description(nav,pc,buffer);
    navit_set_destinations(nav, dst, dstcount_new, buffer, 1);
}



static void popup_set_bookmark(struct navit *nav, struct pcoord *pc) {
    struct attr attr;
    struct coord c;
    struct coord_geo g;
    char buffer[1024];
    char buffer_geo[1024];
    c.x = pc->x;
    c.y = pc->y;
    transform_to_geo(pc->pro, &c, &g);
    coord_format(g.lat,g.lng,DEGREES_MINUTES_SECONDS,buffer_geo,sizeof(buffer_geo));
    sprintf(buffer,"Map Point %s", buffer_geo);
    if (!gui_add_bookmark(navit_get_gui(nav), pc, buffer)) {
        navit_get_attr(nav, attr_bookmarks, &attr, NULL);
        bookmarks_add_bookmark(attr.u.bookmarks, pc, buffer);
    }
}


extern void *vehicle;

static void popup_set_position(struct navit *nav, struct pcoord *pc) {
    dbg(lvl_debug,"%p %p", nav, pc);
    navit_set_position(nav, pc);
}

#if 0
static void popup_break_crossing(struct display_list *l) {
    struct segment *seg=(struct segment *)(l->data);
    struct street_str *str=(struct street_str *)(seg->data[0]);
    char log[256];
    int segid=str->segid;
    if (segid < 0)
        segid=-segid;

    sprintf(log,"Coordinates Street 0x%x updated: limit=0x%x(0x%x)", segid, 0x33, str->limit);
    str->limit=0x33;
    log_write(log, seg->blk_inf.file, str, sizeof(*str));
}
#endif


#define popup_printf(menu, type, ...) popup_printf_cb(menu, type, NULL, __VA_ARGS__)

static void *popup_printf_cb(void *menu, enum menu_type type, struct callback *cb, const char *fmt, ...) {
    gchar *str,*us;
    int usc=0;
    va_list ap;
    void *ret;

    va_start(ap, fmt);
    str=g_strdup_vprintf(fmt, ap);
    dbg(lvl_debug,"%s", str);
    us=str;
    while (*us) {
        if (*us == '_')
            usc++;
        us++;
    }
    if (usc) {
        gchar *str2=g_malloc(strlen(str)+usc+1);
        gchar *us2=str2;
        us=str;
        while (*us) {
            if (*us == '_')
                *us2++=*us;
            *us2++=*us++;
        }
        *us2='\0';
        g_free(str);
        str=str2;
    }
    ret=menu_add(menu, str, type, cb);
    va_end(ap);
    g_free(str);
    return ret;
}



static void popup_show_visitbefore(struct navit *nav,struct pcoord *pc, void *menu) {
    void *menuvisitbefore;
    char buffer[100];
    int i, dstcount;
    dstcount=navit_get_destination_count(nav);
    if (dstcount>=1) {
        menuvisitbefore=popup_printf(menu, menu_type_submenu, _("Visit before..."));
        for (i=0; i<dstcount; i++) {
            sprintf(buffer,_("Waypoint %d"),i+1);
            popup_printf_cb(menuvisitbefore, menu_type_menu, callback_new_3(callback_cast(popup_set_visitbefore), nav, pc,i),
                            buffer);
        }
    }
}

static void popup_show_attr_val(struct map *map, void *menu, struct attr *attr) {
    char *attr_name=attr_to_name(attr->type);
    char *str;

    str=attr_to_text(attr, map, 1);
    popup_printf(menu, menu_type_menu, "%s: %s", attr_name, str);
    g_free(str);
}

#if 0
static void popup_show_attr(void *menu, struct item *item, enum attr_type attr_type) {
    struct attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type=attr_type;
    if (item_attr_get(item, attr_type, &attr))
        popup_show_attr_val(menu, &attr);
}
#endif

static void popup_show_attrs(struct map *map, void *menu, struct item *item) {
#if 0
    popup_show_attr(menu, item, attr_debug);
    popup_show_attr(menu, item, attr_address);
    popup_show_attr(menu, item, attr_phone);
    popup_show_attr(menu, item, attr_phone);
    popup_show_attr(menu, item, attr_entry_fee);
    popup_show_attr(menu, item, attr_open_hours);
#else
    struct attr attr;
    for (;;) {
        memset(&attr, 0, sizeof(attr));
        if (item_attr_get(item, attr_any, &attr))
            popup_show_attr_val(map, menu, &attr);
        else
            break;
    }

#endif
}

static void popup_item_dump(struct item *item) {
    struct map_rect *mr;
    mr=map_rect_new(item->map,NULL);
    item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
    dbg(lvl_debug,"item=%p",item);
    item_dump_filedesc(item,item->map,stdout);
    map_rect_destroy(mr);
}


static void popup_show_item(struct navit *nav, void *popup, struct displayitem *di) {
    struct map_rect *mr;
    void *menu, *menu_item, *menu_dist;
    char *label;
    struct item *item,*diitem;
    int count;

    label=graphics_displayitem_get_label(di);
    diitem=graphics_displayitem_get_item(di);
    count=graphics_displayitem_get_coord_count(di);

    dbg_assert(diitem);

    if (label)
        menu=popup_printf(popup, menu_type_submenu, "%s '%s' (%d coords)", item_to_name(diitem->type), label, count);
    else
        menu=popup_printf(popup, menu_type_submenu, "%s (%d coords)", item_to_name(diitem->type), count);
    menu_item=popup_printf(menu, menu_type_submenu, "Item");
    popup_printf(menu_item, menu_type_menu, "type: 0x%x", diitem->type);
    popup_printf(menu_item, menu_type_menu, "id: 0x%x 0x%x", diitem->id_hi, diitem->id_lo);
    if (diitem->map) {
        struct attr type,data;
        if (!map_get_attr(diitem->map, attr_type, &type, NULL))
            type.u.str="";
        if (!map_get_attr(diitem->map, attr_data, &data, NULL))
            data.u.str="";
        popup_printf(menu_item, menu_type_menu, "map: %s:%s", type.u.str, data.u.str);
    }
    if (diitem->map) {
        mr=map_rect_new(diitem->map,NULL);
        item=map_rect_get_item_byid(mr, diitem->id_hi, diitem->id_lo);
        dbg(lvl_debug,"item=%p", item);
        if (item) {
            popup_show_attrs(item->map, menu_item, item);
            popup_printf_cb(menu_item, menu_type_menu, callback_new_1(callback_cast(popup_item_dump), diitem), "Dump");
            if (item->type < type_line) {
                struct coord co;
                struct pcoord *c;
                if (item_coord_get(item, &co, 1)) {
                    c=g_new(struct pcoord, 1);
                    c->pro = transform_get_projection(navit_get_trans(nav));
                    c->x = co.x;
                    c->y = co.y;
                    popup_printf_cb(menu_item, menu_type_menu, callback_new_2(callback_cast(popup_set_position), nav, c),
                                    _("Set as position"));
                    popup_printf_cb(menu_item, menu_type_menu, callback_new_2(callback_cast(popup_set_destination), nav, c),
                                    _("Set as destination"));
                    popup_printf_cb(menu_item, menu_type_menu, callback_new_2(callback_cast(popup_set_bookmark), nav, c),
                                    _("Add as bookmark"));
                }
            }
        }
        map_rect_destroy(mr);
    } else {
        popup_printf(menu, menu_type_menu, "(No map)");
    }
    if (item_get_default_flags(diitem->type)) {
        int speeds[]= {5,10,20,30,40,50,60,70,80,90,100};
        int delays[]= {1,2,3,5,10,15,20,30,45,60,75,90,120,150,180,240,300};
        int i;
        menu_dist=popup_printf(menu, menu_type_submenu, "Traffic distortion");
        popup_printf_cb(menu_dist, menu_type_menu, callback_new_1(callback_cast(popup_traffic_distortion_blocked), diitem),
                        "Blocked");
        menu_item=popup_printf(menu_dist, menu_type_submenu,"Max speed");
        for (i = 0 ; i < sizeof(speeds)/sizeof(int); i++) {
            popup_printf_cb(menu_item, menu_type_menu, callback_new_2(callback_cast(popup_traffic_distortion_speed), diitem,
                            speeds[i]), "%d km/h",speeds[i]);
        }
        menu_item=popup_printf(menu_dist, menu_type_submenu,"Delay");
        for (i = 0 ; i < sizeof(delays)/sizeof(int); i++) {
            popup_printf_cb(menu_item, menu_type_menu, callback_new_2(callback_cast(popup_traffic_distortion_delay), diitem,
                            delays[i]*600), "%d min",delays[i]);
        }
    }
}

static void popup_display(struct navit *nav, void *popup, struct point *p) {
    struct displaylist_handle *dlh;
    struct displaylist *display;
    struct displayitem *di;

    display=navit_get_displaylist(nav);
    dlh=graphics_displaylist_open(display);
    while ((di=graphics_displaylist_next(dlh))) {
        if (graphics_displayitem_within_dist(display, di, p, 5)) {
            popup_show_item(nav, popup, di);
        }
    }
    graphics_displaylist_close(dlh);
}

static struct pcoord c;

void popup(struct navit *nav, int button, struct point *p) {
    void *popup,*men;
    char buffer[1024];
    struct coord_geo g;
    struct coord co;

    popup=gui_popup_new(navit_get_gui(nav));
    if (! popup)
        return;
    transform_reverse(navit_get_trans(nav), p, &co);
    men=popup_printf(popup, menu_type_submenu, _("Point 0x%x 0x%x"), co.x, co.y);
    popup_printf(men, menu_type_menu, _("Screen coord : %d %d"), p->x, p->y);
    transform_to_geo(transform_get_projection(navit_get_trans(nav)), &co, &g);
    coord_format(g.lat,g.lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
    popup_printf(men, menu_type_menu, "%s", buffer);
    popup_printf(men, menu_type_menu, "%f %f", g.lat, g.lng);
    dbg(lvl_debug,"%p %p", nav, &c);
    c.pro = transform_get_projection(navit_get_trans(nav));
    c.x = co.x;
    c.y = co.y;
    popup_printf_cb(men, menu_type_menu, callback_new_2(callback_cast(popup_set_position), nav, &c), _("Set as position"));
    popup_printf_cb(men, menu_type_menu, callback_new_2(callback_cast(popup_set_destination), nav, &c),
                    _("Set as destination"));
    popup_show_visitbefore(nav,&c,men);
    popup_printf_cb(men, menu_type_menu, callback_new_2(callback_cast(popup_set_bookmark), nav, &c), _("Add as bookmark"));
    popup_display(nav, popup, p);
    menu_popup(popup);
}
