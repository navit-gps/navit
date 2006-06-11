#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "transform.h"
#include "block.h"
#include "data_window.h"
#include "country.h"
#include "town.h"
#include "street.h"
#include "street_name.h"
#include "gui/gtk/gtkeyboard.h"
#include "cursor.h"
#include "route.h"
#include "statusbar.h"
#include "unistd.h"
#include "destination.h"
#include "coord.h"
#include "container.h"
#include "graphics.h"

extern gint track_focus(gpointer data);



GtkWidget *entry_country, *entry_postal, *entry_city, *entry_district;
GtkWidget *entry_street, *entry_number;
GtkWidget *listbox, *current=NULL;
int row_count=8;

int selected;

struct search_param {
	struct map_data *map_data;
	const char *country;
	GHashTable *country_hash;
	const char *town;
	GHashTable *town_hash;
	GHashTable *district_hash;
	const char *street;
	GHashTable *street_hash;
	const char *number;
	int number_low, number_high;
	GtkWidget *clist;
	int count;
} search_param2 = {};

struct destination {
	struct town *town;
	struct street_name *street_name;
	struct coord *c;
};

struct country_list *country_list;

static void
select_row(GtkCList *clist, int row, int column, GdkEventButton *event, struct data_window *win)
{
	selected=row;
	gchar *text;
	printf("Selected row %d, column %d\n", row, column);
	
	if(current==entry_country) {
		gtk_clist_get_text(GTK_CLIST(clist),row,0,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_country),g_strdup(text));
	}
	else if( current==entry_city) {
		gtk_clist_get_text(GTK_CLIST(clist),row,5,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_city),g_strdup(text));
	}
	else if( current==entry_street) {
		gtk_clist_get_text(GTK_CLIST(clist),row,4,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_street),g_strdup(text));
	}
	else if( current==entry_number) {
		gtk_clist_get_text(GTK_CLIST(clist),row,8,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_number),g_strdup(text));
	}
}

int
destination_set(struct container *co, enum destination_type type, char *text, struct coord *pos)
{
	route_set_position(co->route, cursor_pos_get(co->cursor));
	route_set_destination(co->route, pos);
	graphics_redraw(co);
	if (co->statusbar && co->statusbar->statusbar_route_update)
		co->statusbar->statusbar_route_update(co->statusbar, co->route);
	return 0;
}

static int
get_position(struct search_param *search, struct coord *c)
{
	struct destination *dest;

	if (selected == -1)
		selected=0;
	dest=gtk_clist_get_row_data (GTK_CLIST(search->clist), selected);

	printf("row %d dest %p dest:0x%lx,0x%lx\n", selected, dest, dest->c->x, dest->c->y);
	*c=*dest->c;
	return 0;
}

static void button_map(GtkWidget *widget, struct container *co)
{
	struct coord c;

	if (!get_position(&search_param2, &c)) {
		graphics_set_view(co, &c.x, &c.y, NULL);
	}
}

static void button_destination(GtkWidget *widget, struct container *co)
{
	struct coord c;

	if (!get_position(&search_param2, &c)) {
		route_set_position(co->route, cursor_pos_get(co->cursor));
		route_set_destination(co->route, &c);
	        graphics_redraw(co);
	}
}

struct dest_town {
	int country;
	int assoc;
	char *name;
	char postal_code[16];
	struct town town;
};

static guint
destination_town_hash(gconstpointer key)
{
	const struct dest_town *hash=key;
	gconstpointer hashkey=(gconstpointer)(hash->country^hash->assoc);
	return g_direct_hash(hashkey);	
}

static gboolean
destination_town_equal(gconstpointer a, gconstpointer b)
{
	const struct dest_town *t_a=a;	
	const struct dest_town *t_b=b;	
	if (t_a->assoc == t_b->assoc && t_a->country == t_b->country) {
		if (t_a->name && t_b->name && strcmp(t_a->name, t_b->name))
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

static GHashTable *
destination_town_new(void)
{
	return g_hash_table_new_full(destination_town_hash, destination_town_equal, NULL, g_free);
}

static void
destination_town_set(const struct dest_town *town, char **rows, int full)
{
	char country[32];
	struct country *cou;
	if ((cou=country_get_by_id(town->country))) {
		rows[1]=cou->car;
	} else {
		sprintf(country,"(%d)", town->country);
		rows[1]=country;
	} 
	if (full) {
		rows[4]=(char *)(town->town.postal_code2);
		rows[5]=g_convert(town->town.name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		if (town->town.district[0]) 
			rows[6]=g_convert(town->town.district,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		else
			rows[6]=NULL;
	} else {
		rows[4]=(char *)(town->postal_code);
		rows[5]=g_convert(town->name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
	} 
}

static void
destination_town_show(gpointer key, gpointer value, gpointer user_data)
{
	struct dest_town *town=value;
	struct search_param *search=(struct search_param *)user_data;
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	int row;

	if (search->count > 0) {
		struct destination *dest=g_new(struct destination, 1);
		dest->town=&town->town;
		dest->street_name=NULL;
		dest->c=town->town.c;
		destination_town_set(town, rows, 0);
		row=gtk_clist_append(GTK_CLIST(search->clist), rows);
		printf("town row %d %p dest:0x%lx,0x%lx\n", row, dest, dest->c->x, dest->c->y);
		gtk_clist_set_row_data(GTK_CLIST(search->clist), row, dest);
		search->count--;
	}
}

static GHashTable *
destination_country_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}

static int
destination_country_add(struct country *cou, void *data)
{
	struct search_param *search=data;
	struct country *cou2;

	void *first;
	first=g_hash_table_lookup(search->country_hash, (void *)(cou->id));
	if (! first) {
		cou2=g_new(struct country, 1);
		*cou2=*cou;
		g_hash_table_insert(search->country_hash, (void *)(cou->id), cou2);
	}
	return 0;
}

static void
destination_country_show(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search_param *search=(struct search_param *)user_data;
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	if (search->count > 0) {
		rows[0]=cou->name;
		rows[1]=cou->car;
		rows[2]=cou->iso2;
		rows[3]=cou->iso3;
		gtk_clist_append(GTK_CLIST(search->clist), rows);
		search->count--;
	}
}

static int
destination_town_add(struct town *town, void *data)
{
	struct search_param *search=data;
	struct dest_town *first;

	struct dest_town cmp;
	char *zip1, *zip2;

	if (town->id == 0x1d546b7e) {
		printf("found\n");
	}
	cmp.country=town->country;
	cmp.assoc=town->street_assoc;
	cmp.name=town->name;
	first=g_hash_table_lookup(search->town_hash, &cmp);
	if (! first) {
		first=g_new(struct dest_town, 1);
		first->country=cmp.country;
		first->assoc=cmp.assoc;
		strcpy(first->postal_code, town->postal_code2);
		first->name=town->name;
		first->town=*town;
		g_hash_table_insert(search->town_hash, first, first);
	} else {
		zip1=town->postal_code2;
		zip2=first->postal_code;
		while (*zip1 && *zip2) {
			if (*zip1 != *zip2) {
				while (*zip2) {
					*zip2++='.';
				}
				break;
			}
			zip1++;
			zip2++;	
		}
	}
	cmp.name=NULL;
	cmp.assoc=town->id;
	first=g_hash_table_lookup(search->district_hash, &cmp);
	if (! first) {
		first=g_new(struct dest_town, 1);
		first->country=cmp.country;
		first->assoc=cmp.assoc;
		first->name=NULL;
		first->town=*town;
		g_hash_table_insert(search->district_hash, first, first);
	}
	return 0;
}

static void
destination_town_search(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search_param *search=(struct search_param *)user_data;
	town_search_by_name(search->map_data, cou->id, search->town, 1, destination_town_add, search);
	
}

static GHashTable *
destination_street_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}


static int
destination_street_add(struct street_name *name, void *data)
{
	struct search_param *search=data;
	struct street_name *name2;

	name2=g_new(struct street_name, 1);
	*name2=*name;
	g_hash_table_insert(search->street_hash, name2, name2);
	return 0;
}

static int
number_partial(int search, int ref, int ext)
{
	int max=1;

	printf("number_partial(%d,%d,%d)", search, ref, ext);
	if (ref >= 10)
		max=10;
	if (ref >= 100)
		max=100;
	if (ref >= 1000)
		max=1000;
	while (search < max) {
		search*=10;
		search+=ext;
	}
	printf("max=%d result=%d\n", max, search);
	return search;
}

static int
check_number(int low, int high, int s_low, int s_high)
{
	printf("check_number(%d,%d,%d,%d)\n", low, high, s_low, s_high);
	if (low <= s_high && high >= s_low)
		return 1;
	if (s_low == s_high) {
		if (low <= number_partial(s_high, high, 9) && high >= number_partial(s_low, low, 0)) 
			return 1;
	}
	printf("return 0\n");
	return 0;
}

static void
destination_street_show_common(gpointer key, gpointer value, gpointer user_data, int number)
{
	struct street_name *name=value;
	struct search_param *search=(struct search_param *)user_data;
	char *utf8;
	struct dest_town cmp;
	struct dest_town *town;
	int row;
	char buffer[32];
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	struct street_name_info info;
	struct street_name_number_info num_info;

	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	while (street_name_get_info(&info, name) && search->count > 0) {
		struct destination *dest;
		cmp.country=info.country;
		cmp.assoc=info.dist;
		cmp.name=NULL;
		town=g_hash_table_lookup(search->district_hash, &cmp);
		printf("town=%p\n", town);
		if (town) {
			destination_town_set(town, rows, 1);
			utf8=g_convert(name->name2,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			rows[4]=utf8;
			if (number) {
				info.tmp_len=info.aux_len;
				info.tmp_data=info.aux_data;
				while (street_name_get_number_info(&num_info, &info) && search->count > 0) {
					dest=g_new(struct destination, 1);
					dest->town=&town->town;
					dest->street_name=name;
					dest->c=num_info.c;
					if (check_number(num_info.first, num_info.last, search->number_low, search->number_high)) {
						if (num_info.first == num_info.last)
							sprintf(buffer,"%d",num_info.first);
						else
							sprintf(buffer,"%d-%d",num_info.first,num_info.last);
						rows[8]=buffer;
						printf("'%s','%s','%s','%s','%s','%s'\n", rows[0],rows[1],rows[2],rows[3],rows[4],rows[5]);
						row=gtk_clist_append(GTK_CLIST(listbox), rows);
						gtk_clist_set_row_data(GTK_CLIST(listbox), row, dest);
						search->count--;
					}
				}
			} else {
				row=gtk_clist_append(GTK_CLIST(listbox), rows);
				dest=g_new(struct destination, 1);
				dest->town=&town->town;
				dest->street_name=name;
				dest->c=info.c;
				gtk_clist_set_row_data(GTK_CLIST(listbox), row, dest);
				search->count--;
			}
			g_free(utf8);
		} else {
			printf("Town for '%s' not found\n", name->name2);
		}
	}
}

static void
destination_street_show(gpointer key, gpointer value, gpointer user_data)
{
	destination_street_show_common(key, value, user_data, 0);
}

static void
destination_street_show_number(gpointer key, gpointer value, gpointer user_data)
{
	destination_street_show_common(key, value, user_data, 1);
}

static void
destination_street_search(gpointer key, gpointer value, gpointer user_data)
{
	const struct dest_town *town=value;
	struct search_param *search=(struct search_param *)user_data;
	street_name_search(search->map_data, town->country, town->assoc, search->street, 1, destination_street_add, search);
}



static void changed(GtkWidget *widget, struct search_param *search)
{
	char *str;
	char *empty[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	char *dash;

	current=widget;
    
	gtk_clist_freeze(GTK_CLIST(listbox));
	gtk_clist_clear(GTK_CLIST(listbox));
	
	selected=-1;

	search->count=row_count;

	str=g_convert(gtk_entry_get_text(GTK_ENTRY(widget)),-1,"iso8859-1","utf-8",NULL,NULL,NULL);
	/*FIXME free buffers with g_free() after search or when the dialog gets closed. */

	if (widget == entry_country) {
		if (search->country_hash) g_hash_table_destroy(search->country_hash);
		search->country_hash=NULL;
	}
	if (widget == entry_country || widget == entry_city) {
		if (search->town_hash) g_hash_table_destroy(search->town_hash);
		if (search->district_hash) g_hash_table_destroy(search->district_hash);
		search->town_hash=NULL;
		search->district_hash=NULL;
	}

	if (widget == entry_country || widget == entry_city || widget == entry_street) {
		if (search->street_hash) g_hash_table_destroy(search->street_hash);
		search->street_hash=NULL;
	}

	if (widget != entry_number) {
		gtk_entry_set_text(GTK_ENTRY(entry_number),"");
		if (widget != entry_street) {
			gtk_entry_set_text(GTK_ENTRY(entry_street),"");
			if (widget != entry_city) {
				gtk_entry_set_text(GTK_ENTRY(entry_city),"");
			}
		}
	}

	if (widget == entry_country) {
		search->country_hash=destination_country_new();
		g_free(str);
		if (search->country) g_free(search->country);
		search->country = str = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		country_search_by_name(str, 1, destination_country_add, search);
		country_search_by_car(str, 1, destination_country_add, search);
		country_search_by_iso2(str, 1, destination_country_add, search);
		country_search_by_iso3(str, 1, destination_country_add, search);
		g_hash_table_foreach(search->country_hash, destination_country_show, search);
	}
	if (widget == entry_city) {
		int i;
		for(i = 0 ;i < strlen(str); i++) {
			char u = toupper(str[i]);

			if (u == 'ä') str[i] = 'a';
			if (u == 'ö') str[i] = 'o';
			if (u == 'ü') str[i] = 'u';
			if (u == 'ß') {
				char *tmp;
				str[i] = '\0';
				tmp = g_strjoin(NULL, str, "ss", str[i+1], NULL);
				g_free(str);
				str = tmp;
				i++;
			}
			printf("\"%s\"\n",str);
		}
		printf("Ort: '%s'\n", str);
		if (strlen(str) > 1) {
			if (search->town) g_free(search->town);
			search->town=str;
			search->town_hash=destination_town_new();
			search->district_hash=destination_town_new();
			g_hash_table_foreach(search->country_hash, destination_town_search, search);
			g_hash_table_foreach(search->town_hash, destination_town_show, search);
		}
	}
	if (widget == entry_street) {
		printf("Street: '%s'\n", str);
		if (search->street) g_free(search->street);
		search->street=str;
		search->street_hash=destination_street_new();
		g_hash_table_foreach(search->town_hash, destination_street_search, search);
		g_hash_table_foreach(search->street_hash, destination_street_show, search);
	}
	if (widget == entry_number) {
		char buffer[strlen(str)+1];
		strcpy(buffer, str);
		search->number=str;
		dash=index(buffer,'-'); 
		if (dash) {
			*dash++=0;
			search->number_low=atoi(buffer);
			if (strlen(str)) 
				search->number_high=atoi(dash);
			else
				search->number_high=10000;
		} else {
			if (!strlen(str)) {
				search->number_low=0;
				search->number_high=10000;
			} else {
				search->number_low=atoi(str);
				search->number_high=atoi(str);
			}
		}
		g_hash_table_foreach(search->street_hash, destination_street_show_number, search);
	}
	while (search->count-- > 0) {
		gtk_clist_append(GTK_CLIST(listbox), empty);
	}
        gtk_clist_columns_autosize (GTK_CLIST(listbox));
	gtk_clist_thaw(GTK_CLIST(listbox));
}

int destination_address(struct container *co)
{
	GtkWidget *window2, *keyboard, *vbox, *table;
	GtkWidget *label_country;
	GtkWidget *label_postal, *label_city, *label_district;
	GtkWidget *label_street, *label_number;
	GtkWidget *hseparator1,*hseparator2;
	GtkWidget *button1,*button2;
	init_keyboard_stuff((char *) NULL);
	int handlerid;
	int i;
	gchar *text[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	struct search_param *search=&search_param2;

#if 0
	if (co->cursor) {
		struct coord *c;
		struct route_info *rt;
		struct street_str *st;
		struct block_info *blk;
		struct street_name name;
		struct town town;

		c=cursor_pos_get(co->cursor);
		rt=route_find_nearest_street(co->map_data, c);
		st=route_info_get_street(rt);	
		blk=route_info_get_block(rt);
		printf("segid 0x%lx nameid 0x%lx\n", st->segid, st->nameid);
		street_name_get_by_id(&name, blk->mdata, st->nameid);
		printf("'%s' '%s' %d\n", name.name1, name.name2, name.segment_count);
		for (i = 0 ; i < name.segment_count ; i++) {
			if (name.segments[i].segid == st->segid) {
				printf("found: 0x%x, 0x%x\n", name.segments[i].country, name.segments[i].segid);
				town_get_by_id(&town, co->map_data, name.segments[i].country, name.townassoc);
				printf("%s/%s\n", town.name, town.district);	
			}
		}
	}
#endif

	window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	keyboard = build_keyboard(NULL, "/usr/share/gtkeyboard/key/DE.key");
	vbox = gtk_vbox_new(FALSE, 0);
	table = gtk_table_new(3, 8, FALSE);

	entry_country = gtk_entry_new();
	label_country = gtk_label_new("Land");
	entry_postal = gtk_entry_new();
	label_postal = gtk_label_new("PLZ");
	entry_city = gtk_entry_new();
	label_city = gtk_label_new("Ort");
	entry_district = gtk_entry_new();
	label_district = gtk_label_new("Ortsteil/Gemeinde");
	hseparator1 = gtk_vseparator_new();
	entry_street = gtk_entry_new();
	label_street = gtk_label_new("Strasse");
	entry_number = gtk_entry_new();
	label_number = gtk_label_new("Nummer");
	listbox = gtk_clist_new(9);
	for (i=0 ; i < row_count ; i++) {
		gtk_clist_append(GTK_CLIST(listbox), text);
	}
	gtk_clist_thaw(GTK_CLIST(listbox));
        gtk_clist_columns_autosize (GTK_CLIST(listbox));

	hseparator2 = gtk_vseparator_new();
	button1 = gtk_button_new_with_label("Karte");
	button2 = gtk_button_new_with_label("Ziel");

	gtk_table_attach(GTK_TABLE(table), label_country,  0, 1,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_postal,   1, 2,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_city,     2, 3,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), entry_country,  0, 1,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_postal,   1, 2,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_city,     2, 3,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), label_district, 0, 1,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_street,   1, 2,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_number,   2, 3,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), entry_district, 0, 1,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_street,   1, 2,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_number,   2, 3,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), listbox,        0, 3,  4, 5,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), button1, 0, 1, 5, 6, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), button2, 2, 3, 5, 6, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

	search->map_data=co->map_data;
	search->clist=listbox;
	g_signal_connect(G_OBJECT(entry_country), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(entry_postal), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(entry_city), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(entry_district), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(entry_street), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(entry_number), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(button_map), co);
	g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(button_destination), co);
	gtk_widget_grab_focus(entry_city);

	gtk_container_add(GTK_CONTAINER(vbox), table);
	gtk_container_add(GTK_CONTAINER(vbox), keyboard);
	gtk_container_add(GTK_CONTAINER(window2), vbox);
	handlerid = gtk_timeout_add(256, (GtkFunction) track_focus, NULL);

	g_signal_connect(G_OBJECT(listbox), "select-row", G_CALLBACK(select_row), NULL);
	
	gtk_widget_show_all(window2);

	gtk_entry_set_text(GTK_ENTRY(entry_country),"Deutschland");

	return 0;
}
