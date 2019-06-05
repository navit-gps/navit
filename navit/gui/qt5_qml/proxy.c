#include <glib.h>

#include "item.h"
#include "attr.h"
#include "navit.h"
#include "xmlconfig.h" // for NAVIT_OBJECT
#include "layout.h"
#include "map.h"
#include "transform.h"
#include "debug.h"
#include "search.h"


char * get_icon(struct navit *nav, struct item *item) {

    struct attr layout;
    struct attr icon_src;
    GList *layer;
    navit_get_attr(nav, attr_layout, &layout, NULL);
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
                            icon[strlen(icon)-3]='s';
                            icon[strlen(icon)-2]='v';
                            icon[strlen(icon)-1]='g';
                            return icon;
                            // FIXME
                            g_free(icon);
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
    return "unknown.svg";
}

