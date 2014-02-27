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

static void
gui_internal_search_country(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_prune_menu_count(this, 1, 0);
	gui_internal_search(this,_("Country"),"Country",0);
}

static void
gui_internal_search_town(struct gui_priv *this, struct widget *wm, void *data)
{
	if (this->sl)
		search_list_select(this->sl, attr_country_all, 0, 0);
	g_free(this->country_iso2);
	this->country_iso2=NULL;
	gui_internal_search(this,_("Town"),"Town",0);
}

static void
gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data)
{
	search_list_select(this->sl, attr_town_or_district_name, 0, 0);
	gui_internal_search(this,_("Street"),"Street",0);
}

static void
gui_internal_search_house_number(struct gui_priv *this, struct widget *widget, void *data)
{
	search_list_select(this->sl, attr_street_name, 0, 0);
	gui_internal_search(this,_("House number"),"House number",0);
}

void
gui_internal_search_idle_end(struct gui_priv *this)
{
	if (this->idle) {
		event_remove_idle(this->idle);
		this->idle=NULL;
	}
	if (this->idle_cb) {
		callback_destroy(this->idle_cb);
		this->idle_cb=NULL;
	}
}

static int
gui_internal_search_cmp(gconstpointer _a, gconstpointer _b)
{
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
  dbg(1,"%s %s %d\n",sa,sb,r);
  g_free(sa);
  g_free(sb);
  return r;
}

static char *
postal_str(struct search_list_result *res, int level)
{
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

static char *
get_string_from_attr_list(struct attr **attrs, enum attr_type type, char *dflt)
{
	struct attr attr;
	if(attr_generic_get_attr(attrs,NULL,type,&attr,NULL))
		return attr.u.str;
	else
		return dflt;
}

static char *
district_str(struct search_list_result *res, int level, enum attr_type district, char *dflt)
{
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

static char *
town_str(struct search_list_result *res, int level, int flags)
{
	char *town=district_str(res, level,attr_town_name,"");
	char *district=district_str(res, level,attr_district_name,NULL);
	char *postal=postal_str(res, level);
	char *postal_sep=" ";
	char *district_begin=" (";
	char *district_end=")";
	char *county_sep = ", Co. ";
	char *county = res->town->common.county_name;
	
	if (!postal)
		postal_sep=postal="";
	if (!district || (flags & 1))
		district_begin=district_end=district="";
	if (!county)
		county_sep=county="";

	if(level==1 ) {
		if(flags & 2) {
			int n=0;
			char *s[10]={NULL};
		
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

	return g_strdup_printf("%s%s%s%s%s%s%s%s", postal, postal_sep, town, district_begin, district, district_end, county_sep, county);
}

static void
gui_internal_search_idle(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param)
{
	char *text=NULL,*text2=NULL,*name=NULL, *wcname=NULL;
	struct search_list_result *res;
	struct widget *wc;
	struct item *item=NULL;
	GList *l;
	static char possible_keys[256]="";
        struct widget *wi=NULL;
	struct widget *wr, *wb;

	res=search_list_get_result(this->sl);
	if (res) {
		gchar* trunk_name = NULL;

		struct widget *menu=g_list_last(this->root.children)->data;
		wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);

		if (wi) {
			if (! strcmp(wm_name,"Town"))
				trunk_name = g_strrstr(res->town->common.town_name, wi->text);
			if (! strcmp(wm_name,"Street"))
			{
				name=res->street->name;
				if (name)
					trunk_name = g_strrstr(name, wi->text);
				else
					trunk_name = NULL;
			}

			if (trunk_name) {
				char next_char = trunk_name[strlen(wi->text)];
				int i;
				int len = strlen(possible_keys);
				for(i = 0; (i<len) && (possible_keys[i] != next_char) ;i++) ;
				if (i==len || !len) {
					possible_keys[len]=trunk_name[strlen(wi->text)];
					possible_keys[len+1]='\0';
				}
				dbg(1,"%s %s possible_keys:%s \n", wi->text, res->town->common.town_name, possible_keys);
			}
		} else {
			dbg(0, "Unable to find widget");
		}
	}

	if (! res) {
		struct menu_data *md;
		gui_internal_search_idle_end(this);

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
					if (child_->data && strcmp("\b", child_->data)) { // FIXME don't disable special keys
						if (strlen(possible_keys) == 0)
							child_->state|= STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SENSITIVE|STATE_CLEAR ;
						else if (g_strrstr(possible_keys, child_->data)!=NULL ) {
							child_->state|= STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SENSITIVE|STATE_CLEAR ;
						} else {
							child_->state&= ~(STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SELECTED) ;
						}
						gui_internal_widget_render(this,child_);
					}
				}
				lk=g_list_next(lk);
			}
			gui_internal_widget_render(this,md->keyboard);
			graphics_draw_mode(this->gra, draw_mode_end);
		}

		possible_keys[0]='\0';
		return;
	}

	if (! strcmp(wm_name,"Country")) {
		name=res->country->name;
		item=&res->country->common.item;
		text=g_strdup_printf("%s", res->country->name);
	}
	if (! strcmp(wm_name,"Town")) {
		item=&res->town->common.item;
		name=res->town->common.town_name;
		text=town_str(res, 1, 0);
		text2=town_str(res, 1, 2);
	}
	if (! strcmp(wm_name,"Street")) {
		name=res->street->name;
		item=&res->street->common.item;
		text=g_strdup(res->street->name);
		text2=town_str(res, 2, 1);
	}
	if (! strcmp(wm_name,"House number")) {
		name=res->house_number->house_number;
		text=g_strdup_printf("%s, %s", name, res->street->name);
		text2=town_str(res, 3, 0);
		wcname=g_strdup(text);
	}

	if(!wcname)
		wcname=g_strdup(name);

	dbg(1,"res->country->flag=%s\n", res->country->flag);
	wr=gui_internal_widget_table_row_new(this, gravity_left|orientation_horizontal|flags_fill);

	if (!text2)
		wr->text=g_strdup(text);
	else
		wr->text=g_strdup_printf("%s %s",name,text2);
	
	if (! strcmp(wm_name,"House number") && !res->street->name) {
		wr->datai=2048;
	} else if(name) {
		int i;
		char *folded_name=linguistics_casefold(name);
		char *folded_query=linguistics_casefold(wi->text);
		wr->datai=1024;

		for(i=0;wi && i<3 ;i++) {
			char *exp=linguistics_expand_special(folded_name,i);
			char *p;
			if(!exp)
				continue;
			if(!strcmp(exp,folded_query)) {
				dbg(1,"exact match for the whole string %s\n", exp);
				wr->datai=0;
				g_free(exp);
				break;
			} 
			if((p=strstr(exp,folded_query))!=NULL) {
				p+=strlen(folded_query);
				if(!*p||strchr(LINGUISTICS_WORD_SEPARATORS_ASCII,*p)) {
					dbg(1,"exact matching word found inside string %s\n",exp);
					wr->datai=512;
				}
			}
			g_free(exp);
		}
		g_free(folded_name);
		g_free(folded_query);
	}
	gui_internal_widget_insert_sorted(search_list, wr, gui_internal_search_cmp);
	if(text2) {
		wc=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
		gui_internal_widget_append(wr, wc);
			gui_internal_widget_append(wc, gui_internal_image_new(this, image_new_xs(this, res->country->flag)));
		wb=gui_internal_box_new(this, gravity_left_center|orientation_vertical|flags_fill);
		gui_internal_widget_append(wc, wb);
		gui_internal_widget_append(wb, gui_internal_label_new(this, text));
		gui_internal_widget_append(wb, gui_internal_label_font_new(this, text2, 1));
		wc->func=gui_internal_cmd_position;
		wc->data=param;
		wc->state |= STATE_SENSITIVE;
		wc->speech=g_strdup(text);
	} else {
		gui_internal_widget_append(wr,
			wc=gui_internal_button_new_with_callback(this, text,
				image_new_xs(this, res->country->flag),
				gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_position, param));
	}
	wc->name=wcname;
	if (res->c)
		wc->c=*res->c;
	wc->selection_id=res->id;
	if (item)
		wc->item=*item;
	gui_internal_widget_pack(this, search_list);
	l=g_list_last(this->root.children);
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, l->data);
	graphics_draw_mode(this->gra, draw_mode_end);
	g_free(text);
	g_free(text2);
}

static void
gui_internal_search_idle_start(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param)
{
	this->idle_cb=callback_new_4(callback_cast(gui_internal_search_idle), this, wm_name, search_list, param);
	this->idle=event_add_idle(50,this->idle_cb);
	callback_call_0(this->idle_cb);
}

static void
gui_internal_search_changed(struct gui_priv *this, struct widget *wm, void *data)
{
	GList *l;
	struct widget *search_list=gui_internal_menu_data(this)->search_list;
	void *param=(void *)3;
	int minlen=1;

	gui_internal_widget_table_clear(this, search_list);

	if (! strcmp(wm->name,"Country"))
		param=(void *)4;
	if (! strcmp(wm->name,"Street"))
		param=(void *)5;
	if (! strcmp(wm->name,"House number"))
		param=(void *)6;
	dbg(1,"%s now '%s'\n", wm->name, wm->text);

	gui_internal_search_idle_end(this);
	if (wm->text && g_utf8_strlen(wm->text, -1) >= minlen) {
		struct attr search_attr;

		dbg(1,"process\n");
		if (! strcmp(wm->name,"Country"))
			search_attr.type=attr_country_all;
		if (! strcmp(wm->name,"Town"))
			search_attr.type=attr_town_or_district_name;
		if (! strcmp(wm->name,"Street"))
			search_attr.type=attr_street_name;
		if (! strcmp(wm->name,"House number"))
			search_attr.type=attr_house_number;
		search_attr.u.str=wm->text;
		search_list_search(this->sl, &search_attr, 1);
		gui_internal_search_idle_start(this, wm->name, search_list, param);
	}
	l=g_list_last(this->root.children);
	gui_internal_widget_render(this, l->data);
}

static void
gui_internal_search_list_set_default_country(struct gui_priv *this)
{
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
			dbg(1,"country %s\n", country_name.u.str);
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
		dbg(0,"warning: no default country found\n");
		if (this->country_iso2) {
		    dbg(0,"attempting to use country '%s'\n",this->country_iso2);
		    search_attr.type=attr_country_iso2;
		    search_attr.u.str=this->country_iso2;
            search_list_search(this->sl, &search_attr, 0);
            while((res=search_list_get_result(this->sl)));
		}
	}
}

static void
gui_internal_search_list_new(struct gui_priv *this)
{
	struct mapset *ms=navit_get_mapset(this->nav);
	if (! this->sl) {
		this->sl=search_list_new(ms);
		gui_internal_search_list_set_default_country(this);
	}
}

void
gui_internal_search_list_destroy(struct gui_priv *this)
{
	if (this->sl) {
		search_list_destroy(this->sl);
		this->sl=NULL;
	}
}

void
gui_internal_search(struct gui_priv *this, const char *what, const char *type, int flags)
{
	struct widget *wb,*wk,*w,*wr,*we,*wl,*wnext=NULL;
	char *country;
	int keyboard_mode;
	gui_internal_search_list_new(this);
	keyboard_mode=2+gui_internal_keyboard_init_mode(this->country_iso2?this->country_iso2:getenv("LANG"));
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
		keyboard_mode=18;
	}
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
	if (wnext) {
		gui_internal_widget_append(we, wnext);
		wnext->state |= STATE_SENSITIVE;
	}
	wl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);//gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wr, wl);
	gui_internal_menu_data(this)->search_list=wl;
	wk->state |= STATE_EDIT|STATE_EDITABLE;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_search_changed;
	wk->name=g_strdup(type);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,keyboard_mode));
	gui_internal_menu_render(this);
}

void
gui_internal_search_house_number_in_street(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(2,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_street_name, 0, 0);
	search_list_select(this->sl, attr_street_name, widget->selection_id, 1);
	gui_internal_search(this,_("House number"),"House number",0);
}

void
gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(2,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_town_or_district_name, 0, 0);
	search_list_select(this->sl, attr_town_or_district_name, widget->selection_id, 1);
	gui_internal_search(this,_("Street"),"Street",0);
}

void
gui_internal_search_town_in_country(struct gui_priv *this, struct widget *widget)
{
	struct search_list_common *slc;
	dbg(2,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_country_all, 0, 0);
	slc=search_list_select(this->sl, attr_country_all, widget->selection_id, 1);
	if (slc) {
		g_free(this->country_iso2);
		this->country_iso2=g_strdup(((struct search_list_country *)slc)->iso2);
	}
	gui_internal_search(this,widget->name,"Town",0);
}

