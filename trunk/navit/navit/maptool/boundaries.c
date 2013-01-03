/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include <stdio.h>
#include <string.h>
#include "maptool.h"
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

char *
osm_tag_value(struct item_bin *ib, char *key)
{
	char *tag=NULL;
	int len=strlen(key);
	while ((tag=item_bin_get_attr(ib, attr_osm_tag, tag))) {
		if (!strncmp(tag,key,len) && tag[len] == '=')
			return tag+len+1;
	}
	return NULL;
}

static char *
osm_tag_name(struct item_bin *ib)
{
	return osm_tag_value(ib, "name");
}

osmid
boundary_relid(struct boundary *b)
{
	long long *id;
	if (!b)
		return 0;
	if (!b->ib)
		return 0;
	id=item_bin_get_attr(b->ib, attr_osm_relationid, NULL);
	if (id)
		return *id;
	return 0;
}
static void
process_boundaries_member(void *func_priv, void *relation_priv, struct item_bin *member, void *member_priv)
{
	struct boundary *b=relation_priv;
	enum geom_poly_segment_type role=(long)member_priv;
	int *dup;
	dup=item_bin_get_attr(member,attr_duplicate,NULL);
	if(!dup || *dup==0)
		b->segments=g_list_prepend(b->segments,item_bin_to_poly_segment(member, role));
}

static GList *
process_boundaries_setup(FILE *boundaries, struct relations *relations)
{
	struct item_bin *ib;
	GList *boundaries_list=NULL;
	struct relations_func *relations_func;

	relations_func=relations_func_new(process_boundaries_member, NULL);
	while ((ib=read_item(boundaries))) {
		char *member=NULL;
		struct boundary *boundary=g_new0(struct boundary, 1);
		char *admin_level=osm_tag_value(ib, "admin_level");
		char *iso=osm_tag_value(ib, "ISO3166-1");
		/* disable spain for now since it creates a too large index */
		if (admin_level && !strcmp(admin_level, "2") && (!iso || strcasecmp(iso,"es"))) {
			if (iso) {
				struct country_table *country=country_from_iso2(iso);	
				if (!country) 
					osm_warning("relation",item_bin_get_relationid(ib),0,"Country Boundary contains unknown ISO3166-1 value '%s'\n",iso);
				else {
					boundary->iso2=g_strdup(iso);
					osm_info("relation",item_bin_get_relationid(ib),0,"Country Boundary for '%s'\n",iso);
				}
				boundary->country=country;
			} else 
				osm_warning("relation",item_bin_get_relationid(ib),0,"Country Boundary doesn't contain an ISO3166-1 tag\n");
		}
		while ((member=item_bin_get_attr(ib, attr_osm_member, member))) {
			long long wayid;
			int read=0;
			if (sscanf(member,"2:"LONGLONG_FMT":%n",&wayid,&read) >= 1) {
				char *rolestr=member+read;
				enum geom_poly_segment_type role;
				if (!strcmp(rolestr,"outer") || !strcmp(rolestr,"exclave"))
					role=geom_poly_segment_type_way_outer;
				else if (!strcmp(rolestr,"inner") || !strcmp(rolestr,"enclave"))
					role=geom_poly_segment_type_way_inner;
				else if (!strcmp(rolestr,""))
					role=geom_poly_segment_type_way_unknown;
				else {
					osm_warning("relation",item_bin_get_relationid(ib),0,"Unknown role %s in member ",rolestr);
					osm_warning("way",wayid,1,"\n");
					role=geom_poly_segment_type_none;
				}
				relations_add_func(relations, relations_func, boundary, (gpointer)role, 2, wayid);
			}
		}
		boundary->ib=item_bin_dup(ib);
		boundaries_list=g_list_append(boundaries_list, boundary);
	}
	return boundaries_list;
}

GList *
boundary_find_matches(GList *l, struct coord *c)
{
	GList *ret=NULL;
	while (l) {
		struct boundary *boundary=l->data;
		if (bbox_contains_coord(&boundary->r, c)) {
			if (geom_poly_segments_point_inside(boundary->sorted_segments,c) > 0) 
				ret=g_list_prepend(ret, boundary);
			ret=g_list_concat(ret,boundary_find_matches(boundary->children, c));
		}
		l=g_list_next(l);
	}
	return ret;
}

#if 0
static void
test(GList *boundaries_list)
{
	struct item_bin *ib;
	FILE *f=fopen("country_276.bin.unsorted","r");
	printf("start\n");
	while ((ib=read_item(f))) {
		struct coord *c=(struct coord *)(ib+1);
		char *name=item_bin_get_attr(ib, attr_town_name, NULL);
		printf("%s:",name);
		boundary_find_matches(boundaries_list, c);
		printf("\n");
	}
	fclose(f);
	printf("end\n");
}

static void
dump_hierarchy(GList *l, char *prefix)
{
	char *newprefix=g_alloca(sizeof(char)*(strlen(prefix)+2));
	strcpy(newprefix, prefix);
	strcat(newprefix," ");
	while (l) {
		struct boundary *boundary=l->data;
		fprintf(stderr,"%s:%s (0x%x,0x%x)-(0x%x,0x%x)\n",prefix,osm_tag_name(boundary->ib),boundary->r.l.x,boundary->r.l.y,boundary->r.h.x,boundary->r.h.y);
		dump_hierarchy(boundary->children, newprefix);
		l=g_list_next(l);
	}
}

static gint
boundary_bbox_compare(gconstpointer a, gconstpointer b)
{
	const struct boundary *boundarya=a;
	const struct boundary *boundaryb=b;
	long long areaa=bbox_area(&boundarya->r);
	long long areab=bbox_area(&boundaryb->r);
	if (areaa > areab)
		return 1;
	if (areaa < areab)
		return -1;
	return 0;
}
#endif

static GList *
process_boundaries_insert(GList *list, struct boundary *boundary)
{
	GList *l=list;
	while (l) {
		struct boundary *b=l->data;
		if (bbox_contains_bbox(&boundary->r, &b->r)) {
			list=g_list_remove(list, b);
			boundary->children=g_list_prepend(boundary->children, b);
			l=list;
		} else if (bbox_contains_bbox(&b->r, &boundary->r)) {
			b->children=process_boundaries_insert(b->children, boundary);
			return list;
		} else
			l=g_list_next(l);
	}
	return g_list_prepend(list, boundary);
}


static GList *
process_boundaries_finish(GList *boundaries_list)
{
	GList *l,*sl;
	GList *ret=NULL;
	l=boundaries_list;
	while (l) {
		struct boundary *boundary=l->data;
		int first=1;
		FILE *f=NULL,*fu=NULL;
		if (boundary->country) {
			char *name=g_strdup_printf("country_%s_poly",boundary->iso2);
			f=tempfile("",name,1);
			g_free(name);
		}
		boundary->sorted_segments=geom_poly_segments_sort(boundary->segments, geom_poly_segment_type_way_right_side);
		sl=boundary->sorted_segments;
		while (sl) {
			struct geom_poly_segment *gs=sl->data;
			struct coord *c=gs->first;
			while (c <= gs->last) {
				if (first) {
					boundary->r.l=*c;
					boundary->r.h=*c;
					first=0;
				} else
					bbox_extend(c, &boundary->r);
				c++;
			}
			if (f) {
				struct item_bin *ib=item_bin;
				item_bin_init(ib, type_selected_line);
				item_bin_add_coord(ib, gs->first, gs->last-gs->first+1);
				item_bin_write(ib, f);
			}
			if (boundary->country) {
				if (!coord_is_equal(*gs->first,*gs->last)) {
					struct item_bin *ib;
					if (!fu) {
						char *name=g_strdup_printf("country_%s_broken",boundary->iso2);
						fu=tempfile("",name,1);
						g_free(name);
					}
					ib=item_bin;
					item_bin_init(ib, type_selected_point);
					item_bin_add_coord(ib, gs->first, 1);
					item_bin_write(ib, fu);
					item_bin_init(ib, type_selected_point);
					item_bin_add_coord(ib, gs->last, 1);
					item_bin_write(ib, fu);
				}
			}
			sl=g_list_next(sl);
		}	
		ret=process_boundaries_insert(ret, boundary);
		l=g_list_next(l);
		if (f) 
			fclose(f);
		if (fu) {
			if (boundary->country)
				osm_warning("relation",item_bin_get_relationid(boundary->ib),0,"Broken country polygon '%s'\n",boundary->iso2);
			fclose(fu);
		}
		
	}
#if 0
	printf("hierarchy\n");
#endif
#if 0
	boundaries_list=g_list_sort(boundaries_list, boundary_bbox_compare);
	l=boundaries_list;
	while (l) {
		struct boundary *boundary=l->data;
		GList *l2,*ln;
		ln=l2=g_list_next(l);
		while (l2) {
			struct boundary *boundary2=l2->data;
			if (bbox_contains_bbox(&boundary2->r, &boundary->r)) {
				boundaries_list=g_list_remove(boundaries_list, boundary);
				boundary2->children=g_list_append(boundary2->children, boundary);
#if 0
				printf("found\n");
#endif
				break;
			}
			l2=g_list_next(l2);
		}
		l=ln;
	}
	dump_hierarchy(boundaries_list,"");
#if 0
	printf("hierarchy done\n");
	printf("test\n");
	test(boundaries_list);
#endif
#endif
	return ret;
}

GList *
process_boundaries(FILE *boundaries, FILE *ways)
{
	GList *boundaries_list;
	struct relations *relations=relations_new();

	boundaries_list=process_boundaries_setup(boundaries, relations);
	relations_process(relations, NULL, ways, NULL);
	return process_boundaries_finish(boundaries_list);
}
