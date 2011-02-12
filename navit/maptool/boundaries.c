#include <stdio.h>
#include <string.h>
#include "maptool.h"

struct boundary {
	struct item_bin *ib;
	GList *segments,*sorted_segments;
	GList *children;
	struct rect r;
};

struct boundary_member {
	long long wayid;
	enum geom_poly_segment_type role;
	struct boundary *boundary;
};

static guint
boundary_member_hash(gconstpointer key)
{
	const struct boundary_member *memb=key;
	return (memb->wayid >> 32)^(memb->wayid & 0xffffffff);
}

static gboolean
boundary_member_equal(gconstpointer a, gconstpointer b)
{
	const struct boundary_member *memba=a;
	const struct boundary_member *membb=b;
	return (memba->wayid == membb->wayid);
}

GHashTable *member_hash;

static char *
osm_tag_name(struct item_bin *ib)
{
	char *tag=NULL;
	while ((tag=item_bin_get_attr(ib, attr_osm_tag, tag))) {
		if (!strncmp(tag,"name=",5))
			return tag+5;
	}
	return NULL;
}

static GList *
build_boundaries(FILE *boundaries)
{
	struct item_bin *ib;
	GList *boundaries_list=NULL;

	while ((ib=read_item(boundaries))) {
		char *member=NULL;
		struct boundary *boundary=g_new0(struct boundary, 1);
		while ((member=item_bin_get_attr(ib, attr_osm_member, member))) {
			long long wayid;
			int read=0;
			if (sscanf(member,"2:%Ld:%n",&wayid,&read) >= 1) {
				struct boundary_member *memb=g_new(struct boundary_member, 1);
				char *role=member+read;
				memb->wayid=wayid;
				memb->boundary=boundary;
				if (!strcmp(role,"outer"))
					memb->role=geom_poly_segment_type_way_outer;
				else if (!strcmp(role,"inner"))
					memb->role=geom_poly_segment_type_way_inner;
				else if (!strcmp(role,""))
					memb->role=geom_poly_segment_type_way_unknown;
				else {
					printf("Unknown role %s\n",role);
					memb->role=geom_poly_segment_type_none;
				}
				g_hash_table_insert(member_hash, memb, g_list_append(g_hash_table_lookup(member_hash, memb), memb));

			}
		}
		boundary->ib=item_bin_dup(ib);
		boundaries_list=g_list_append(boundaries_list, boundary);
	}
	return boundaries_list;
}

static void
find_matches(GList *l, struct coord *c)
{
	while (l) {
		struct boundary *boundary=l->data;
		if (bbox_contains_coord(&boundary->r, c)) {
			struct item_bin *ib=boundary->ib;
			if (geom_poly_segments_point_inside(boundary->sorted_segments,c)) 
				printf("%s,",osm_tag_name(ib));
			find_matches(boundary->children, c);
		}
		l=g_list_next(l);
	}
}

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
		find_matches(boundaries_list, c);
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
		printf("%s:%s\n",prefix,osm_tag_name(boundary->ib));
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

int
process_boundaries(FILE *boundaries, FILE *ways)
{
	struct item_bin *ib;
	GList *boundaries_list,*l,*sl,*l2,*ln;

	member_hash=g_hash_table_new_full(boundary_member_hash, boundary_member_equal, NULL, NULL);
	boundaries_list=build_boundaries(boundaries);
	while ((ib=read_item(ways))) {
		long long *wayid=item_bin_get_attr(ib, attr_osm_wayid, NULL);
		if (wayid) {
			GList *l=g_hash_table_lookup(member_hash, wayid);
			while (l) {
				struct boundary_member *memb=l->data;
				memb->boundary->segments=g_list_prepend(memb->boundary->segments,item_bin_to_poly_segment(ib, memb->role));

				l=g_list_next(l);
			}
		}
	}
	l=boundaries_list;
	while (l) {
		struct boundary *boundary=l->data;
		int first=1;
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
			sl=g_list_next(sl);
		}	
		l=g_list_next(l);
		
	}
	printf("hierarchy\n");
	boundaries_list=g_list_sort(boundaries_list, boundary_bbox_compare);
	l=boundaries_list;
	while (l) {
		struct boundary *boundary=l->data;
		ln=l2=g_list_next(l);
		while (l2) {
			struct boundary *boundary2=l2->data;
			if (bbox_contains_bbox(&boundary2->r, &boundary->r)) {
				boundaries_list=g_list_remove(boundaries_list, boundary);
				boundary2->children=g_list_append(boundary2->children, boundary);
				printf("found\n");
				break;
			}
			l2=g_list_next(l2);
		}
		l=ln;
	}
	printf("hierarchy done\n");
	dump_hierarchy(boundaries_list,"");
	printf("test\n");
	test(boundaries_list);
	return 1;
}
