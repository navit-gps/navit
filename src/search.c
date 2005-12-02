#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "search.h"
#include "coord.h"
#include "country.h"
#include "town.h"
#include "street.h"
#include "street_name.h"


struct search {
	struct map_data *map_data;
	char *country;
	GHashTable *country_hash;
	char *postal;
	char *town;
	GHashTable *town_hash;
	char *district;
	GHashTable *district_hash;
	char *street;
	GHashTable *street_hash;
	char *number;
	int number_low, number_high;
	int (*func)(struct search_destination *dest, void *user_data);
	void *user_data;
};

struct dest_town {
	int country;
	int assoc;
	char *name;
	char postal_code[16];
	struct town town;
};

static GHashTable *
search_country_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}

static int
search_country_add(struct country *cou, void *data)
{
	struct search *search=data;
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
search_country_show(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search *search=(struct search *)user_data;
	struct search_destination dest;

	memset(&dest, 0, sizeof(dest));
	dest.country=cou;
	dest.country_name=cou->name;
	dest.country_car=cou->car;
	dest.country_iso2=cou->iso2;
	dest.country_iso3=cou->iso3;
	(*search->func)(&dest, search->user_data);
}

static guint
search_town_hash(gconstpointer key)
{
	const struct dest_town *hash=key;
	gconstpointer hashkey=(gconstpointer)(hash->country^hash->assoc);
	return g_direct_hash(hashkey);	
}

static gboolean
search_town_equal(gconstpointer a, gconstpointer b)
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
search_town_new(void)
{
	return g_hash_table_new_full(search_town_hash, search_town_equal, NULL, g_free);
}


static int
search_town_add(struct town *town, void *data)
{
	struct search *search=data;
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
search_town_search(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search *search=user_data;

	town_search_by_name(search->map_data, cou->id, search->town, 1, search_town_add, search);
}

static void
search_town_set(const struct dest_town *town, struct search_destination *dest, int full)
{
	char country[32];
	struct country *cou;
	if ((cou=country_get_by_id(town->country))) {
		dest->country=cou;
		dest->country_name=cou->name;
		dest->country_car=cou->car;
		dest->country_iso2=cou->iso2;
		dest->country_iso3=cou->iso3;
	} else {
		sprintf(country,"(%d)", town->country);
		dest->country=NULL;
		dest->country_car=country;
	} 
	if (full) {
		dest->town_postal=(char *)(town->town.postal_code2);
		dest->town_name=g_convert(town->town.name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		if (town->town.district[0]) 
			dest->district=g_convert(town->town.district,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		else
			dest->district=NULL;
	} else {
		dest->town_postal=(char *)(town->postal_code);
		dest->town_name=g_convert(town->name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
	} 
}


static void
search_town_show(gpointer key, gpointer value, gpointer user_data)
{
	struct dest_town *town=value;
	struct search *search=user_data;
	struct search_destination dest;

	memset(&dest, 0, sizeof(dest));
	dest.town=&town->town;
	dest.street_name=NULL;
	dest.c=town->town.c;
	search_town_set(town, &dest, 0);
	
	(*search->func)(&dest, search->user_data);
}


GHashTable *
search_street_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}


static int
search_street_add(struct street_name *name, void *data)
{
	struct search *search=data;
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
	int debug=0;

	if (debug)
		printf("check_number(%d,%d,%d,%d)\n", low, high, s_low, s_high);
	if (low <= s_high && high >= s_low)
		return 1;
	if (s_low == s_high) {
		if (low <= number_partial(s_high, high, 9) && high >= number_partial(s_low, low, 0)) 
			return 1;
	}
	if (debug)
		printf("return 0\n");
	return 0;
}

static void
search_street_show_common(gpointer key, gpointer value, gpointer user_data, int number)
{
	struct street_name *name=value;
	struct search *search=user_data;
	char *utf8;
	struct dest_town cmp;
	struct dest_town *town;
	char buffer[32];
	struct street_name_info info;
	struct street_name_number_info num_info;
	struct search_destination dest;
	int debug=0;

	memset(&dest, 0, sizeof(dest));
	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	while (street_name_get_info(&info, name)) {
		cmp.country=info.country;
		cmp.assoc=info.dist;
		cmp.name=NULL;
		town=g_hash_table_lookup(search->district_hash, &cmp);
		if (debug)
			printf("town=%p\n", town);
		if (town) {
			search_town_set(town, &dest, 1);
			utf8=g_convert(name->name2,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			dest.street_name=utf8;
			if (number) {
				info.tmp_len=info.aux_len;
				info.tmp_data=info.aux_data;
				while (street_name_get_number_info(&num_info, &info)) {
					dest.town=&town->town;
					dest.street=name;
					dest.c=num_info.c;
					if (check_number(num_info.first, num_info.last, search->number_low, search->number_high)) {
						if (num_info.first == num_info.last)
							sprintf(buffer,"%d",num_info.first);
						else
							sprintf(buffer,"%d-%d",num_info.first,num_info.last);
						dest.street_number=buffer;
						(*search->func)(&dest, search->user_data);
					}
				}
			} else {
				dest.town=&town->town;
				dest.street=name;
				dest.c=info.c;
				(*search->func)(&dest, search->user_data);
			}
			g_free(utf8);
		} else {
			printf("Town for '%s' not found\n", name->name2);
		}
	}
}

static void
search_street_show(gpointer key, gpointer value, gpointer user_data)
{
	search_street_show_common(key, value, user_data, 0);
}

static void
search_street_show_number(gpointer key, gpointer value, gpointer user_data)
{
	search_street_show_common(key, value, user_data, 1);
}

static void
search_street_search(gpointer key, gpointer value, gpointer user_data)
{
	const struct dest_town *town=value;
	struct search *search=user_data;
	street_name_search(search->map_data, town->country, town->assoc, search->street, 1, search_street_add, search);
}



void search_update(struct search *search, enum search_param what, char *val)
{
	char *dash;

	if (what == search_country) {
		if (search->country_hash) g_hash_table_destroy(search->country_hash);
		search->country_hash=NULL;
	}
	if (what == search_country || what == search_town) {
		if (search->town_hash) g_hash_table_destroy(search->town_hash);
		if (search->district_hash) g_hash_table_destroy(search->district_hash);
		search->town_hash=NULL;
		search->district_hash=NULL;
	}

	if (what == search_country || what == search_town || what == search_street) {
		if (search->street_hash) g_hash_table_destroy(search->street_hash);
		search->street_hash=NULL;
	}

	if (what == search_country) {
		g_free(search->country);
		search->country=g_strdup(val);
		if (val) {
			search->country_hash=search_country_new();
			country_search_by_name(val, 1, search_country_add, search);
			country_search_by_car(val, 1, search_country_add, search);
			country_search_by_iso2(val, 1, search_country_add, search);
			country_search_by_iso3(val, 1, search_country_add, search);
			g_hash_table_foreach(search->country_hash, search_country_show, search);
		}
	}
	if (what == search_town) {
		g_free(search->town);
		search->town=g_strdup(val);
		if (val) {
			search->town_hash=search_town_new();
			search->district_hash=search_town_new();
			g_hash_table_foreach(search->country_hash, search_town_search, search);
			g_hash_table_foreach(search->town_hash, search_town_show, search);
		}
	}
	if (what == search_street) {
		g_free(search->street);
		search->street=g_strdup(val);
		if (val) {
			search->street_hash=search_street_new();
			g_hash_table_foreach(search->town_hash, search_street_search, search);
			g_hash_table_foreach(search->street_hash, search_street_show, search);
		}
	}
	if (what == search_number) {
		g_free(search->number);
		search->number=g_strdup(val);
		if (val) {
			char buffer[strlen(val)+1];
			strcpy(buffer, val);
			dash=index(buffer,'-'); 
			if (dash) {
				*dash++=0;
				search->number_low=atoi(buffer);
				if (strlen(val)) 
					search->number_high=atoi(dash);
				else
					search->number_high=10000;
			} else {
				if (!strlen(val)) {
					search->number_low=0;
					search->number_high=10000;
				} else {
					search->number_low=atoi(val);
					search->number_high=atoi(val);
				}
			}
			g_hash_table_foreach(search->street_hash, search_street_show_number, search);
		}
	}
}

struct search *
search_new(struct map_data *mdat, char *country, char *postal, char *town, char *district, char *street, char *number, int (*func)(struct search_destination *dest, void *user_data), void *user_data)
{
	struct search *this=g_new0(struct search,1);
	this->map_data=mdat;
	this->country=g_strdup(country);
	this->postal=g_strdup(postal);
	this->town=g_strdup(town);
	this->district=g_strdup(district);
	this->street=g_strdup(street);
	this->number=g_strdup(number);
	this->func=func;
	this->user_data=user_data;
	return this;
}
