#ifndef NAVIT_MAP_H
#define NAVIT_MAP_H

struct map_priv;
struct attr;
#include "coord.h"
#include "point.h"
#include "layer.h"

struct map_selection {
	struct map_selection *next;
	union {
		struct coord_rect c_rect;
		struct point_rect p_rect;
	} u;
	int order[layer_end];		
};

struct map_methods {
	enum projection pro;
	char *charset;
	void 			(*map_destroy)(struct map_priv *priv);
	struct map_rect_priv *  (*map_rect_new)(struct map_priv *map, struct map_selection *sel);
	void			(*map_rect_destroy)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item_byid)(struct map_rect_priv *mr, int id_hi, int id_lo);
	struct map_search_priv *(*map_search_new)(struct map_priv *map, struct item *item, struct attr *search, int partial);
	void			(*map_search_destroy)(struct map_search_priv *ms);
	struct item *		(*map_search_get_item)(struct map_search_priv *ms);
};

static inline int
map_selection_contains_point(struct map_selection *sel, struct coord *c)
{
	struct map_selection *curr=sel;
        while (curr) {
                struct coord_rect *r=&curr->u.c_rect;
                if (c->x >= r->lu.x && c->x <= r->rl.x &&
                    c->y <= r->lu.y && c->y >= r->rl.y)
                        return 1;
                curr=curr->next;
        }
        return sel ? 0:1;
}

static inline int
map_selection_contains_polyline(struct map_selection *sel, struct coord *c, int count)
{
	int i,x_mi,x_ma,y_mi,y_ma;
	struct map_selection *curr;

	if (! sel)
		return 1;
	for (i = 0 ; i < count-1 ; i++) {
		x_mi=c[i].x;
		if (c[i+1].x < x_mi)
			x_mi=c[i+1].x;
		x_ma=c[i].x;
		if (c[i+1].x > x_ma)
			x_ma=c[i+1].x;
		y_mi=c[i].y;
		if (c[i+1].y < y_mi)
			y_mi=c[i+1].y;
		y_ma=c[i].y;
		if (c[i+1].y > y_ma)
			y_ma=c[i+1].y;
		curr=sel;
		while (curr) {
			struct coord_rect *sr=&curr->u.c_rect;
			if (x_mi <= sr->rl.x && x_ma >= sr->lu.x &&
			    y_ma >= sr->rl.y && y_mi <= sr->lu.y)
				return 1;
			curr=curr->next;
		}
	}
	return 0;
}

static inline int
map_selection_contains_rect(struct map_selection *sel, struct coord_rect *r)
{
	struct map_selection *curr;

	g_assert(r->lu.x <= r->rl.x);
	g_assert(r->lu.y >= r->rl.y);

	if (! sel)
		return 1;
	curr=sel;
	while (curr) {
		struct coord_rect *sr=&curr->u.c_rect;
		g_assert(sr->lu.x <= sr->rl.x);
		g_assert(sr->lu.y >= sr->rl.y);
		if (r->lu.x <= sr->rl.x && r->rl.x >= sr->lu.x &&
		    r->lu.y >= sr->rl.y && r->rl.y <= sr->lu.y)
			return 1;
		curr=curr->next;
	}
	return 0;
}

static inline int
map_selection_contains_polygon(struct map_selection *sel, struct coord *c, int count)
{
	struct coord_rect r;
	int i;

	if (! sel)
		return 1;
	if (! count)
		return 0;
	r.lu=c[0];
	r.rl=c[0];
	for (i = 1 ; i < count ; i++) {
		if (c[i].x < r.lu.x)
			r.lu.x=c[i].x;
		if (c[i].x > r.rl.x)
			r.rl.x=c[i].x;
		if (c[i].y < r.rl.y)
			r.rl.y=c[i].y;
		if (c[i].y > r.lu.y)
			r.lu.y=c[i].y;
	}
	return map_selection_contains_rect(sel, &r);
}

/* prototypes */
enum projection;
struct attr;
struct item;
struct map;
struct map_rect;
struct map_search;
struct map_selection;
struct map *map_new(const char *type, struct attr **attrs);
char *map_get_filename(struct map *this_);
char *map_get_type(struct map *this_);
int map_get_active(struct map *this_);
void map_set_active(struct map *this_, int active);
int map_requires_conversion(struct map *this_);
char *map_convert_string(struct map *this_, char *str);
void map_convert_free(char *str);
enum projection map_projection(struct map *this_);
void map_set_projection(struct map *this_, enum projection pro);
void map_destroy(struct map *m);
struct map_rect *map_rect_new(struct map *m, struct map_selection *sel);
struct item *map_rect_get_item(struct map_rect *mr);
struct item *map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo);
void map_rect_destroy(struct map_rect *mr);
struct map_search *map_search_new(struct map *m, struct item *item, struct attr *search_attr, int partial);
struct item *map_search_get_item(struct map_search *this_);
void map_search_destroy(struct map_search *this_);
struct map_selection *map_selection_dup(struct map_selection *sel);
void map_selection_destroy(struct map_selection *sel);
/* end of prototypes */

#endif
