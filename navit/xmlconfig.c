/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

/* see http://library.gnome.org/devel/glib/stable/glib-Simple-XML-Subset-Parser.html
 * for details on how the xml file parser works.
 */

#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "file.h"
#include "coord.h"
#include "item.h"
#include "xmlconfig.h"
#include "layout.h"
#include "mapset.h"
#include "projection.h"
#include "map.h"
#include "navigation.h"
#include "navit.h"
#include "plugin.h"
#include "route.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "point.h"
#include "graphics.h"
#include "gui.h"
#include "osd.h"
#include "log.h"
#include "announcement.h"
#include "vehicleprofile.h"
#include "callback.h"
#include "config_.h"

#if (defined __MINGW32__) || (defined _MSC_VER)
/* This only works if a is a string constant, i.e. "name" */
#define unsetenv(a) putenv(a "=")
#endif

struct xistate {
    struct xistate *parent;
    struct xistate *child;
    const gchar *element;
    const gchar **attribute_names;
    const gchar **attribute_values;
};

struct xmldocument {
    const gchar *href;
    const gchar *xpointer;
    gpointer user_data;
    struct xistate *first;
    struct xistate *last;
    int active;
    int level;
};


struct xmlstate {
    const gchar **attribute_names;
    const gchar **attribute_values;
    struct xmlstate *parent;
    struct attr element_attr;
    const gchar *element;
    xmlerror **error;
    struct element_func *func;
    struct object_func *object_func;
    struct xmldocument *document;
};


struct attr_fixme {
    char *element;
    char **attr_fixme;
};

static struct attr ** convert_to_attrs(struct xmlstate *state, struct attr_fixme *fixme) {
    const gchar **attribute_name=state->attribute_names;
    const gchar **attribute_value=state->attribute_values;
    const gchar *name;
    int count=0;
    struct attr **ret;
    static int fixme_count;

    while (*attribute_name) {
        count++;
        attribute_name++;
    }
    ret=g_new(struct attr *, count+1);
    attribute_name=state->attribute_names;
    count=0;
    while (*attribute_name) {
        name=*attribute_name;
        if (fixme) {
            char **attr_fixme=fixme->attr_fixme;
            while (attr_fixme[0]) {
                if (! strcmp(name, attr_fixme[0])) {
                    name=attr_fixme[1];
                    if (fixme_count++ < 10)
                        dbg(lvl_error,"Please change attribute '%s' to '%s' in <%s />", attr_fixme[0], attr_fixme[1], fixme->element);
                    break;
                }
                attr_fixme+=2;
            }
        }
        ret[count]=attr_new_from_text(name,*attribute_value);
        if (ret[count])
            count++;
        else if (strcmp(*attribute_name,"enabled") && strcmp(*attribute_name,"xmlns:xi"))
            dbg(lvl_error,"failed to create attribute '%s' with value '%s'", *attribute_name,*attribute_value);
        attribute_name++;
        attribute_value++;
    }
    ret[count]=NULL;
    dbg(lvl_debug,"ret=%p", ret);
    return ret;
}


static const char * find_attribute(struct xmlstate *state, const char *attribute, int required) {
    const gchar **attribute_name=state->attribute_names;
    const gchar **attribute_value=state->attribute_values;
    while(*attribute_name) {
        if(! g_ascii_strcasecmp(attribute,*attribute_name))
            return *attribute_value;
        attribute_name++;
        attribute_value++;
    }
    if (required)
        g_set_error(state->error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "element '%s' is missing attribute '%s'",
                    state->element, attribute);
    return NULL;
}

static int find_boolean(struct xmlstate *state, const char *attribute, int deflt, int required) {
    const char *value;

    value=find_attribute(state, attribute, required);
    if (! value)
        return deflt;
    if (g_ascii_strcasecmp(value,"no") && g_ascii_strcasecmp(value,"0") && g_ascii_strcasecmp(value,"false"))
        return 1;
    return 0;
}

/**
 * * Convert a string number to int
 * *
 * * @param val the string value to convert
 * * @returns int value of converted string
 * */
static int convert_number(const char *val) {
    if (val)
        return g_ascii_strtoull(val,NULL,0);
    else
        return 0;
}

static int xmlconfig_announce(struct xmlstate *state) {
    const char *type,*value;
    char key[32];
    int level[3];
    int i;
    enum item_type itype;
    char *tok, *type_str, *str;

    type=find_attribute(state, "type", 1);
    if (! type)
        return 0;
    for (i = 0 ; i < 3 ; i++) {
        sprintf(key,"level%d", i);
        value=find_attribute(state, key, 0);
        if (value)
            level[i]=convert_number(value);
        else
            level[i]=-1;
    }
    type_str=g_strdup(type);
    str=type_str;
    while ((tok=strtok(str, ","))) {
        itype=item_from_name(tok);
        if (itype!=type_none) {
            navigation_set_announce(state->parent->element_attr.u.data, itype, level);
        } else {
            dbg(lvl_error, "Invalid type for announcement: %s",tok);
        }
        str=NULL;
    }
    g_free(type_str);
    return 1;
}
/**
 * * Define the elements in our config
 * *
 * */

#define NEW(x) (void *(*)(struct attr *, struct attr **))(x)
#define GET(x) (int (*)(void *, enum attr_type type, struct attr *attr, struct attr_iter *iter))(x)
#define ITERN(x) (struct attr_iter * (*)(void *))(x)
#define ITERD(x) (void (*)(struct attr_iter *iter))(x)
#define SET(x) (int (*)(void *, struct attr *attr))(x)
#define ADD(x) (int (*)(void *, struct attr *attr))(x)
#define REMOVE(x) (int (*)(void *, struct attr *attr))(x)
#define INIT(x) (int (*)(void *))(x)
#define DESTROY(x) (void (*)(void *))(x)

static struct object_func object_funcs[] = {
    { attr_announcement,NEW(announcement_new),  GET(announcement_get_attr), NULL, NULL, SET(announcement_set_attr), ADD(announcement_add_attr) },
    { attr_arrows,     NEW(arrows_new)},
    { attr_circle,     NEW(circle_new),   NULL, NULL, NULL, NULL, ADD(element_add_attr)},
    { attr_coord,      NEW(coord_new_from_attrs)},
    { attr_cursor,     NEW(cursor_new),   NULL, NULL, NULL, NULL, ADD(cursor_add_attr)},
    { attr_debug,      NEW(debug_new)},
    { attr_graphics,   NEW(graphics_new), GET(graphics_get_attr)},
    { attr_gui,        NEW(gui_new), GET(gui_get_attr), NULL, NULL, SET(gui_set_attr), ADD(gui_add_attr)},
    { attr_icon,       NEW(icon_new),     NULL, NULL, NULL, NULL, ADD(element_add_attr)},
    { attr_image,      NEW(image_new)},
    { attr_itemgra,    NEW(itemgra_new),  NULL, NULL, NULL, NULL, ADD(itemgra_add_attr)},
    { attr_plugins,    NEW(plugins_new),  NULL, NULL, NULL, NULL, NULL, NULL, INIT(plugins_init)},
    { attr_plugin,     NEW(plugin_new)},
    { attr_polygon,    NEW(polygon_new),  NULL, NULL, NULL, NULL, ADD(element_add_attr)},
    { attr_polyline,   NEW(polyline_new), NULL, NULL, NULL, NULL, ADD(element_add_attr)},
    { attr_text,       NEW(text_new)},
};

struct object_func *
object_func_lookup(enum attr_type type) {
    int i;
    switch (type) {
    case attr_config:
        return &config_func;
    case attr_layer:
        return &layer_func;
    case attr_layout:
        return &layout_func;
    case attr_log:
        return &log_func;
    case attr_map:
        return &map_func;
    case attr_maps:
        return &maps_func;
    case attr_mapset:
        return &mapset_func;
    case attr_navigation:
        return &navigation_func;
    case attr_navit:
        return &navit_func;
    case attr_profile_option:
        return &profile_option_func;
    case attr_roadprofile:
        return &roadprofile_func;
    case attr_route:
        return &route_func;
    case attr_script:
        return &script_func;
    case attr_osd:
        return &osd_func;
    case attr_trackingo:
        return &tracking_func;
    case attr_speech:
        return &speech_func;
    case attr_vehicle:
        return &vehicle_func;
    case attr_vehicleprofile:
        return &vehicleprofile_func;
    default:
        for (i = 0 ; i < sizeof(object_funcs)/sizeof(struct object_func); i++) {
            if (object_funcs[i].type == type)
                return &object_funcs[i];
        }
        return NULL;
    }
}

struct element_func {
    char *name;
    char *parent;
    int (*func)(struct xmlstate *state);
    enum attr_type type;
};
struct element_func *elements;

static char *attr_fixme_itemgra[]= {
    "type","item_types",
    NULL,NULL,
};

static char *attr_fixme_text[]= {
    "label_size","text_size",
    NULL,NULL,
};

static char *attr_fixme_circle[]= {
    "label_size","text_size",
    NULL,NULL,
};

static struct attr_fixme attr_fixmes[]= {
    {"item",attr_fixme_itemgra},
    {"itemgra",attr_fixme_itemgra},
    {"text",attr_fixme_text},
    {"label",attr_fixme_text},
    {"circle",attr_fixme_circle},
    {NULL,NULL},
};


static char *element_fixmes[]= {
    "item","itemgra",
    "label","text",
    NULL,NULL,
};

static void initStatic(void) {
    elements=g_new0(struct element_func,44); //43 is a number of elements + ending NULL element

    elements[0].name="config";
    elements[0].parent=NULL;
    elements[0].func=NULL;
    elements[0].type=attr_config;

    elements[1].name="announce";
    elements[1].parent="navigation";
    elements[1].func=xmlconfig_announce;

    elements[2].name="speech";
    elements[2].parent="navit";
    elements[2].func=NULL;
    elements[2].type=attr_speech;

    elements[3].name="tracking";
    elements[3].parent="navit";
    elements[3].func=NULL;
    elements[3].type=attr_trackingo;

    elements[4].name="route";
    elements[4].parent="navit";
    elements[4].func=NULL;
    elements[4].type=attr_route;

    elements[5].name="mapset";
    elements[5].parent="navit";
    elements[5].func=NULL;
    elements[5].type=attr_mapset;

    elements[6].name="map";
    elements[6].parent="mapset";
    elements[6].func=NULL;
    elements[6].type=attr_map;

    elements[7].name="debug";
    elements[7].parent="config";
    elements[7].func=NULL;
    elements[7].type=attr_debug;

    elements[8].name="osd";
    elements[8].parent="navit";
    elements[8].func=NULL;
    elements[8].type=attr_osd;

    elements[9].name="navigation";
    elements[9].parent="navit";
    elements[9].func=NULL;
    elements[9].type=attr_navigation;

    elements[10].name="navit";
    elements[10].parent="config";
    elements[10].func=NULL;
    elements[10].type=attr_navit;

    elements[11].name="graphics";
    elements[11].parent="navit";
    elements[11].func=NULL;
    elements[11].type=attr_graphics;

    elements[12].name="gui";
    elements[12].parent="navit";
    elements[12].func=NULL;
    elements[12].type=attr_gui;

    elements[13].name="layout";
    elements[13].parent="navit";
    elements[13].func=NULL;
    elements[13].type=attr_layout;

    elements[14].name="cursor";
    elements[14].parent="layout";
    elements[14].func=NULL;
    elements[14].type=attr_cursor;

    elements[15].name="layer";
    elements[15].parent="layout";
    elements[15].func=NULL;
    elements[15].type=attr_layer;

    elements[16].name="itemgra";
    elements[16].parent="layer";
    elements[16].func=NULL;
    elements[16].type=attr_itemgra;

    elements[17].name="circle";
    elements[17].parent="itemgra";
    elements[17].func=NULL;
    elements[17].type=attr_circle;

    elements[18].name="coord";
    elements[18].parent="circle";
    elements[18].func=NULL;
    elements[18].type=attr_coord;

    elements[19].name="icon";
    elements[19].parent="itemgra";
    elements[19].func=NULL;
    elements[19].type=attr_icon;

    elements[20].name="coord";
    elements[20].parent="icon";
    elements[20].func=NULL;
    elements[20].type=attr_coord;

    elements[21].name="image";
    elements[21].parent="itemgra";
    elements[21].func=NULL;
    elements[21].type=attr_image;

    elements[22].name="text";
    elements[22].parent="itemgra";
    elements[22].func=NULL;
    elements[22].type=attr_text;

    elements[23].name="polygon";
    elements[23].parent="itemgra";
    elements[23].func=NULL;
    elements[23].type=attr_polygon;

    elements[24].name="coord";
    elements[24].parent="polygon";
    elements[24].func=NULL;
    elements[24].type=attr_coord;

    elements[25].name="polyline";
    elements[25].parent="itemgra";
    elements[25].func=NULL;
    elements[25].type=attr_polyline;

    elements[26].name="coord";
    elements[26].parent="polyline";
    elements[26].func=NULL;
    elements[26].type=attr_coord;

    elements[27].name="arrows";
    elements[27].parent="itemgra";
    elements[27].func=NULL;
    elements[27].type=attr_arrows;

    elements[28].name="vehicle";
    elements[28].parent="navit";
    elements[28].func=NULL;
    elements[28].type=attr_vehicle;

    elements[29].name="vehicleprofile";
    elements[29].parent="navit";
    elements[29].func=NULL;
    elements[29].type=attr_vehicleprofile;

    elements[30].name="roadprofile";
    elements[30].parent="vehicleprofile";
    elements[30].func=NULL;
    elements[30].type=attr_roadprofile;

    elements[31].name="announcement";
    elements[31].parent="roadprofile";
    elements[31].func=NULL;
    elements[31].type=attr_announcement;

    elements[32].name="cursor";
    elements[32].parent="vehicle";
    elements[32].func=NULL;
    elements[32].type=attr_cursor;

    elements[33].name="itemgra";
    elements[33].parent="cursor";
    elements[33].func=NULL;
    elements[33].type=attr_itemgra;

    elements[34].name="log";
    elements[34].parent="vehicle";
    elements[34].func=NULL;
    elements[34].type=attr_log;

    elements[35].name="log";
    elements[35].parent="navit";
    elements[35].func=NULL;
    elements[35].type=attr_log;

    elements[36].name="plugins";
    elements[36].parent="config";
    elements[36].func=NULL;
    elements[36].type=attr_plugins;

    elements[37].name="plugin";
    elements[37].parent="plugins";
    elements[37].func=NULL;
    elements[37].type=attr_plugin;

    elements[38].name="maps";
    elements[38].parent="mapset";
    elements[38].func=NULL;
    elements[38].type=attr_maps;

    elements[39].name="layer";
    elements[39].parent="navit";
    elements[39].func=NULL;
    elements[39].type=attr_layer;

    elements[40].name="profile_option";
    elements[40].parent="vehicleprofile";
    elements[40].func=NULL;
    elements[40].type=attr_profile_option;

    elements[41].name="roadprofile";
    elements[41].parent="profile_option";
    elements[41].func=NULL;
    elements[41].type=attr_roadprofile;

    elements[42].name="script";
    elements[42].parent="navit";
    elements[42].func=NULL;
    elements[42].type=attr_script;
}

/**
 * * Parse the opening tag of a config element
 * *
 * * @param context document parse context
 * * @param element_name the current tag name
 * * @param attribute_names ptr to return the set of attribute names
 * * @param attribute_values ptr return the set of attribute values
 * * @param user_data ptr to xmlstate structure
 * * @param error ptr return error context
 * * @returns nothing
 * */

static void start_element(xml_context *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          xmlerror             **error) {
    struct xmlstate *new=NULL, **parent = user_data;
    struct element_func *e=elements,*func=NULL;
    struct attr_fixme *attr_fixme=attr_fixmes;
    char **element_fixme=element_fixmes;
    int found=0;
    static int fixme_count;
    const char *parent_name=NULL;
    char *s,*sep="",*possible_parents;
    struct attr *parent_attr;
    dbg(lvl_info,"name='%s' parent='%s'", element_name, *parent ? (*parent)->element:NULL);

    if (!strcmp(element_name,"xml"))
        return;
    /* determine if we have to fix any attributes */
    while (attr_fixme[0].element) {
        if (!strcmp(element_name,attr_fixme[0].element))
            break;
        attr_fixme++;
    }
    if (!attr_fixme[0].element)
        attr_fixme=NULL;

    /* tell user to fix  deprecated element names */
    while (element_fixme[0]) {
        if (!strcmp(element_name,element_fixme[0])) {
            element_name=element_fixme[1];
            if (fixme_count++ < 10)
                dbg(lvl_error,"Please change <%s /> to <%s /> in config file", element_fixme[0], element_fixme[1]);
        }
        element_fixme+=2;
    }
    /* validate that this element is valid
     * and that the element has a valid parent */
    possible_parents=g_strdup("");
    if (*parent)
        parent_name=(*parent)->element;
    while (e->name) {
        if (!g_ascii_strcasecmp(element_name, e->name)) {
            found=1;
            s=g_strconcat(possible_parents,sep,e->parent,NULL);
            g_free(possible_parents);
            possible_parents=s;
            sep=",";
            if ((parent_name && e->parent && !g_ascii_strcasecmp(parent_name, e->parent)) ||
                    (!parent_name && !e->parent))
                func=e;
        }
        e++;
    }
    if (! found) {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    "Unknown element '%s'", element_name);
        g_free(possible_parents);
        return;
    }
    if (! func) {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
                    "Element '%s' within unexpected context '%s'. Expected '%s'%s",
                    element_name, parent_name, possible_parents, ! strcmp(possible_parents,
                            "config") ? "\nPlease add <config> </config> tags at the beginning/end of your navit.xml": "");
        g_free(possible_parents);
        return;
    }
    g_free(possible_parents);

    new=g_new(struct xmlstate, 1);
    new->attribute_names=attribute_names;
    new->attribute_values=attribute_values;
    new->parent=*parent;
    new->element_attr.u.data=NULL;
    new->element=element_name;
    new->error=error;
    new->func=func;
    new->object_func=NULL;
    *parent=new;
    if (!find_boolean(new, "enabled", 1, 0))
        return;
    if (new->parent && !new->parent->element_attr.u.data)
        return;
    if (func->func) {
        if (!func->func(new)) {
            return;
        }
    } else {
        struct attr **attrs;

        new->object_func=object_func_lookup(func->type);
        if (! new->object_func)
            return;
        attrs=convert_to_attrs(new,attr_fixme);
        new->element_attr.type=attr_none;
        if (!new->parent || new->parent->element_attr.type == attr_none)
            parent_attr=NULL;
        else
            parent_attr=&new->parent->element_attr;
        new->element_attr.u.data = new->object_func->create(parent_attr, attrs);
        if (! new->element_attr.u.data)
            return;
        new->element_attr.type=attr_from_name(element_name);
        if (new->element_attr.type == attr_none)
            dbg(lvl_error,"failed to create object of type '%s'", element_name);
        if (new->element_attr.type == attr_tracking)
            new->element_attr.type=attr_trackingo;
        if (new->parent && new->parent->object_func && new->parent->object_func->add_attr)
            new->parent->object_func->add_attr(new->parent->element_attr.u.data, &new->element_attr);
    }
    return;
}


/* Called for close tags </foo> */
static void end_element (xml_context *context,
                         const gchar         *element_name,
                         gpointer             user_data,
                         xmlerror             **error) {
    struct xmlstate *curr, **state = user_data;

    if (!strcmp(element_name,"xml"))
        return;
    dbg(lvl_info,"name='%s'", element_name);
    curr=*state;
    if (curr->object_func && curr->object_func->init)
        curr->object_func->init(curr->element_attr.u.data);
    if (curr->object_func && curr->object_func->unref)
        curr->object_func->unref(curr->element_attr.u.data);
    *state=curr->parent;
    g_free(curr);
}

static gboolean parse_file(struct xmldocument *document, xmlerror **error);

static void xinclude(xml_context *context, const gchar **attribute_names, const gchar **attribute_values,
                     struct xmldocument *doc_old, xmlerror **error) {
    struct xmldocument doc_new;
    struct file_wordexp *we;
    int i,count;
    const char *href=NULL;
    char **we_files;

    if (doc_old->level >= 16) {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include recursion too deep");
        return;
    }
    memset(&doc_new, 0, sizeof(doc_new));
    i=0;
    while (attribute_names[i]) {
        if(!g_ascii_strcasecmp("href", attribute_names[i])) {
            if (!href)
                href=attribute_values[i];
            else {
                g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has more than one href");
                return;
            }
        } else if(!g_ascii_strcasecmp("xpointer", attribute_names[i])) {
            if (!doc_new.xpointer)
                doc_new.xpointer=attribute_values[i];
            else {
                g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has more than one xpointer");
                return;
            }
        } else {
            g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has invalid attributes");
            return;
        }
        i++;
    }
    if (!doc_new.xpointer && !href) {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT, "xi:include has neither href nor xpointer");
        return;
    }
    doc_new.level=doc_old->level+1;
    doc_new.user_data=doc_old->user_data;
    if (! href) {
        dbg(lvl_debug,"no href, using '%s'", doc_old->href);
        doc_new.href=doc_old->href;
        if (file_exists(doc_new.href)) {
            parse_file(&doc_new, error);
        } else {
            dbg(lvl_error,"Unable to include %s",doc_new.href);
        }
    } else {
        dbg(lvl_debug,"expanding '%s'", href);
        we=file_wordexp_new(href);
        we_files=file_wordexp_get_array(we);
        count=file_wordexp_get_count(we);
        dbg(lvl_debug,"%d results", count);
        if (file_exists(we_files[0])) {
            for (i = 0 ; i < count ; i++) {
                dbg(lvl_debug,"result[%d]='%s'", i, we_files[i]);
                doc_new.href=we_files[i];
                parse_file(&doc_new, error);
            }
        } else {
            dbg(lvl_error,"Unable to include %s",we_files[0]);
        }
        file_wordexp_destroy(we);

    }

}
static int strncmp_len(const char *s1, int s1len, const char *s2) {
    int ret;
    ret=strncmp(s1, s2, s1len);
    if (ret)
        return ret;
    return strlen(s2)-s1len;
}

static int xpointer_value(const char *test, int len, struct xistate *elem, const char **out, int out_len) {
    int i,ret=0;
    if (len <= 0 || out_len <= 0) {
        return 0;
    }
    if (!(strncmp_len(test,len,"name(.)"))) {
        out[0]=elem->element;
        return 1;
    }
    if (test[0] == '@') {
        i=0;
        while (elem->attribute_names[i] && out_len > 0) {
            if (!strncmp_len(test+1,len-1,elem->attribute_names[i])) {
                out[ret++]=elem->attribute_values[i];
                out_len--;
            }
            i++;
        }
        return ret;
    }
    return 0;
}

static int xpointer_test(const char *test, int len, struct xistate *elem) {
    int eq,i,count,vlen,cond_req=1,cond=0;
    char c;
    const char *tmp[16];
    if (!len)
        return 0;
    c=test[len-1];
    if (c != '\'' && c != '"')
        return 0;
    eq=strcspn(test, "=");
    if (eq >= len || test[eq+1] != c)
        return 0;
    vlen=eq;
    if (eq > 0 && test[eq-1] == '!') {
        cond_req=0;
        vlen--;
    }
    count=xpointer_value(test,vlen,elem,tmp,16);
    for (i = 0 ; i < count ; i++) {
        if (!strncmp_len(test+eq+2,len-eq-3, tmp[i]))
            cond=1;
    }
    if (cond == cond_req)
        return 1;
    return 0;
}

static int xpointer_element_match(const char *xpointer, int len, struct xistate *elem) {
    int start,tlen;
    start=strcspn(xpointer, "[");
    if (start > len)
        start=len;
    if (strncmp_len(xpointer, start, elem->element) && (start != 1 || xpointer[0] != '*'))
        return 0;
    if (start == len)
        return 1;
    if (xpointer[len-1] != ']')
        return 0;
    for (;;) {
        start++;
        tlen=strcspn(xpointer+start,"]");
        if (start + tlen > len)
            return 1;
        if (!xpointer_test(xpointer+start, tlen, elem))
            return 0;
        start+=tlen+1;
    }
}

static int xpointer_xpointer_match(const char *xpointer, int len, struct xistate *first) {
    const char *c;
    int s;
    dbg(lvl_info,"%s", xpointer);
    if (xpointer[0] != '/')
        return 0;
    c=xpointer+1;
    len--;
    do {
        s=strcspn(c, "/");
        if (s > len)
            s=len;
        if (! xpointer_element_match(c, s, first))
            return 0;
        first=first->child;
        c+=s+1;
        len-=s+1;
    } while (len > 0 && first);
    if (len > 0)
        return 0;
    return 1;
}

static int xpointer_match(const char *xpointer, struct xistate *first) {
    char *prefix="xpointer(";
    int len;
    if (! xpointer)
        return 1;
    len=strlen(xpointer);
    if (strncmp(xpointer,prefix,strlen(prefix)))
        return 0;
    if (xpointer[len-1] != ')')
        return 0;
    return xpointer_xpointer_match(xpointer+strlen(prefix), len-strlen(prefix)-1, first);

}

static void xi_start_element(xml_context *context,
                             const gchar         *element_name,
                             const gchar        **attribute_names,
                             const gchar        **attribute_values,
                             gpointer             user_data,
                             xmlerror             **error) {
    struct xmldocument *doc=user_data;
    struct xistate *xistate;
    int i,count=0;
    while (attribute_names[count++*XML_ATTR_DISTANCE]);
    xistate=g_new0(struct xistate, 1);
    xistate->element=element_name;
    xistate->attribute_names=g_new0(const char *, count);
    xistate->attribute_values=g_new0(const char *, count);
    for (i = 0 ; i < count ; i++) {
        if (attribute_names[i*XML_ATTR_DISTANCE] && attribute_values[i*XML_ATTR_DISTANCE]) {
            xistate->attribute_names[i]=g_strdup(attribute_names[i*XML_ATTR_DISTANCE]);
            xistate->attribute_values[i]=g_strdup(attribute_values[i*XML_ATTR_DISTANCE]);
        }
    }
    xistate->parent=doc->last;

    if (doc->last) {
        doc->last->child=xistate;
    } else
        doc->first=xistate;
    doc->last=xistate;
    if (doc->active > 0 || xpointer_match(doc->xpointer, doc->first)) {
        if(!g_ascii_strcasecmp("xi:include", element_name)) {
            xinclude(context, xistate->attribute_names, xistate->attribute_values, doc, error);
            return;
        }
        start_element(context, element_name, xistate->attribute_names, xistate->attribute_values, doc->user_data, error);
        doc->active++;
    }

}
/**
 * * Reached closing tag of a config element
 * *
 * * @param context
 * * @param element name
 * * @param user_data ptr to xmldocument
 * * @param error ptr to struct for error information
 * * @returns nothing
 * */

static void xi_end_element (xml_context *context,
                            const gchar         *element_name,
                            gpointer             user_data,
                            xmlerror             **error) {
    struct xmldocument *doc=user_data;
    struct xistate *xistate=doc->last;
    int i=0;
    doc->last=doc->last->parent;
    if (! doc->last)
        doc->first=NULL;
    else
        doc->last->child=NULL;
    if (doc->active > 0) {
        if(!g_ascii_strcasecmp("xi:include", element_name)) {
            return;
        }
        end_element(context, element_name, doc->user_data, error);
        doc->active--;
    }
    while (xistate->attribute_names[i]) {
        g_free((char *)(xistate->attribute_names[i]));
        g_free((char *)(xistate->attribute_values[i]));
        i++;
    }
    g_free(xistate->attribute_names);
    g_free(xistate->attribute_values);
    g_free(xistate);
}

/* Called for character data */
/* text is not nul-terminated */
static void xi_text (xml_context *context,
                     const gchar            *text,
                     gsize                   text_len,
                     gpointer                user_data,
                     xmlerror               **error) {
    struct xmldocument *doc=user_data;
    int i;
    if (doc->active) {
        for (i = 0 ; i < text_len ; i++) {
            if (!isspace(text[i])) {
                struct xmldocument *doc=user_data;
                struct xmlstate *curr, **state = doc->user_data;
                struct attr attr;

                curr=*state;
                char *text_dup = g_strndup(text, text_len);
                attr.type=attr_xml_text;
                attr.u.str=text_dup;
                if (curr->object_func && curr->object_func->add_attr && curr->element_attr.u.data)
                    curr->object_func->add_attr(curr->element_attr.u.data, &attr);
                g_free(text_dup);
                return;
            }
        }
    }
}

#if USE_EZXML
static void parse_node_text(ezxml_t node, void *data, void (*start)(void *, const char *, const char **, const char **,
                            void *,
                            void *),
                            void (*end)(void *, const char *, void *, void *),
                            void (*text)(void *, const char *, int, void *, void *)) {
    while (node) {
        if (start)
            start(NULL, node->name, (const char **)node->attr, (const char **)(node->attr+1), data, NULL);
        if (text && node->txt)
            text(NULL, node->txt, strlen(node->txt), data, NULL);
        if (node->child)
            parse_node_text(node->child, data, start, end, text);
        if (end)
            end(NULL, node->name, data, NULL);
        node=node->ordered;
    }
}
#endif

void xml_parse_text(const char *document, void *data,
                    void (*start)(xml_context *, const char *, const char **, const char **, void *, GError **),
                    void (*end)(xml_context *, const char *, void *, GError **),
                    void (*text)(xml_context *, const char *, gsize, void *, GError **)) {
#if !USE_EZXML
    GMarkupParser parser = { start, end, text, NULL, NULL};
    xml_context *context;
    gboolean result;

    context = g_markup_parse_context_new (&parser, 0, data, NULL);
    if (!document) {
        dbg(lvl_error, "FATAL: No XML data supplied (looks like incorrect configuration for internal GUI).");
        exit(1);
    }
    result = g_markup_parse_context_parse (context, document, strlen(document), NULL);
    if (!result) {
        dbg(lvl_error, "FATAL: Cannot parse data as XML: '%s'", document);
        exit(1);
    }
    g_markup_parse_context_free (context);
#else
    char *str=g_strdup(document);
    ezxml_t root = ezxml_parse_str(str, strlen(str));
    if (!root)
        return;
    parse_node_text(root, data, start, end, text);
    ezxml_free(root);
    g_free(str);
#endif
}


#if !USE_EZXML

static const GMarkupParser parser = {
    xi_start_element,
    xi_end_element,
    xi_text,
    NULL,
    NULL
};
/**
 * * Parse the contents of the configuration file
 * *
 * * @param document struct holding info  about the config file
 * * @param error info on any errors detected
 * * @returns boolean TRUE or FALSE
 * */

static gboolean parse_file(struct xmldocument *document, xmlerror **error) {
    xml_context *context;
    gchar *contents, *message;
    gsize len;
    gint line, chr;
    gboolean result;
    char *xmldir,*newxmldir,*xmlfile,*newxmlfile,*sep;

    dbg(lvl_debug,"enter filename='%s'", document->href);
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 12
#define G_MARKUP_TREAT_CDATA_AS_TEXT 0
#endif
    context = g_markup_parse_context_new (&parser, G_MARKUP_TREAT_CDATA_AS_TEXT, document, NULL);

    if (!g_file_get_contents (document->href, &contents, &len, error)) {
        g_markup_parse_context_free (context);
        return FALSE;
    }
    xmldir=getenv("XMLDIR");
    xmlfile=getenv("XMLFILE");
    newxmlfile=g_strdup(document->href);
    newxmldir=g_strdup(document->href);
    if ((sep=strrchr(newxmldir,'/')))
        *sep='\0';
    else {
        g_free(newxmldir);
        newxmldir=g_strdup(".");
    }
    setenv("XMLDIR",newxmldir,1);
    setenv("XMLFILE",newxmlfile,1);
    document->active=document->xpointer ? 0:1;
    document->first=NULL;
    document->last=NULL;
    result = g_markup_parse_context_parse (context, contents, len, error);
    if (!result && error && *error) {
        g_markup_parse_context_get_position(context, &line, &chr);
        message=g_strdup_printf("%s at line %d, char %d\n", (*error)->message, line, chr);
        g_free((*error)->message);
        (*error)->message=message;
    }
    g_markup_parse_context_free (context);
    g_free (contents);
    if (xmldir)
        setenv("XMLDIR",xmldir,1);
    else
        unsetenv("XMLDIR");
    if (xmlfile)
        setenv("XMLFILE",xmlfile,1);
    else
        unsetenv("XMLFILE");
    g_free(newxmldir);
    g_free(newxmlfile);
    dbg(lvl_debug,"return %d", result);

    return result;
}
#else
static void parse_node(struct xmldocument *document, ezxml_t node) {
    while (node) {
        xi_start_element(NULL,node->name, node->attr, node->attr+1, document, NULL);
        if (node->txt)
            xi_text(NULL,node->txt,strlen(node->txt),document,NULL);
        if (node->child)
            parse_node(document, node->child);
        xi_end_element (NULL,node->name,document,NULL);
        node=node->ordered;
    }
}

static gboolean parse_file(struct xmldocument *document, xmlerror **error) {
    FILE *f;
    ezxml_t root;

    f=fopen(document->href,"rb");
    if (!f)
        return FALSE;
    root = ezxml_parse_fp(f);
    fclose(f);
    if (!root)
        return FALSE;
    document->active=document->xpointer ? 0:1;
    document->first=NULL;
    document->last=NULL;

    parse_node(document, root);

    return TRUE;
}
#endif

/**
 * * Load and parse the master config file
 * *
 * * @param filename FQFN of the file
 * * @param error ptr to error details, if any
 * * @returns boolean TRUE or FALSE (if error detected)
 * */

gboolean config_load(const char *filename, xmlerror **error) {
    struct xmldocument document;
    struct xmlstate *curr=NULL;
    gboolean result;

    attr_create_hash();
    item_create_hash();
    initStatic();

    dbg(lvl_debug,"enter filename='%s'", filename);
    memset(&document, 0, sizeof(document));
    document.href=filename;
    document.user_data=&curr;
    result=parse_file(&document, error);
    if (result && curr) {
        g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_PARSE, "element '%s' not closed", curr->element);
        result=FALSE;
    }
    attr_destroy_hash();
    item_destroy_hash();
    dbg(lvl_debug,"return %d", result);
    return result;
}

int navit_object_set_methods(void *in, int in_size, void *out, int out_size) {
    int ret,size=out_size;
    if (out_size > in_size) {
        ret=-1;
        size=in_size;
        memset((char *)out+in_size, 0, out_size-in_size);
    } else if (in_size == out_size)
        ret=0;
    else
        ret=1;
    memcpy(out, in, size);
    return ret;
}

struct navit_object *
navit_object_new(struct attr **attrs, struct object_func *func, int size) {
    struct navit_object *ret=g_malloc0(size);
    ret->func=func;
    ret->attrs=attr_list_dup(attrs);
    navit_object_ref(ret);
    return ret;
}

struct navit_object *
navit_object_ref(struct navit_object *obj) {
    obj->refcount++;
    dbg(lvl_debug,"refcount %s %p %d",attr_to_name(obj->func->type),obj,obj->refcount);
    return obj;
}

void navit_object_unref(struct navit_object *obj) {
    if (obj) {
        obj->refcount--;
        dbg(lvl_debug,"refcount %s %p %d",attr_to_name(obj->func->type),obj,obj->refcount);
        if (obj->refcount <= 0 && obj->func && obj->func->destroy)
            obj->func->destroy(obj);
    }
}

struct attr_iter {
    void *last;
};

struct attr_iter *
navit_object_attr_iter_new(void) {
    return g_new0(struct attr_iter, 1);
}

void navit_object_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}

/**
 * @brief Generic get function
 *
 * This function searches an attribute list for an attribute of a given type and stores it in the {@code attr}
 * parameter. Internally it calls
 * {@link attr_generic_get_attr(struct attr **, struct attr **, enum attr_type, struct attr *, struct attr_iter *)}
 * to retrieve the attribute; see its documentation for details.
 * <p>
 * Searching for attr_any or attr_any_xml is supported.
 * <p>
 * An iterator can be specified to get multiple attributes of the same type:
 * The first call will return the first match; each subsequent call
 * with the same iterator will return the next match. If no more matching
 * attributes are found in either of them, false is returned.
 *
 * @param obj The object to return an attribute for. This can be any Navit object type, but the parameter should
 * be cast to {@code struct navit_object *} to avoid compiler warnings.
 * @param type The attribute type to search for. Searching for {@code attr_any} or {@code attr_any_xml} is
 * possible.
 * @param attr Points to a {@code struct attr} which will receive the attribute
 * @param iter An iterator to receive multiple attributes of the same type with subsequent calls. If {@code NULL},
 * the first matching attribute will be retrieved.
 *
 * @return True if a matching attribute was found, false if not.
 */
int navit_object_get_attr(struct navit_object *obj, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(obj->attrs, NULL, type, attr, iter);
}

void navit_object_callbacks(struct navit_object *obj, struct attr *attr) {
    if (obj->attrs && obj->attrs[0] && obj->attrs[0]->type == attr_callback_list)
        callback_list_call_attr_2(obj->attrs[0]->u.callback_list, attr->type, attr->u.data, 0);
}

int navit_object_set_attr(struct navit_object *obj, struct attr *attr) {
    dbg(lvl_debug, "enter, obj=%p, attr=%p (%s)", obj, attr, attr_to_name(attr->type));
    obj->attrs=attr_generic_set_attr(obj->attrs, attr);
    navit_object_callbacks(obj, attr);
    return 1;
}

int navit_object_add_attr(struct navit_object *obj, struct attr *attr) {
    dbg(lvl_debug, "enter, obj=%p, attr=%p (%s)", obj, attr, attr_to_name(attr->type));
    if (attr->type == attr_callback) {
        struct callback_list *cbl;
        if (obj->attrs && obj->attrs[0] && obj->attrs[0]->type == attr_callback_list)
            cbl=obj->attrs[0]->u.callback_list;
        else {
            struct attr attr;
            cbl=callback_list_new();
            attr.type=attr_callback_list;
            attr.u.callback_list=cbl;
            obj->attrs=attr_generic_prepend_attr(obj->attrs, &attr);
        }
        callback_list_add(cbl, attr->u.callback);
        return 1;
    }
    obj->attrs=attr_generic_add_attr(obj->attrs, attr);
    if (obj->attrs && obj->attrs[0] && obj->attrs[0]->type == attr_callback_list)
        callback_list_call_attr_2(obj->attrs[0]->u.callback_list, attr->type, attr->u.data, 1);
    return 1;
}

int navit_object_remove_attr(struct navit_object *obj, struct attr *attr) {
    if (attr->type == attr_callback) {
        if (obj->attrs && obj->attrs[0] && obj->attrs[0]->type == attr_callback_list) {
            callback_list_remove(obj->attrs[0]->u.callback_list, attr->u.callback);
            return 1;
        } else
            return 0;
    }
    obj->attrs=attr_generic_remove_attr(obj->attrs, attr);
    if (obj->attrs && obj->attrs[0] && obj->attrs[0]->type == attr_callback_list)
        callback_list_call_attr_2(obj->attrs[0]->u.callback_list, attr->type, attr->u.data, -1);
    return 1;
}

void navit_object_destroy(struct navit_object *obj) {
    attr_list_free(obj->attrs);
    g_free(obj);
}
