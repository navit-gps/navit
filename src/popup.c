#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
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
#include "map.h"
#include "graphics.h"
#include "item.h"
#include "route.h"

#if 0
static void
popup_set_no_passing(struct popup_item *item, void *param)
{
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

static void
popup_set_destination(struct menu *menu, void *data1, void *data2)
{
	struct navit *nav=data1;
	struct coord *c=data2;
	struct coord_geo g;
	char buffer[1024];
	char buffer_geo[1024];
	transform_to_geo(transform_get_projection(navit_get_trans(nav)), c, &g);
	transform_geo_text(&g, buffer_geo);	
	sprintf(buffer,"Map Point %s", buffer_geo);
	navit_set_destination(nav, c, buffer);
}

static void
popup_set_bookmark(struct menu *menu, void *data1, void *data2)
{
	struct navit *nav=data1;
	struct coord *c=data2;
	struct coord_geo g;
	char buffer[1024];
	char buffer_geo[1024];
	transform_to_geo(transform_get_projection(navit_get_trans(nav)), c, &g);
	transform_geo_text(&g, buffer_geo);	
	sprintf(buffer,"Map Point %s", buffer_geo);
	if (!gui_add_bookmark(navit_get_gui(nav), c, buffer)) 
		navit_add_bookmark(nav, c, buffer);
}


extern void *vehicle;

static void
popup_set_position(struct menu *menu, void *data1, void *data2)
{
	struct navit *nav=data1;
	struct coord *c=data2;
	dbg(0,"%p %p\n", nav, c);
	navit_set_position(nav, c);
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


#define popup_printf(menu, type, fmt...) popup_printf_cb(menu, type, NULL, NULL, NULL, fmt)

static void *
popup_printf_cb(void *menu, enum menu_type type, void (*callback)(struct menu *menu, void *data1, void *data2), void *data1, void *data2, const char *fmt, ...)
{
	gchar *str;
	va_list ap;
	void *ret;

	va_start(ap, fmt);
	str=g_strdup_vprintf(fmt, ap);
	dbg(0,"%s\n", str);
	ret=menu_add(menu, str, type, callback, data1, data2);
	va_end(ap);
	g_free(str);
	return ret;
}

static void
popup_show_attr_val(void *menu, struct attr *attr)
{
	char *attr_name=attr_to_name(attr->type);

	printf("attr\n");
	if (attr->type == attr_limit) 
		popup_printf(menu, menu_type_menu, "%s: %d", attr_name, attr->u.num);
	else 
		popup_printf(menu, menu_type_menu, "%s: %s", attr_name, attr->u.str);
}

#if 0
static void
popup_show_attr(void *menu, struct item *item, enum attr_type attr_type)
{
	struct attr attr;
	memset(&attr, 0, sizeof(attr));
	attr.type=attr_type;
	if (item_attr_get(item, attr_type, &attr)) 
		popup_show_attr_val(menu, &attr);
}
#endif

static void
popup_show_attrs(void *menu, struct item *item)
{
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
			popup_show_attr_val(menu, &attr);
		else
			break;
	}
	
#endif
}

static void
popup_show_item(void *popup, struct displayitem *di)
{
	struct map_rect *mr;
	void *menu, *menu_map, *menu_item;
	char *label;
	struct item *item;

	label=graphics_displayitem_get_label(di);
	item=graphics_displayitem_get_item(di);

	if (label) 
		menu=popup_printf(popup, menu_type_submenu, "%s '%s'", item_to_name(item->type), label);
	else
		menu=popup_printf(popup, menu_type_submenu, "%s", item_to_name(item->type));
	menu_item=popup_printf(menu, menu_type_submenu, "Item");
	popup_printf(menu_item, menu_type_menu, "type: 0x%x", item->type);
	popup_printf(menu_item, menu_type_menu, "id: 0x%x 0x%x", item->id_hi, item->id_lo);
	if (item->map) {
		mr=map_rect_new(item->map,NULL);
		item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
		dbg(1,"item=%p\n", item);
		if (item) {
			popup_show_attrs(menu_item, item);
		}
		map_rect_destroy(mr);
		menu_map=popup_printf(menu, menu_type_submenu, "Map");
	} else {
		popup_printf(menu, menu_type_menu, "(No map)");
	}
}

static void
popup_display(struct navit *nav, void *popup, struct point *p)
{
	struct displaylist_handle *dlh;
	struct displaylist *display;
	struct displayitem *di;

	display=navit_get_displaylist(nav);
	dlh=graphics_displaylist_open(display);
	while ((di=graphics_displaylist_next(dlh))) {
		if (graphics_displayitem_within_dist(di, p, 5)) {
			popup_show_item(popup, di);
		}
	}
	graphics_displaylist_close(dlh);
}

struct coord c;

void
popup(struct navit *nav, int button, struct point *p)
{
	void *popup,*men;
	char buffer[1024];
	struct coord_geo g;

	popup=gui_popup_new(navit_get_gui(nav));
	transform_reverse(navit_get_trans(nav), p, &c);
	men=popup_printf(popup, menu_type_submenu, "Point 0x%x 0x%x", c.x, c.y);
	popup_printf(men, menu_type_menu, "Screen %d %d", p->x, p->y);
	transform_to_geo(transform_get_projection(navit_get_trans(nav)), &c, &g);
	transform_geo_text(&g, buffer);	
	popup_printf(men, menu_type_menu, "%s", buffer);
	popup_printf(men, menu_type_menu, "%f %f", g.lat, g.lng);
	dbg(0,"%p %p\n", nav, &c);
	popup_printf_cb(men, menu_type_menu, popup_set_position, nav, &c, "Set as position");
	popup_printf_cb(men, menu_type_menu, popup_set_destination, nav, &c, "Set as destination");
	popup_printf_cb(men, menu_type_menu, popup_set_bookmark, nav, &c, "Add as bookmark");
	popup_display(nav, popup, p);
}
