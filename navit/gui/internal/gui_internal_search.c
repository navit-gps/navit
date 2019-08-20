#include <glib.h>
#include <stdlib.h>
#include "config.h"
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "navit.h"
#include "navit_nls.h"
#include "event.h"
#include "search.h"
#include "country.h"
#include "track.h"
#include "linguistics.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_html.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"
#include "gui_internal_search.h"
#ifdef HAVE_API_ANDROID
#include "util.h"
#endif

static void gui_internal_search_country(struct gui_priv *this, struct widget *widget, void *data) {
    gui_internal_prune_menu_count(this, 1, 0);
    gui_internal_search(this,_("Country"),"Country",0);
}

static void gui_internal_search_town(struct gui_priv *this, struct widget *wm, void *data) {
    if (this->sl)
        search_list_select(this->sl, attr_country_all, 0, 0);
    g_free(this->country_iso2);
    this->country_iso2=NULL;
    gui_internal_search(this,_("Town"),"Town",0);
}

static void gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data) {
    search_list_select(this->sl, attr_town_or_district_name, 0, 0);
    gui_internal_search(this,_("Street"),"Street",0);
}

static void gui_internal_search_house_number(struct gui_priv *this, struct widget *widget, void *data) {
    search_list_select(this->sl, attr_street_name, 0, 0);
    gui_internal_search(this,_("House number"),"House number",0);
}

void gui_internal_search_idle_end(struct gui_priv *this) {
    if (this->idle) {
        event_remove_idle(this->idle);
        this->idle=NULL;
    }
    if (this->idle_cb) {
        callback_destroy(this->idle_cb);
        this->idle_cb=NULL;
    }
}

static int gui_internal_search_cmp(gconstpointer _a, gconstpointer _b) {
    struct widget *a=(struct widget *)_a, *b=(struct widget *)_b;
    char *sa,*sb;
    int r;
    if(!b)
        if((!a || a->type!=widget_table_row || !a->text) && (!b || b->type!=widget_table_row || !b->text))
            return 0;
    if(!a || a->type!=widget_table_row || !a->text)
        return -1;
    if(!b || b->type!=widget_table_row || !b->text)
        return 1;
    r=a->datai-b->datai;
    if(r<0)
        return -1;
    if(r>0)
        return 1;
    sa=removecase(a->text);
    sb=removecase(b->text);
    r=strcmp(sa,sb);
    dbg(lvl_debug,"%s %s %d",sa,sb,r);
    g_free(sa);
    g_free(sb);
    return r;
}

static char *postal_str(struct search_list_result *res, int level) {
    char *ret=NULL;
    if (res->town->common.postal)
        ret=res->town->common.postal;
    if (res->town->common.postal_mask)
        ret=res->town->common.postal_mask;
    if (level == 1)
        return ret;
    if (res->street->common.postal)
        ret=res->street->common.postal;
    if (res->street->common.postal_mask)
        ret=res->street->common.postal_mask;
    if (level == 2)
        return ret;
    if (res->house_number->common.postal)
        ret=res->house_number->common.postal;
    if (res->house_number->common.postal_mask)
        ret=res->house_number->common.postal_mask;
    return ret;
}

static char *get_string_from_attr_list(struct attr **attrs, enum attr_type type, char *dflt) {
    struct attr attr;
    if(attr_generic_get_attr(attrs,NULL,type,&attr,NULL))
        return attr.u.str;
    else
        return dflt;
}

static char *district_str(struct search_list_result *res, int level, enum attr_type district, char *dflt) {
    char *ret=dflt;

    ret=get_string_from_attr_list(res->town->common.attrs, district, ret);
    if (level == 1)
        return ret;

    ret=get_string_from_attr_list(res->street->common.attrs, district, ret);

    if (level == 2)
        return ret;

    ret=get_string_from_attr_list(res->house_number->common.attrs, district, ret);

    return ret;
}

static char *town_display_label(struct search_list_result *res, int level, int flags) {
    char *town=district_str(res, level,attr_town_name,"");
    char *district=district_str(res, level,attr_district_name,NULL);
    char *postal=postal_str(res, level);
    char *postal_sep=" ";
    char *district_begin=" (";
    char *district_end=")";
    char *county_sep = ", ";
    char *county = res->town->common.county_name;

    if (!postal)
        postal_sep=postal="";
    if (!district || (flags & 1))
        district_begin=district_end=district="";
    if (!county || !strcmp(county, town))
        county_sep=county="";

    if(level==1 ) {
        if(flags & 2) {
            int n=0;
            char *s[10]= {NULL};

            s[n]=district_str(res, level, attr_state_name, NULL);
            if(s[n])
                n++;
            s[n]=district_str(res, level, attr_county_name, NULL);
            if(s[n])
                n++;
            s[n]=district_str(res, level, attr_municipality_name, NULL);
            if(s[n])
                n++;

            return g_strjoinv(", ",s);
        }
        county=county_sep="";
    }

    return g_strdup_printf("%s%s%s%s%s%s%s%s", postal, postal_sep, town, district_begin, district, district_end, county_sep,
                           county);
}

static void gui_internal_find_next_possible_key(char *search_text, char *wm_name, char *possible_keys,
        char *item_name) {
    gchar* trunk_name;
    if (item_name) {

        trunk_name = linguistics_expand_special(item_name,1);
        if(!trunk_name) {
            trunk_name = g_strrstr(item_name, search_text);
        }

        if (trunk_name) {
            int next_char_pos = strlen(search_text);
            char next_char = trunk_name[next_char_pos];
            int i;
            int len = strlen(possible_keys);

            for(i = 0; (i<len) && (possible_keys[i] != next_char) ; i++) ;

            if ((i==len || !len) && !strncmp(trunk_name, search_text, next_char_pos)) {
                possible_keys[len]=trunk_name[next_char_pos];
                possible_keys[len+1]='\0';
            }
            dbg(lvl_info,"searching for %s, found: %s, currently possible_keys: %s ", search_text, item_name, possible_keys);
        }
    }
}

static void gui_internal_highlight_possible_keys(struct gui_priv *this, char *possible_keys) {
    struct menu_data *md;
    int first_available_key_found = 0;

    if (!this->keyboard)
        return;

    md=gui_internal_menu_data(this);
    if (md && md->keyboard && !(this->flags & 2048)) {
        GList *lk=md->keyboard->children;
        graphics_draw_mode(this->gra, draw_mode_begin);
        while (lk) {
            struct widget *child=lk->data;
            GList *lk2=child->children;
            while (lk2) {
                struct widget *child_=lk2->data;
                lk2=g_list_next(lk2);
                // The data_free part is an evil hack based on the observation that
                // regular keys have set it to non-NULL whereas special keys appear
                // to have it set to NULL.
                if (child_->data && strcmp("\b", child_->data) && child_->data_free) {
                    if ( (strlen(possible_keys) == 0) ||
                            (g_strrstr(possible_keys, child_->data)!=NULL ) ) {
                        if(this->hide_keys) {
                            child_->state|= STATE_SENSITIVE|STATE_CLEAR ;
                            child_->state&= ~(STATE_INVISIBLE);
                        } else {
                            child_->state|= STATE_SENSITIVE|STATE_CLEAR|STATE_HIGHLIGHTED ;
                        }
                        // Select and highlight the first possible button.
                        if (!first_available_key_found) {
                            gui_internal_highlight_do(this, child_);
                            first_available_key_found=1;
                        }
                    } else {
                        if(this->hide_keys) {
                            child_->state&= ~(STATE_SELECTED|STATE_SENSITIVE) ;
                            child_->state|= STATE_INVISIBLE;
                        } else {
                            child_->state&= ~(STATE_HIGHLIGHTED);
                        }
                    }
                    gui_internal_widget_render(this,child_);
                }
            }
            lk=g_list_next(lk);
        }
        gui_internal_widget_render(this,md->keyboard);
        graphics_draw_mode(this->gra, draw_mode_end);
    }

}

static int gui_internal_get_match_quality(char *item_name, char* search_text, int is_house_number_without_street) {
    enum match_quality {
        full_string_match, word_match, substring_match, housenum_but_no_street_match
    }
    match_quality;
    if (is_house_number_without_street) {
        match_quality=housenum_but_no_street_match;
    } else if(item_name) {
        int i;
        char *folded_name=linguistics_casefold(item_name);
        char *folded_query=linguistics_casefold(search_text);
        match_quality=substring_match;

        for(i=0; i<3 ; i++) {
            char *exp=linguistics_expand_special(folded_name,i);
            char *p;
            if(!exp)
                continue;
            if(!strcmp(exp,folded_query)) {
                dbg(lvl_debug,"exact match for the whole string %s", exp);
                match_quality=full_string_match;
                g_free(exp);
                break;
            }
            if((p=strstr(exp,folded_query))!=NULL) {
                p+=strlen(folded_query);
                if(!*p||strchr(LINGUISTICS_WORD_SEPARATORS_ASCII,*p)) {
                    dbg(lvl_debug,"exact matching word found inside string %s",exp);
                    match_quality=word_match;
                }
            }
            g_free(exp);
        }
        g_free(folded_name);
        g_free(folded_query);
    }
    return match_quality;
}

static struct widget* gui_internal_create_resultlist_entry(struct gui_priv *this, struct search_list_result *res,
        char *result_main_label,
        char *result_sublabel, void *param, char *widget_name, struct item *item) {
    struct widget *resultlist_entry;
    if(result_sublabel) {
        struct widget *entry_sublabel;
        resultlist_entry=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
        gui_internal_widget_append(resultlist_entry,
                                   gui_internal_image_new(this, image_new_xs(this, res->country->flag)));
        entry_sublabel=gui_internal_box_new(this, gravity_left_center|orientation_vertical|flags_fill);
        gui_internal_widget_append(resultlist_entry, entry_sublabel);
        gui_internal_widget_append(entry_sublabel, gui_internal_label_new(this, result_main_label));
        gui_internal_widget_append(entry_sublabel, gui_internal_label_font_new(this, result_sublabel, 1));
        resultlist_entry->func=gui_internal_cmd_position;
        resultlist_entry->data=param;
        resultlist_entry->state |= STATE_SENSITIVE;
        resultlist_entry->speech=g_strdup(result_main_label);
    } else {
        resultlist_entry=gui_internal_button_new_with_callback(this, result_main_label,
                         image_new_xs(this, res->country->flag),
                         gravity_left_center|orientation_horizontal|flags_fill,
                         gui_internal_cmd_position, param);
    }
    resultlist_entry->name=widget_name;
    if (res->c)
        resultlist_entry->c=*res->c;
    resultlist_entry->selection_id=res->id;
    if (item)
        resultlist_entry->item=*item;

    return resultlist_entry;
}

/**
 * @brief List of possible next keys/characters given the current result list of the incremental search.
 */
char possible_keys_incremental_search[256]="";

static void gui_internal_search_idle(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param) {
    char *result_main_label=NULL,*result_sublabel=NULL,*item_name=NULL, *widget_name=NULL, *search_text;
    struct search_list_result *res;
    struct item *item=NULL;
    struct widget *search_input=NULL;
    struct widget *menu, *resultlist_row, *resultlist_entry;

    res=search_list_get_result(this->sl);
    if (!res) {
        gui_internal_search_idle_end(this);
        gui_internal_highlight_possible_keys(this, possible_keys_incremental_search);
        return;
    }

    if (! strcmp(wm_name,"Country")) {
        item_name=res->country->name;
        item=&res->country->common.item;
        result_main_label=g_strdup_printf("%s", res->country->name);
    } else if (! strcmp(wm_name,"Town")) {
        item=&res->town->common.item;
        item_name=res->town->common.town_name;
        result_main_label=town_display_label(res, 1, 0);
        result_sublabel=town_display_label(res, 1, 2);
    } else if (! strcmp(wm_name,"Street")) {
        item_name=res->street->name;
        item=&res->street->common.item;
        result_main_label=g_strdup(res->street->name);
        result_sublabel=town_display_label(res, 2, 1);
    } else if (! strcmp(wm_name,"House number")) {
        item_name=res->house_number->house_number;
        result_main_label=g_strdup_printf("%s, %s", item_name, res->street->name);
        result_sublabel=town_display_label(res, 3, 0);
        widget_name=g_strdup(result_main_label);
    }
    if(!item_name) {
        dbg(lvl_error, "Skipping nameless item in search (search type: %s). Please report this as a bug.", wm_name);
        return;
    }

    if(!widget_name)
        widget_name=g_strdup(item_name);

    menu=g_list_last(this->root.children)->data;
    search_input=gui_internal_find_widget(menu, NULL, STATE_EDIT);
    dbg_assert(search_input);
    search_text=search_input->text;

    gui_internal_find_next_possible_key(search_text, wm_name, possible_keys_incremental_search, item_name);

    resultlist_row=gui_internal_widget_table_row_new(this, gravity_left|orientation_horizontal|flags_fill);
    if (!result_sublabel)
        resultlist_row->text=g_strdup(result_main_label);
    else
        resultlist_row->text=g_strdup_printf("%s %s",item_name,result_sublabel);
    int is_house_number_without_street=!strcmp(wm_name,"House number") && !res->street->name;
    resultlist_row->datai=gui_internal_get_match_quality(item_name, search_text, is_house_number_without_street);
    gui_internal_widget_insert_sorted(search_list, resultlist_row, gui_internal_search_cmp);

    resultlist_entry=gui_internal_create_resultlist_entry(
                         this, res, result_main_label, result_sublabel, param, widget_name, item);
    gui_internal_widget_append(resultlist_row, resultlist_entry);
    gui_internal_widget_pack(this, search_list);
    graphics_draw_mode(this->gra, draw_mode_begin);
    gui_internal_widget_render(this, menu);
    graphics_draw_mode(this->gra, draw_mode_end);

    g_free(result_main_label);
    g_free(result_sublabel);
}

static void gui_internal_search_idle_start(struct gui_priv *this, char *wm_name, struct widget *search_list,
        void *param) {
    this->idle_cb=callback_new_4(callback_cast(gui_internal_search_idle), this, wm_name, search_list, param);
    this->idle=event_add_idle(50,this->idle_cb);
    callback_call_0(this->idle_cb);
}

static void gui_internal_search_changed(struct gui_priv *this, struct widget *wm, void *data) {
    GList *l;
    struct widget *search_list=gui_internal_menu_data(this)->search_list;
    void *param=(void *)3;

    gui_internal_widget_table_clear(this, search_list);
    possible_keys_incremental_search[0]='\0';

    if (! strcmp(wm->name,"Country"))
        param=(void *)4;
    if (! strcmp(wm->name,"Street"))
        param=(void *)5;
    if (! strcmp(wm->name,"House number"))
        param=(void *)6;
    dbg(lvl_debug,"%s now '%s'", wm->name, wm->text);

    gui_internal_search_idle_end(this);
    if (wm->text && g_utf8_strlen(wm->text, -1) > 0) {
        struct attr search_attr;

        dbg(lvl_debug,"process");
        if (! strcmp(wm->name,"Country"))
            search_attr.type=attr_country_all;
        if (! strcmp(wm->name,"Town"))
            search_attr.type=attr_town_postal; /*attr_town_or_district_name to exclude zip code*/
        if (! strcmp(wm->name,"Street"))
            search_attr.type=attr_street_name;
        if (! strcmp(wm->name,"House number"))
            search_attr.type=attr_house_number;
        search_attr.u.str=wm->text;
        search_list_search(this->sl, &search_attr, 1);
        gui_internal_search_idle_start(this, wm->name, search_list, param);
    } else {
        // If not enough content is entered, we highlight all keys.
        gui_internal_highlight_possible_keys(this, "");
    }
    l=g_list_last(this->root.children);
    gui_internal_widget_render(this, l->data);
}

static void gui_internal_search_list_set_default_country(struct gui_priv *this) {
    struct attr search_attr, country_name, country_iso2, *country_attr;
    struct item *item;
    struct country_search *cs;
    struct tracking *tracking;
    struct search_list_result *res;

    country_attr=country_default();
    tracking=navit_get_tracking(this->nav);
    if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL))
        country_attr=&search_attr;
    if (country_attr) {
        cs=country_search_new(country_attr, 0);
        item=country_search_get_item(cs);
        if (item && item_attr_get(item, attr_country_name, &country_name)) {
            search_attr.type=attr_country_all;
            dbg(lvl_debug,"country %s", country_name.u.str);
            search_attr.u.str=country_name.u.str;
            search_list_search(this->sl, &search_attr, 0);
            while((res=search_list_get_result(this->sl)));
            if(this->country_iso2) {
                g_free(this->country_iso2);
                this->country_iso2=NULL;
            }
            if (item_attr_get(item, attr_country_iso2, &country_iso2))
                this->country_iso2=g_strdup(country_iso2.u.str);
        }
        country_search_destroy(cs);
    } else {
        dbg(lvl_error,"warning: no default country found");
        if (this->country_iso2) {
            dbg(lvl_debug,"attempting to use country '%s'",this->country_iso2);
            search_attr.type=attr_country_iso2;
            search_attr.u.str=this->country_iso2;
            search_list_search(this->sl, &search_attr, 0);
            while((res=search_list_get_result(this->sl)));
        }
    }
}

static void gui_internal_search_list_new(struct gui_priv *this) {
    struct mapset *ms=navit_get_mapset(this->nav);
    if (! this->sl) {
        this->sl=search_list_new(ms);
        gui_internal_search_list_set_default_country(this);
    }
}

void gui_internal_search_list_destroy(struct gui_priv *this) {
    if (this->sl) {
        search_list_destroy(this->sl);
        this->sl=NULL;
    }
}

void gui_internal_search(struct gui_priv *this, const char *what, const char *type, int flags) {
    struct widget *wb,*wk,*w,*wr,*we,*wl,*wnext=NULL;
    char *country;
    int keyboard_mode;
    gui_internal_search_list_new(this);
    keyboard_mode = VKBD_FLAG_2 | gui_internal_keyboard_init_mode(this->country_iso2 ? this->country_iso2 : getenv("LANG"));
    wb=gui_internal_menu(this, what);
    w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wb, w);
    wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(w, wr);
    we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
    gui_internal_widget_append(wr, we);
    if (!strcmp(type,"Country")) {
        wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_town"));
        wnext->func=gui_internal_search_town;
    } else if (!strcmp(type,"Town")) {
        if (this->country_iso2) {
#ifdef HAVE_API_ANDROID
            char country_iso2[strlen(this->country_iso2)+1];
            strtolower(country_iso2, this->country_iso2);
            country=g_strdup_printf("country_%s", country_iso2);
#else
            country=g_strdup_printf("country_%s", this->country_iso2);
#endif
        } else
            country=g_strdup("gui_select_country");
        gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, country)));
        wb->state |= STATE_SENSITIVE;
        if (flags)
            wb->func = gui_internal_search_country;
        else
            wb->func = gui_internal_back;
        wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_street"));
        wnext->func=gui_internal_search_street;
        g_free(country);
    } else if (!strcmp(type,"Street")) {
        gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_select_town")));
        wb->state |= STATE_SENSITIVE;
        wb->func = gui_internal_back;
        wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_house_number"));
        wnext->func=gui_internal_search_house_number;
    } else if (!strcmp(type,"House number")) {
        gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_select_street")));
        wb->state |= STATE_SENSITIVE;
        wb->func = gui_internal_back;
        keyboard_mode = VKBD_NUMERIC | VKBD_FLAG_2;
    }
    gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
    if (wnext) {
        gui_internal_widget_append(we, wnext);
        wnext->state |= STATE_SENSITIVE;
    }
    wl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,
                                     1);//gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(wr, wl);
    gui_internal_menu_data(this)->search_list=wl;
    wk->state |= STATE_EDIT|STATE_EDITABLE;
    wk->background=this->background;
    wk->flags |= flags_expand|flags_fill;
    wk->func = gui_internal_search_changed;
    wk->name=g_strdup(type);
    if (this->keyboard)
        gui_internal_widget_append(w, gui_internal_keyboard(this, keyboard_mode));
    else
        gui_internal_keyboard_show_native(this, w, keyboard_mode, getenv("LANG"));
    gui_internal_menu_render(this);
}

void gui_internal_search_house_number_in_street(struct gui_priv *this, struct widget *widget, void *data) {
    dbg(lvl_info,"id %d", widget->selection_id);
    search_list_select(this->sl, attr_street_name, 0, 0);
    search_list_select(this->sl, attr_street_name, widget->selection_id, 1);
    gui_internal_search(this,_("House number"),"House number",0);
}

void gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data) {
    dbg(lvl_info,"id %d", widget->selection_id);
    search_list_select(this->sl, attr_town_or_district_name, 0, 0);
    search_list_select(this->sl, attr_town_or_district_name, widget->selection_id, 1);
    gui_internal_search(this,_("Street"),"Street",0);
}

void gui_internal_search_town_in_country(struct gui_priv *this, struct widget *widget) {
    struct search_list_common *slc;
    dbg(lvl_info,"id %d", widget->selection_id);
    search_list_select(this->sl, attr_country_all, 0, 0);
    slc=search_list_select(this->sl, attr_country_all, widget->selection_id, 1);
    if (slc) {
        g_free(this->country_iso2);
        this->country_iso2=g_strdup(((struct search_list_country *)slc)->iso2);
    }
    gui_internal_search(this,widget->name,"Town",0);
}

