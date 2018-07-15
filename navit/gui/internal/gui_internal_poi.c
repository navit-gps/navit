#include <glib.h>
#include <stdlib.h>
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "navit_nls.h"
#include "item.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "layout.h"
#include "route.h"
#include "transform.h"
#include "linguistics.h"
#include "fib.h"
#include "util.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"
#include "gui_internal_poi.h"


struct item_data {
    int dist;
    char *label;
    struct item item;
    struct coord c;
};

struct selector {
    char *icon;
    char *name;
    enum item_type *types;
};
static enum item_type selectors_BankTypes[]= {type_poi_bank,type_poi_bank, type_poi_atm,type_poi_atm, type_none};
static enum item_type selectors_FuelTypes[]= {type_poi_fuel,type_poi_fuel,type_none};
static enum item_type selectors_BusTrainTypes[]= {type_poi_rail_station,type_poi_rail_station,
                                                  type_poi_rail_halt,type_poi_rail_tram_stop,type_poi_bus_station,type_poi_bus_stop,type_none
                                                 };
static enum item_type selectors_HotelTypes[]= {type_poi_hotel,type_poi_camp_rv,type_poi_camping,type_poi_camping,
                                               type_poi_resort,type_poi_resort,type_poi_motel,type_poi_hostel,type_none
                                              };
static enum item_type selectors_RestaurantTypes[]= {type_poi_bar,type_poi_picnic,type_poi_burgerking,type_poi_fastfood,
                                                    type_poi_restaurant,type_poi_restaurant,type_poi_cafe,type_poi_cafe,type_poi_pub,type_poi_pub,type_none
                                                   };
static enum item_type selectors_ShoppingTypes[]= {type_poi_mall,type_poi_mall,type_poi_shop_grocery,type_poi_shop_grocery,
                                                  type_poi_shopping,type_poi_shopping,type_poi_shop_butcher,type_poi_shop_baker,type_poi_shop_fruit,
                                                  type_poi_shop_fruit,type_poi_shop_beverages,type_poi_shop_beverages,type_none
                                                 };
static enum item_type selectors_ServiceTypes[]= {type_poi_marina,type_poi_marina,type_poi_hospital,type_poi_hospital,
                                                 type_poi_public_utilities,type_poi_public_utilities,type_poi_police,type_poi_autoservice,type_poi_information,
                                                 type_poi_information,type_poi_pharmacy,type_poi_pharmacy,type_poi_personal_service,type_poi_repair_service,
                                                 type_poi_restroom,type_poi_restroom,type_none
                                                };
static enum item_type selectors_ParkingTypes[]= {type_poi_car_parking,type_poi_car_parking,type_none};
static enum item_type selectors_LandFeaturesTypes[]= {type_poi_land_feature,type_poi_rock,type_poi_dam,type_poi_dam,
                                                      type_poi_peak,type_poi_peak,type_none
                                                     };
static enum item_type selectors_OtherTypes[]= {type_point_unspecified,type_poi_land_feature-1,type_poi_rock+1,type_poi_fuel-1,
                                               type_poi_marina+1,type_poi_shopping-1,type_poi_shopping+1,type_poi_car_parking-1,type_poi_car_parking+1,
                                               type_poi_bar-1,type_poi_bank+1,type_poi_dam-1,type_poi_dam+1,type_poi_information-1,type_poi_information+1,
                                               type_poi_mall-1,type_poi_mall+1,type_poi_personal_service-1,type_poi_pharmacy+1,type_poi_repair_service-1,
                                               type_poi_repair_service+1,type_poi_restaurant-1,type_poi_restaurant+1,type_poi_restroom-1,type_poi_restroom+1,
                                               type_poi_shop_grocery-1,type_poi_shop_grocery+1,type_poi_peak-1,type_poi_peak+1,type_poi_motel-1,type_poi_hostel+1,
                                               type_poi_shop_butcher-1,type_poi_shop_baker+1,type_poi_shop_fruit-1,type_poi_shop_fruit+1,type_poi_shop_beverages-1,
                                               type_poi_shop_beverages+1,type_poi_pub-1,type_poi_atm+1,type_line-1,type_none
                                              };
/*static enum item_type selectors_UnknownTypes[]={type_point_unkn,type_point_unkn,type_none};*/
struct selector selectors[]= {
    {"bank","Bank",selectors_BankTypes},
    {"fuel","Fuel",selectors_FuelTypes},
    {"bus_stop","Bus&Train",selectors_BusTrainTypes},
    {"hotel","Hotel",selectors_HotelTypes},
    {"restaurant","Restaurant",selectors_RestaurantTypes},
    {"shopping","Shopping",selectors_ShoppingTypes},
    {"hospital","Service",selectors_ServiceTypes},
    {"parking","Parking",selectors_ParkingTypes},
    {"peak","Land Features",selectors_LandFeaturesTypes},
    {"unknown","Other",selectors_OtherTypes},
    /*	{"unknown","Unknown",selectors_UnknownTypes},*/
};
/**
 * @brief Get icon for given POI type.
 *
 * @param this pointer to gui context
 * @param type POI type
 * @return  Pointer to graphics_image object, or NULL if no picture available.
 */

static struct graphics_image *gui_internal_poi_icon(struct gui_priv *this, struct item *item) {
    struct attr layout;
    struct attr icon_src;
    GList *layer;
    navit_get_attr(this->nav, attr_layout, &layout, NULL);
    layer=layout.u.layout->layers;
    while(layer) {
        GList *itemgra=((struct layer *)layer->data)->itemgras;
        while(itemgra) {
            GList *types=((struct itemgra *)itemgra->data)->type;
            while(types) {
                if((long)types->data==item->type) {
                    GList *element=((struct itemgra *)itemgra->data)->elements;
                    while(element) {
                        struct element * el=element->data;
                        if(el->type==element_icon) {
                            char *src;
                            char *icon;
                            struct graphics_image *img;
                            if(item_is_custom_poi(*item)) {
                                struct map_rect *mr=map_rect_new(item->map, NULL);
                                item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
                                if(item_attr_get(item, attr_icon_src, &icon_src)) {
                                    src=el->u.icon.src;
                                    if(!src || !src[0])
                                        src="%s";
                                    icon=g_strdup_printf(src,map_convert_string_tmp(item->map,icon_src.u.str));
                                } else {
                                    icon=g_strdup(el->u.icon.src);
                                }
                            } else {
                                icon=g_strdup(el->u.icon.src);
                            }
                            char *dot=g_strrstr(icon,".");
                            dbg(lvl_debug,"%s %s", item_to_name(item->type),icon);
                            if(dot)
                                *dot=0;
                            img=image_new_xs(this,icon);
                            g_free(icon);
                            if(img)
                                return img;
                        }
                        element=g_list_next(element);
                    }
                }
                types=g_list_next(types);
            }
            itemgra=g_list_next(itemgra);
        }
        layer=g_list_next(layer);
    }
    return NULL;
}

/**
 * @brief Free poi_param structure.
 *
 * @param p reference to the object to be freed.
 */

void gui_internal_poi_param_free(void *p) {
    if(((struct poi_param *)p)->filterstr)
        g_free(((struct poi_param *)p)->filterstr);
    if(((struct poi_param *)p)->filter)
        g_list_free(((struct poi_param *)p)->filter);
    g_free(p);
};

/**
 * @brief Clone poi_param structure.
 *
 * @param p reference to the object to be cloned.
 * @return  Cloned object reference.
 */

static struct poi_param *gui_internal_poi_param_clone(struct poi_param *p) {
    struct poi_param *r=g_new(struct poi_param,1);
    GList *l=p->filter;
    memcpy(r,p,sizeof(struct poi_param));
    r->filter=NULL;
    r->filterstr=NULL;
    if(p->filterstr) {
        char *last=g_list_last(l)->data;
        int len=(last - p->filterstr) + strlen(last)+1;
        r->filterstr=g_memdup(p->filterstr,len);
    }
    while(l) {
        r->filter=g_list_append(r->filter, r->filterstr + ((char*)(l->data) - p->filterstr) );
        l=g_list_next(l);
    }
    return r;
};

/**
 * @brief Set POIs filter data in poi_param structure.
 * @param param poi_param structure with unset filter data.
 * @param text filter text.
 */

void gui_internal_poi_param_set_filter(struct poi_param *param, char *text) {
    char *s1, *s2;

    param->filterstr=removecase(text);
    s1=param->filterstr;
    do {
        s2=g_utf8_strchr(s1,-1,' ');
        if(s2)
            *s2++=0;
        param->filter=g_list_append(param->filter,s1);
        if(s2) {
            while(*s2==' ')
                s2++;
        }
        s1=s2;
    } while(s2 && *s2);
}

static struct widget *gui_internal_cmd_pois_selector(struct gui_priv *this, struct pcoord *c, int pagenb) {
    struct widget *wl,*wb;
    int nitems,nrows;
    int i;
    //wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal_vertical|flags_fill);
    wl->background=this->background;
    wl->w=this->root.w;
    wl->cols=this->root.w/this->icon_s;
    nitems=sizeof(selectors)/sizeof(struct selector);
    nrows=nitems/wl->cols + (nitems%wl->cols>0);
    wl->h=this->icon_l*nrows;
    for (i = 0 ; i < nitems ; i++) {
        struct poi_param *p=g_new0(struct poi_param,1);
        p->sel = 1;
        p->selnb = i;
        p->pagenb = pagenb;
        p->dist = 0;
        p->filter=NULL;
        p->filterstr=NULL;
        gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
                                          image_new_s(this, selectors[i].icon), gravity_left_center|orientation_vertical,
                                          gui_internal_cmd_pois, p));
        wb->c=*c;
        wb->data_free=gui_internal_poi_param_free;
        wb->bt=10;
    }

    gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
                                      image_new_s(this, "gui_search"), gravity_left_center|orientation_vertical,
                                      gui_internal_cmd_pois_filter, NULL));
    wb->c=*c;
    wb->bt=10;

    gui_internal_widget_pack(this,wl);
    return wl;
}

/**
 * @brief Widget to display POI item.
 *
 * @param this pointer to gui context
 * @param center reference to the standing point where angle to be counted from
 * @param item POI item reference
 * @param c POI coordinates
 * @param dist precomputed distance to POI (or -1 if it's not to be displayed)
 * @param name POI name
 * @return  Pointer to new widget.
 */

static void format_dist(int dist, char *distbuf) {
    if (dist > 10000)
        sprintf(distbuf,"%d ", dist/1000);
    else if (dist>0)
        sprintf(distbuf,"%d.%d ", dist/1000, (dist%1000)/100);
}

struct widget *
gui_internal_cmd_pois_item(struct gui_priv *this, struct coord *center, struct item *item, struct coord *c,
                           struct route *route, int dist, char* name) {
    char distbuf[32]="";
    char dirbuf[32]="";
    char routedistbuf[32]="";
    char *type;
    struct widget *wl;
    char *text;
    struct graphics_image *icon;

    format_dist(dist,distbuf);
    if(c) {
        int len;
        get_compass_direction(dirbuf, transform_get_angle_delta(center, c, 0), 1);
        len=strlen(dirbuf);
        dirbuf[len]=' ';
        dirbuf[len+1]=0;
        if (route) {
            route_get_distances(route, c, 1, &dist);
            if (dist != INT_MAX)
                format_dist(dist, routedistbuf);
        }
    }


    type=item_to_name(item->type);

    icon=gui_internal_poi_icon(this,item);
    if(!icon && item->type==type_house_number)
        icon=image_new_xs(this,"post");
    if(!icon) {
        icon=image_new_xs(this,"gui_inactive");
        text=g_strdup_printf("%s%s%s%s %s", distbuf, dirbuf, routedistbuf, type, name);
    } else if(strlen(name)>0)
        text=g_strdup_printf("%s%s%s%s", distbuf, dirbuf, routedistbuf, name);
    else
        text=g_strdup_printf("%s%s%s%s", distbuf, dirbuf, routedistbuf, type);

    wl=gui_internal_button_new_with_callback(this, text, icon, gravity_left_center|orientation_horizontal|flags_fill, NULL,
            NULL);
    wl->datai=dist;
    g_free(text);
    if (name[0]) {
        wl->name=g_strdup_printf("%s %s",type,name);
    } else {
        wl->name=g_strdup(type);
    }
    wl->func=gui_internal_cmd_position;
    wl->data=(void *)9;
    wl->item=*item;
    wl->state|= STATE_SENSITIVE;
    return wl;
}

/**
 * @brief Get string representation of item address suitable for doing search and for display in POI list.
 *
 * @param item reference to item.
 * @return  Pointer to string representation of address. To be g_free()d after use.
 */

char *gui_internal_compose_item_address_string(struct item *item, int prependPostal) {
    char *s=g_strdup("");
    struct attr attr;
    if(prependPostal && item_attr_get(item, attr_postal, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_house_number, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_street_name, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_street_name_systematic, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_district_name, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_town_name, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_county_name, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);
    if(item_attr_get(item, attr_country_name, &attr))
        s=g_strjoin(" ",s,map_convert_string_tmp(item->map,attr.u.str),NULL);

    if(item_attr_get(item, attr_address, &attr))
        s=g_strjoin(" ",s,"|",map_convert_string_tmp(item->map,attr.u.str),NULL);
    return s;
}

static int gui_internal_cmd_pois_item_selected(struct poi_param *param, struct item *item) {
    enum item_type *types;
    struct selector *sel = param->sel? &selectors[param->selnb]: NULL;
    enum item_type type=item->type;
    struct attr attr;
    int match=0;
    if (type >= type_line && param->filter==NULL)
        return 0;
    if (! sel || !sel->types) {
        match=1;
    } else {
        types=sel->types;
        while (*types != type_none) {
            if (item->type >= types[0] && item->type <= types[1]) {
                return 1;
            }
            types+=2;
        }
    }
    if(type == type_house_number && !param->filter)
        return 0;
    if (param->filter) {
        char *long_name, *s;
        GList *f;
        int i;
        if (param->AddressFilterType>0) {
            s=gui_internal_compose_item_address_string(item,param->AddressFilterType==2?1:0);
        } else if (item_attr_get(item, attr_label, &attr)) {
            s=g_strdup_printf("%s %s", item_to_name(item->type), map_convert_string_tmp(item->map,attr.u.str));
        } else {
            s=g_strdup(item_to_name(item->type));
        }
        long_name=removecase(s);
        g_free(s);

        match=0;
        for(i=0; i<3 && !match; i++) {
            char *long_name_exp=linguistics_expand_special(long_name, i);
            for(s=long_name_exp,f=param->filter; f && s; f=g_list_next(f)) {
                s=strstr(s,f->data);
                if(!s) {
                    break;
                }
                s=g_utf8_strchr(s,-1,' ');
            }
            g_free(long_name_exp);
            if(!f)
                match=1;
        }
        g_free(long_name);
    }
    return match;
}

/**
 * @brief Event handler for POIs list "more" element.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data.
 */
static void gui_internal_cmd_pois_more(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w=g_new0(struct widget,1);
    w->data=wm->data;
    w->c=wm->c;
    w->w=wm->w;
    wm->data_free=NULL;
    gui_internal_back(this, NULL, NULL);
    gui_internal_cmd_pois(this, w, w->data);
    free(w);
}


/**
 * @brief Event to apply POIs text filter.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data (pointer to editor widget containg filter text).
 */
static void gui_internal_cmd_pois_filter_do(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *w=data;
    struct poi_param *param;

    if(!w->text)
        return;

    if(w->data) {
        param=gui_internal_poi_param_clone(w->data);
        param->pagenb=0;
    } else {
        param=g_new0(struct poi_param,1);
    }
    if(!strcmp(wm->name,"AddressFilter"))
        param->AddressFilterType=1;
    else if(!strcmp(wm->name,"AddressFilterZip"))
        param->AddressFilterType=2;
    else
        param->AddressFilterType=0;

    gui_internal_poi_param_set_filter(param, w->text);

    gui_internal_cmd_pois(this,w,param);
    gui_internal_poi_param_free(param);
}

/**
 * @brief POIs filter dialog.
 * Event to handle '\r' '\n' keys pressed.
 *
 */

static void gui_internal_cmd_pois_filter_changed(struct gui_priv *this, struct widget *wm, void *data) {
    if (wm->text && wm->reason==gui_internal_reason_keypress_finish) {
        gui_internal_cmd_pois_filter_do(this, wm, wm);
    }
}


/**
 * @brief POIs filter dialog.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data.
 */
void gui_internal_cmd_pois_filter(struct gui_priv *this, struct widget *wm, void *data) {
    struct widget *wb, *w, *wr, *wk, *we;
    int keyboard_mode;
    keyboard_mode = VKBD_FLAG_2 | gui_internal_keyboard_init_mode(getenv("LANG"));
    wb=gui_internal_menu(this,"Filter");
    w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wr);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(wr, we);

    gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
    wk->state |= STATE_EDIT|STATE_EDITABLE;
    wk->func=gui_internal_cmd_pois_filter_changed;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->name=g_strdup("POIsFilter");
    wk->c=wm->c;
    gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
    wb->state |= STATE_SENSITIVE;
    wb->func = gui_internal_cmd_pois_filter_do;
    wb->name=g_strdup("NameFilter");
    wb->data=wk;
    gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "post")));
    wb->state |= STATE_SENSITIVE;
    wb->name=g_strdup("AddressFilter");
    wb->func = gui_internal_cmd_pois_filter_do;
    wb->data=wk;
    gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "zipcode")));
    wb->state |= STATE_SENSITIVE;
    wb->name=g_strdup("AddressFilterZip");
    wb->func = gui_internal_cmd_pois_filter_do;
    wb->data=wk;

    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this, keyboard_mode));
    else
        gui_internal_keyboard_show_native(this, w, keyboard_mode, getenv("LANG"));
    gui_internal_menu_render(this);


}

/**
 * @brief Do POI search specified by poi_param and display POIs found
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data, reference to poi_param or NULL.
 */
void gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data) {
    struct map_selection *sel,*selm;
    struct coord c,center;
    struct mapset_handle *h;
    struct map *m;
    struct map_rect *mr;
    struct item *item;
    struct widget *wi,*w,*w2,*wb, *wtable, *row;
    enum projection pro=wm->c.pro;
    struct poi_param *param;
    int param_free=0;
    int idist,dist;
    struct selector *isel;
    int pagenb;
    int prevdist;
    // Starting value and increment of count of items to be extracted
    const int pagesize = 50;
    int maxitem, it = 0, i;
    struct item_data *items;
    struct fibheap* fh = fh_makekeyheap();
    int cnt = 0;
    struct table_data *td;
    struct widget *wl,*wt;
    char buffer[32];
    struct poi_param *paramnew;
    struct attr route;
    dbg(lvl_debug,"POIs...");
    if(data) {
        param = data;
    } else {
        param = g_new0(struct poi_param,1);
        param_free=1;
    }
    if (navit_get_attr(this->nav, attr_route, &route, NULL)) {
        struct attr route_status;
        if (!route_get_attr(route.u.route, attr_route_status, &route_status, NULL) ||
                (route_status.u.num != route_status_path_done_new &&
                 route_status.u.num != route_status_path_done_incremental))
            route.u.route=NULL;
    } else
        route.u.route=NULL;
    dist=10000*(param->dist+1);
    isel = param->sel? &selectors[param->selnb]: NULL;
    pagenb = param->pagenb;
    prevdist=param->dist*10000;
    maxitem = pagesize*(pagenb+1);
    items= g_new0( struct item_data, maxitem);


    dbg(lvl_debug, "Params: sel = %i, selnb = %i, pagenb = %i, dist = %i, filterstr = %s, AddressFilterType= %d",
        param->sel, param->selnb, param->pagenb, param->dist, param->filterstr, param->AddressFilterType);

    wb=gui_internal_menu(this, isel ? isel->name : _("POIs"));
    w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    if (!isel && !param->filter)
        gui_internal_widget_append(w, gui_internal_cmd_pois_selector(this,&wm->c,pagenb));
    w2=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, w2);

    sel=map_selection_rect_new(&wm->c,dist*transform_scale(abs(wm->c.y)+dist*1.5),18);
    center.x=wm->c.x;
    center.y=wm->c.y;
    h=mapset_open(navit_get_mapset(this->nav));
    while ((m=mapset_next(h, 1))) {
        selm=map_selection_dup_pro(sel, pro, map_projection(m));
        mr=map_rect_new(m, selm);
        dbg(lvl_debug,"mr=%p", mr);
        if (mr) {
            while ((item=map_rect_get_item(mr))) {
                if (gui_internal_cmd_pois_item_selected(param, item) &&
                        item_coord_get_pro(item, &c, 1, pro) &&
                        coord_rect_contains(&sel->u.c_rect, &c)  &&
                        (idist=transform_distance(pro, &center, &c)) < dist) {
                    struct item_data *data;
                    struct attr attr;
                    char *label;
                    item_attr_rewind(item);
                    if (item->type==type_house_number) {
                        label=gui_internal_compose_item_address_string(item,1);
                    } else if (item_attr_get(item, attr_label, &attr)) {
                        label=map_convert_string(item->map,attr.u.str);
                        // Buildings which label is equal to addr:housenumber value
                        // are duplicated by item_house_number. Don't include such
                        // buildings into the list. This is true for OSM maps created with
                        // maptool patched with #859 latest patch.
                        // FIXME: For non-OSM maps, we probably would better don't skip these items.
                        if(item->type==type_poly_building && item_attr_get(item, attr_house_number, &attr) ) {
                            if(strcmp(label,map_convert_string_tmp(item->map,attr.u.str))==0) {
                                g_free(label);
                                continue;
                            }
                        }

                    } else {
                        label=g_strdup("");
                    }

                    if(it>=maxitem) {
                        data = fh_extractmin(fh);
                        g_free(data->label);
                        data->label=NULL;
                    } else {
                        data = &items[it++];
                    }
                    data->label=label;
                    data->item = *item;
                    data->c = c;
                    data->dist = idist;
                    // Key expression is a workaround to fight
                    // probable heap collisions when two objects
                    // are at the same distance. But it destroys
                    // right order of POIs 2048 km away from cener
                    // and if table grows more than 1024 rows.
                    fh_insertkey(fh, -((idist<<10) + cnt++), data);
                    if (it == maxitem)
                        dist = (-fh_minkey(fh))>>10;
                }
            }
            map_rect_destroy(mr);
        }
        map_selection_destroy(selm);
    }
    map_selection_destroy(sel);
    mapset_close(h);

    wtable = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
    td=wtable->data;

    gui_internal_widget_append(w2,wtable);

    // Move items from heap to the table
    for(i=0;; i++) {
        int key = fh_minkey(fh);
        struct item_data *data = fh_extractmin(fh);
        if (data == NULL) {
            dbg(lvl_debug, "Empty heap: maxitem = %i, it = %i, dist = %i", maxitem, it, dist);
            break;
        }
        dbg(lvl_debug, "dist1: %i, dist2: %i", data->dist, (-key)>>10);
        if(i==(it-pagesize*pagenb) && data->dist>prevdist)
            prevdist=data->dist;
        wi=gui_internal_cmd_pois_item(this, &center, &data->item, &data->c, route.u.route, data->dist, data->label);
        wi->c.x=data->c.x;
        wi->c.y=data->c.y;
        wi->c.pro=pro;
        wi->background=this->background;
        row = gui_internal_widget_table_row_new(this,
                                                gravity_left
                                                | flags_fill
                                                | orientation_horizontal);
        gui_internal_widget_append(row,wi);
        row->datai=data->dist;
        gui_internal_widget_prepend(wtable,row);
        g_free(data->label);
    }

    fh_deleteheap(fh);
    free(items);

    // Add an entry for more POI
    row = gui_internal_widget_table_row_new(this,
                                            gravity_left
                                            | flags_fill
                                            | orientation_horizontal);
    row->datai=100000000; // Really far away for Earth, but won't work for bigger planets.
    gui_internal_widget_append(wtable,row);
    wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(row,wl);
    if (it == maxitem) {
        paramnew=gui_internal_poi_param_clone(param);
        paramnew->pagenb++;
        paramnew->count=it;
        snprintf(buffer, sizeof(buffer), "Get more (up to %d items)...", (paramnew->pagenb+1)*pagesize);
        wt=gui_internal_label_new(this, buffer);
        gui_internal_widget_append(wl, wt);
        wt->func=gui_internal_cmd_pois_more;
        wt->data=paramnew;
        wt->data_free=gui_internal_poi_param_free;
        wt->state |= STATE_SENSITIVE;
        wt->c = wm->c;
    } else {
        static int dist[]= {1,5,10,0};
        wt=gui_internal_label_new(this, "Set distance to");
        gui_internal_widget_append(wl, wt);
        for(i=0; dist[i]; i++) {
            paramnew=gui_internal_poi_param_clone(param);
            paramnew->dist+=dist[i];
            paramnew->count=it;
            snprintf(buffer, sizeof(buffer), " %i ", 10*(paramnew->dist+1));
            wt=gui_internal_label_new(this, buffer);
            gui_internal_widget_append(wl, wt);
            wt->func=gui_internal_cmd_pois_more;
            wt->data=paramnew;
            wt->data_free=gui_internal_poi_param_free;
            wt->state |= STATE_SENSITIVE;
            wt->c = wm->c;
        }
        wt=gui_internal_label_new(this, "km.");
        gui_internal_widget_append(wl, wt);

    }
    // Rendering now is needed to have table_data->bottomrow filled in.
    gui_internal_menu_render(this);
    td=wtable->data;
    if(td->bottom_row!=NULL) {
#if 0
        while(((struct widget*)td->bottom_row->data)->datai<=prevdist
                && (td->next_button->state & STATE_SENSITIVE)) {
            gui_internal_table_button_next(this, td->next_button, NULL);
        }
#else
        int firstrow=g_list_index(wtable->children, td->top_row->data);
        while(firstrow>=0) {
            int currow=g_list_index(wtable->children, td->bottom_row->data) - firstrow;
            if(currow<0) {
                dbg(lvl_debug,"Can't find bottom row in children list. Stop paging.");
                break;
            }
            if(currow>=param->count)
                break;
            if(!(td->scroll_buttons.next_button->state & STATE_SENSITIVE)) {
                dbg(lvl_debug,"Reached last page but item %i not found. Stop paging.",param->count);
                break;
            }
            gui_internal_table_button_next(this, td->scroll_buttons.next_button, NULL);
        }
#endif
    }
    gui_internal_menu_render(this);
    if(param_free)
        g_free(param);
}

