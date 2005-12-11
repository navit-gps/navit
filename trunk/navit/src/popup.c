#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "file.h"
#include "map_data.h"
#include "block.h"
#include "display.h"
#include "town.h"
#include "street.h"
#include "poly.h"
#include "log.h"
#include "popup.h"
#include "plugin.h"
#include "vehicle.h"
#include "route.h"
#include "cursor.h"
#include "statusbar.h"
#include "container.h"
#include "graphics.h"

static void
popup_item_destroy_text(struct popup_item *item)
{
	g_free(item->text);
	g_free(item);
}

struct popup_item *
popup_item_new_text(struct popup_item **last, char *text, int priority)
{
	struct popup_item *curr;
	curr=g_new(struct popup_item,1);
	memset(curr, 0, sizeof(*curr));
	curr->text=g_strdup(text);
	curr->priority=priority;
	curr->destroy=popup_item_destroy_text;
	if (last) {
		curr->next=*last;
		*last=curr;
	}
	return curr;
}

static struct popup_item *
popup_item_new_func(struct popup_item **last, char *text, int priority, void (*func)(struct popup_item *, void *), void *param)
{
	struct popup_item *curr=popup_item_new_text(last, text, priority);
	curr->func=func;
	curr->param=param;
	return curr;
}

static struct popup_item *
param_to_menu_new(char *name,struct param_list *plist, int c, int iso)
{
	struct popup_item *last, *curr, *ret;
	int i;

	ret=popup_item_new_text(NULL,name,1);
	last=NULL;
	for (i = 0 ; i < c ; i++) {
		char name_buffer[strlen(plist[i].name)+strlen(plist[i].value)+2];
		char *text=name_buffer;

		sprintf(name_buffer,"%s:%s", plist[i].name, plist[i].value);
		if (iso) {
			text=g_convert(name_buffer,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			if (! text) {
				printf("problem converting '%s'\n", name_buffer);
			}
		}
		curr=popup_item_new_text(&last, text, i);
		if (iso)
			free(text);
	}
	ret->submenu=last;
	return ret;
}

static void
popup_set_no_passing(struct popup_item *item, void *param)
{
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
}

static void
popup_set_destination(struct popup_item *item, void *param)
{
	struct popup_item *ref=param;
	struct popup *popup=ref->param;
	struct container *co=popup->co;
	printf("Destination %s\n", ref->text);
	route_set_position(co->route, cursor_pos_get(co->cursor));
	route_set_destination(co->route, &popup->c);
	graphics_redraw(popup->co);
	if (co->statusbar && co->statusbar->statusbar_route_update)
		co->statusbar->statusbar_route_update(co->statusbar, co->route);
}

extern void *vehicle;

static void
popup_set_position(struct popup_item *item, void *param)
{
	struct popup_item *ref=param;
	struct popup *popup=ref->param;
	printf("Position %s\n", ref->text);
	vehicle_set_position(popup->co->vehicle, &popup->c);	
}

#if 0
static void
popup_break_crossing(struct display_list *l)
{
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

static void
popup_call_func(GtkObject *obj, void *parm)
{
	struct popup_item *curr=parm;
	curr->func(curr, curr->param);
}

static GtkWidget *
popup_menu(struct popup_item *list)
{
	int min_prio,curr_prio;
	struct popup_item *curr;
	GtkWidget *item,*menu,*submenu;

	curr_prio=0;
	menu=gtk_menu_new();
	do {
		min_prio=INT_MAX;
		curr=list;
		while (curr) {
			if (curr->priority == curr_prio) {
				item=gtk_menu_item_new_with_label(curr->text);
				gtk_menu_append(GTK_MENU(menu), item);
				if (curr->submenu) {
					submenu=popup_menu(curr->submenu);
					gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
				} else if (curr->func) {
					gtk_signal_connect(GTK_OBJECT(item), "activate",
					 	GTK_SIGNAL_FUNC (popup_call_func), curr);
				}
			}
			if (curr->priority > curr_prio && curr->priority < min_prio)
				min_prio=curr->priority;
			curr=curr->next;
		}
		curr_prio=min_prio;
	} while (min_prio != INT_MAX);
	return menu;
}

static void
popup_display_list_default(struct display_list *d, struct popup_item **popup_list)
{
	struct segment *seg;
	char *desc,*text,*item_text;
	struct popup_item *curr_item,*submenu;
	struct param_list plist[100];    

	desc=NULL;
	if (d->type == 0) desc="Polygon";	
	if (d->type == 1) desc="Polyline";
	if (d->type == 2) desc="Street";
	if (d->type == 3) desc="Label";
	if (d->type == 4) desc="Point";
	seg=(struct segment *)(d->data);
	if (seg) {
		if (d->label && strlen(d->label)) {
			item_text=malloc(strlen(desc)+strlen(d->label)+2);
			strcpy(item_text, desc);
			strcat(item_text," ");
			strcat(item_text, d->label);	
		} else {
			item_text=desc;
		}
		text=g_convert(item_text,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		curr_item=popup_item_new_text(popup_list,text,1);
		g_free(text);

		curr_item->submenu=param_to_menu_new("File", plist, file_get_param(seg->blk_inf.file, plist, 100), 1);
		submenu=curr_item->submenu;
		submenu->next=param_to_menu_new("Block", plist, block_get_param(&seg->blk_inf, plist, 100), 1);
		submenu=submenu->next;
		
		if (d->type == 0 || d->type == 1) {
			submenu->next=param_to_menu_new(desc, plist, poly_get_param(seg, plist, 100), 1);
		}
		if (d->type == 2) {
			submenu->next=param_to_menu_new(desc, plist, street_get_param(seg, plist, 100, 1), 1);
			popup_item_new_func(&submenu->next,"Set no passing", 1000, popup_set_no_passing, d);
		}
		if (d->type == 3) {
			submenu->next=param_to_menu_new(desc, plist, town_get_param(seg, plist, 100), 1);
		}
		if (d->type == 4) {
			submenu->next=param_to_menu_new(desc, plist, street_bti_get_param(seg, plist, 100), 1);
		}
	}
}

static void
popup_display_list(struct container *co, struct popup *popup, struct popup_item **popup_list)
{
	GtkWidget *menu, *item;
	struct display_list *list[100],**p=list;

	menu=gtk_menu_new();
	item=gtk_menu_item_new_with_label("Selection");
	gtk_menu_append (GTK_MENU(menu), item);	
	display_find(&popup->pnt, co->disp, display_end, 3, list, 100);
	while (*p) {
		if (! (*p)->info)
			popup_display_list_default(*p, popup_list);
		else
			(*(*p)->info)(*p, popup_list);
		p++;
	}	
}

static void
popup_destroy_items(struct popup_item *item)
{
	struct popup_item *next;
	while (item) {
		if (item->active && item->func)
			item->func(item, item->param);
		if (item->submenu)
			popup_destroy_items(item->submenu);
		next=item->next;
		assert(item->destroy != NULL);
		item->destroy(item);
		item=next;
	}
}

static void
popup_destroy(GtkObject *obj, void *parm)
{
	struct popup *popup=parm;

	popup_destroy_items(popup->items);
	g_free(popup);	
}

void
popup(struct container *co, int x, int y, int button)
{
	GtkWidget *menu;
	struct popup *popup=g_new(struct popup,1);
	struct popup_item *list=NULL;
	struct popup_item *descr;
	struct coord_geo g;
	char buffer[256];

	popup->co=co;
	popup->pnt.x=x;
	popup->pnt.y=y;
	transform_reverse(co->trans, &popup->pnt, &popup->c);
	popup_display_list(co, popup, &list);
	plugin_call_popup(co, popup, &list);
	transform_lng_lat(&popup->c, &g);
	strcpy(buffer,"Map Point ");
	transform_geo_text(&g, buffer+strlen(buffer));
	descr=popup_item_new_text(&list,buffer, 0);
	descr->param=popup;

	popup_item_new_func(&list,"Set as Position", 1000, popup_set_position, descr);
	popup_item_new_func(&list,"Set as Destination", 1001, popup_set_destination, descr);

	popup->items=list;
	menu=popup_menu(list);
	gtk_widget_show_all(menu);
	popup->gui_data=menu;


	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, button, gtk_get_current_event_time());
	gtk_signal_connect(GTK_OBJECT(menu), "selection-done", GTK_SIGNAL_FUNC (popup_destroy), popup);
}


