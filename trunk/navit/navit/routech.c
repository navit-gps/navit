#include <glib.h>
#include <stdio.h>
#include "item.h"
#include "coord.h"
#include "navit.h"
#include "transform.h"
#include "profile.h"
#include "mapset.h"
#include "map.h"

FILE *routefile;

struct ch_edge {
	int flags;
	int weight;
	struct item_id target,middle;
};

struct routech_search {
	struct pq *pq;
	GHashTable *hash;
	int finished;
	int dir;
	unsigned int upper;
	struct item_id *via;
};


struct pq_element {
	struct item_id *node_id;
	struct item_id *parent_node_id;
	int stalled;
	int key;
	int heap_element;
};

struct pq_heap_element {
	int key;
	int element;
};

struct pq {
	int capacity;
	int size;
	int step;
	int elements_capacity;
	int elements_size;
	int elements_step;
	struct pq_element *elements;
	struct pq_heap_element *heap_elements;
};

static struct pq *
pq_new(void)
{
	struct pq *ret=g_new(struct pq, 1);
	ret->step=10;
	ret->capacity=0;
	ret->size=1;
	ret->elements_step=10;
	ret->elements_capacity=0;
	ret->elements_size=1;
	ret->elements=NULL;
	ret->heap_elements=NULL;
	return ret;
}

static int
pq_insert(struct pq *pq, int key, struct item_id *node_id)
{
	int element,i;
	if (pq->size >= pq->capacity) {
		pq->capacity += pq->step;
		pq->heap_elements=g_renew(struct pq_heap_element, pq->heap_elements, pq->capacity);
	}
	if (pq->elements_size >= pq->elements_capacity) {
		pq->elements_capacity += pq->elements_step;
		pq->elements=g_renew(struct pq_element, pq->elements, pq->elements_capacity);
	}
	element=pq->elements_size++;
	pq->elements[element].node_id=node_id;
	i=pq->size++;
	while (i > 1 && pq->heap_elements[i/2].key > key) {
		pq->heap_elements[i]=pq->heap_elements[i/2];
		pq->elements[pq->heap_elements[i].element].heap_element=i;
		i/=2;
	}
	pq->heap_elements[i].key=key;
	pq->heap_elements[i].element=element;
	pq->elements[element].heap_element=i;
	pq->elements[element].key=key;
	return element;
}

static int
pq_get_key(struct pq *pq, int element, int *key)
{
	*key=pq->elements[element].key;
	return 1;
}

static void
pq_set_parent(struct pq *pq, int element, struct item_id *node_id, int stalled)
{
	pq->elements[element].parent_node_id=node_id;
	pq->elements[element].stalled=stalled;
}

static struct item_id *
pq_get_parent_node_id(struct pq *pq, int element)
{
	return pq->elements[element].parent_node_id;
}

static void
pq_set_stalled(struct pq *pq, int element, int stalled)
{
	pq->elements[element].stalled=stalled;
}

static int
pq_get_stalled(struct pq *pq, int element)
{
	return pq->elements[element].stalled;
}

static int
pq_is_deleted(struct pq *pq, int element)
{
	return (pq->elements[element].heap_element == 0);
}

static int
pq_min(struct pq *pq)
{
	return (pq->heap_elements[1].key);
}

static void
pq_decrease_key(struct pq *pq, int element, int key)
{
	int i=pq->elements[element].heap_element;
	while (i > 1 && pq->heap_elements[i/2].key > key) {
		pq->heap_elements[i]=pq->heap_elements[i/2];
		pq->elements[pq->heap_elements[i].element].heap_element=i;
		i/=2;
	}
	pq->heap_elements[i].element=element;
	pq->heap_elements[i].key=key;
	pq->elements[element].heap_element=i;
	pq->elements[element].key=key;
}

static int
pq_delete_min(struct pq *pq, struct item_id **node_id, int *key, int *element)
{
	struct pq_heap_element min, last;
	int i=1,j;
	if (pq->size <= 1)
		return 0;
	min=pq->heap_elements[1];
	if (node_id)
		*node_id=pq->elements[min.element].node_id;
	if (key)
		*key=min.key;
	if (element)
		*element=min.element;
	pq->elements[min.element].heap_element=0;
	min.element=0;
	last=pq->heap_elements[--pq->size];
	while (i <= pq->size / 2) {
		j=2*i;
		if (j < pq->size && pq->heap_elements[j].key > pq->heap_elements[j+1].key)
			j++;
		if (pq->heap_elements[j].key >= last.key)
			break;
		pq->heap_elements[i]=pq->heap_elements[j];
		pq->elements[pq->heap_elements[i].element].heap_element=i;
		i=j;
	}
	pq->heap_elements[i]=last;
	pq->elements[last.element].heap_element=i;
	return 1;
}

static int
pq_is_empty(struct pq *pq)
{
	return pq->size <= 1;
}

static void
pq_check(struct pq *pq)
{
	int i;
	for (i = 2 ; i < pq->size ; i++) {
		if (pq->heap_elements[i/2].key > pq->heap_elements[i].key) {
			printf("%d vs %d\n", pq->heap_elements[i/2].key, pq->heap_elements[i].key);
			return;
		}
	}
	for (i = 1 ; i < pq->size ; i++) {
		if (i != pq->elements[pq->heap_elements[i].element].heap_element) {
			printf("Error: heap_element %d points to element %d, but that points to %d\n",i,pq->heap_elements[i].element,pq->elements[pq->heap_elements[i].element].heap_element);
		}
	}
}

struct routech_search *
routech_search_new(int dir)
{
	struct routech_search *ret=g_new0(struct routech_search, 1);
	ret->pq=pq_new();
	ret->hash=g_hash_table_new_full(item_id_hash, item_id_equal, g_free, NULL);
	ret->upper=UINT_MAX;
	ret->dir=dir;

	return ret;
}

static int
routech_insert_node(struct routech_search *search, struct item_id **id, int val)
{
	struct item_id *ret;
	int e;
	if (g_hash_table_lookup_extended(search->hash, *id, (gpointer)&ret, (gpointer)&e)) {
		int oldval;
		pq_get_key(search->pq, e, &oldval);
		// printf("Node = %d\n",node);
		if (oldval > val) {
			pq_decrease_key(search->pq, e, val);
			*id=ret;
			return e;
		}
		return 0;
	}
	ret=g_new(struct item_id, 1);
	*ret=**id;
	e=pq_insert(search->pq, val, ret);
	g_hash_table_insert(search->hash, ret, GINT_TO_POINTER(e));
	*id=ret;
	return e;
}


static int
routech_find_nearest(struct mapset *ms, struct coord *c, struct item_id *id, struct map **map_ret)
{
	int dst=50;
	int dstsq=dst*dst;
	int ret=0;
	struct map_selection sel;
	struct map_rect *mr;
	struct mapset_handle *msh;
	struct map *map;
	struct item *item;
	sel.next=NULL;
	sel.order=18;
	sel.range.min=type_ch_node;
	sel.range.max=type_ch_node;
	sel.u.c_rect.lu.x=c->x-dst;
	sel.u.c_rect.lu.y=c->y+dst;
	sel.u.c_rect.rl.x=c->x+dst;
	sel.u.c_rect.rl.y=c->y-dst;
	printf("0x%x,0x%x-0x%x,0x%x\n",sel.u.c_rect.lu.x,sel.u.c_rect.lu.y,sel.u.c_rect.rl.x,sel.u.c_rect.rl.y);
	msh=mapset_open(ms);
	while ((map=mapset_next(msh, 1))) {
		mr=map_rect_new(map, &sel);
		if (!mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			struct coord cn;
			if (item->type == type_ch_node && item_coord_get(item, &cn, 1)) {
				int dist=transform_distance_sq(&cn, c);
				if (dist < dstsq) {	
					dstsq=dist;
					id->id_hi=item->id_hi;
					id->id_lo=item->id_lo;
					*map_ret=map;
					ret=1;
				}
			}
		}
		map_rect_destroy(mr);
	}
	mapset_close(msh);
	dbg_assert(ret==1);
	return ret;
}

static int
routech_edge_valid(struct ch_edge *edge, int dir)
{
	if (edge->flags & (1 << dir))
		return 1;
	return 0;
}

static void
routech_stall(struct map_rect *mr, struct routech_search *curr, struct item_id *id, int key)
{
	struct stall_element {
		struct item_id id;
		int key;
	} *se;
	GList *list=NULL;
	struct item *item;
	struct attr edge_attr;

	int index=GPOINTER_TO_INT(g_hash_table_lookup(curr->hash, id));
	pq_set_stalled(curr->pq, index, key);
	se=g_new(struct stall_element, 1);
	se->id=*id;
	se->key=key;
	list=g_list_append(list, se);
	while (list) {
		se=list->data;
		key=se->key;
		item=map_rect_get_item_byid(mr, se->id.id_hi, se->id.id_lo);
		while (item_attr_get(item, attr_ch_edge, &edge_attr)) {
			struct ch_edge *edge=edge_attr.u.data;
			if (routech_edge_valid(edge, curr->dir)) {
				int index=GPOINTER_TO_INT(g_hash_table_lookup(curr->hash, &edge->target));
				if (index) {
					int newkey=key+edge->weight;
					int target_key;
					pq_get_key(curr->pq, index, &target_key);
					if (newkey < target_key) {
						if (!pq_get_stalled(curr->pq, index)) {
							pq_set_stalled(curr->pq, index, newkey);
							se=g_new(struct stall_element, 1);
							se->id=edge->target;
							se->key=newkey;
							list=g_list_append(list, se);
						}	
					}
				}
			}
		}
		list=g_list_remove(list, se);
		g_free(se);
	}
}

static void
routech_relax(struct map_rect **mr, struct routech_search *curr, struct routech_search *opposite)
{
	int val,element;
	struct item_id *id;
	struct item *item;
	struct attr edge_attr;
	
	if (!pq_delete_min(curr->pq, &id, &val, &element)) {
		return;
	}
	pq_check(curr->pq);
	int opposite_element=GPOINTER_TO_INT(g_hash_table_lookup(opposite->hash, id));
	if (opposite_element && pq_is_deleted(opposite->pq, opposite_element)) {
		int opposite_val;
		pq_get_key(opposite->pq, opposite_element, &opposite_val);
		if (val+opposite_val < curr->upper) {
			curr->upper=opposite->upper=val+opposite_val;
			printf("%d path found: 0x%x,0x%x ub = %d\n",curr->dir,id->id_hi,id->id_lo,curr->upper);
			curr->via=opposite->via=id;
		}
	}
	if (pq_get_stalled(curr->pq, element)) 
		return;
	item=map_rect_get_item_byid(mr[0], id->id_hi, id->id_lo);
	while (item_attr_get(item, attr_ch_edge, &edge_attr)) {
		struct ch_edge *edge=edge_attr.u.data;
		struct item_id *target_id=&edge->target;
		if (routech_edge_valid(edge, curr->dir)) {
			int index=GPOINTER_TO_INT(g_hash_table_lookup(curr->hash, target_id));
			if (index && routech_edge_valid(edge, 1-curr->dir)) {
				int newkey,stallkey;
				stallkey=pq_get_stalled(curr->pq, index);
				if (stallkey)
					newkey=stallkey;
				else
					pq_get_key(curr->pq, index, &newkey);
				newkey+=edge->weight;
				if (newkey < val) {
					routech_stall(mr[1], curr, id, newkey);
					return;
				}
			}
			int element=routech_insert_node(curr, &target_id, edge->weight+val);
			if (element) {
				pq_set_parent(curr->pq, element, id, 0);
			}
		}
	}
}

void
routech_print_coord(struct coord *c, FILE *out)
{
	int x=c->x;
	int y=c->y;
	char *sx="";
	char *sy="";
	if (x < 0) {
		sx="-";
		x=-x;
	}
	if (y < 0) {
		sy="-";
		y=-y;
	}
	fprintf(out,"%s0x%x %s0x%x\n",sx,x,sy,y);
}

void
routech_resolve_route(struct map_rect *mr, struct item_id *id, int flags, int dir)
{
        int i,count,max=16384;
	int rev=0;
	if (!(flags & 8) == dir)
		rev=1;
        struct coord ca[max];
	struct item *item=map_rect_get_item_byid(mr, id->id_hi, id->id_lo);
	dbg_assert(item->type >= type_line && item->type < type_area);
	item->type=type_street_route;
	
        count=item_coord_get(item, ca, max);
        item_dump_attr(item, item->map, routefile);
	fprintf(routefile,"debug=\"flags=%d dir=%d rev=%d\"",flags,dir,rev);
        fprintf(routefile,"\n");
	if (rev) {
		for (i = count-1 ; i >= 0 ; i--)
			routech_print_coord(&ca[i], routefile);
	} else {
		for (i = 0 ; i < count ; i++)
			routech_print_coord(&ca[i], routefile);
	}
}

int
routech_find_edge(struct map_rect *mr, struct item_id *from, struct item_id *to, struct item_id *middle)
{
	struct item *item=map_rect_get_item_byid(mr, from->id_hi, from->id_lo);
	struct attr edge_attr;
	dbg_assert(item->type == type_ch_node);
	dbg(1,"type %s\n",item_to_name(item->type));
	dbg(1,"segment item=%p\n",item);
	while (item_attr_get(item, attr_ch_edge, &edge_attr)) {
		struct ch_edge *edge=edge_attr.u.data;
		dbg(1,"flags=%d\n",edge->flags);
		if (edge->target.id_hi == to->id_hi && edge->target.id_lo == to->id_lo) {
			*middle=edge->middle;
			return edge->flags;
		}
	}
	dbg(0,"** not found\n");
	return 0;
}

void
routech_resolve(struct map_rect *mr, struct item_id *from, struct item_id *to, int dir)
{
	struct item_id middle_node;
	int res;
	if (dir) 
		res=routech_find_edge(mr, to, from, &middle_node);
	else
		res=routech_find_edge(mr, from, to, &middle_node);
	dbg(1,"res=%d\n",res);
	if (res & 4) {
		routech_resolve(mr, from, &middle_node, 1);
		routech_resolve(mr, &middle_node, to, 0);
	} else 
		routech_resolve_route(mr, &middle_node, res, dir);
}

void
routech_find_path(struct map_rect *mr, struct routech_search *search)
{
	struct item_id *curr_node=search->via;
	GList *i,*n,*list=NULL;
	dbg(1,"node %p\n",curr_node);
	for (;;) {
		int element=GPOINTER_TO_INT(g_hash_table_lookup(search->hash, curr_node));
		struct item_id *next_node=pq_get_parent_node_id(search->pq,element);
		if (search->dir) 
			list=g_list_append(list, curr_node);
		else
			list=g_list_prepend(list, curr_node);
		dbg(1,"element %d\n",element);
		dbg(1,"next node %p\n",next_node);
		if (!next_node)
			break;
		curr_node=next_node;
	}
	i=list;
	while (i && (n=g_list_next(i))) {
		routech_resolve(mr, i->data, n->data, search->dir);
		i=n;
	}
}

void
routech_test(struct navit *navit)
{
	struct attr mapset;
	struct coord src={0x3fd661,0x727146};
	struct coord dst={0xfff07fc2,0x4754c9};
	struct item_id id[2],*id_ptr;
	struct routech_search *search[2],*curr,*opposite;
	struct map *map[2];
	struct map_rect *mr[2];
	int element;
	int k;

	navit_get_attr(navit, attr_mapset, &mapset, NULL);
	routech_find_nearest(mapset.u.mapset, &src, &id[0], &map[0]);
	routech_find_nearest(mapset.u.mapset, &dst, &id[1], &map[1]);
	for (k = 0 ; k < 2 ; k++) {
	profile(0,"start\n");
	search[0]=routech_search_new(0);
	search[1]=routech_search_new(1);
	printf("Start 0x%x,0x%x\n",id[0].id_hi,id[0].id_lo);
	printf("End 0x%x,0x%x\n",id[1].id_hi,id[1].id_lo);
	id_ptr=&id[0];
	element=routech_insert_node(search[0], &id_ptr, 0);
	pq_set_parent(search[0]->pq, element, NULL, 0);

	id_ptr=&id[1];
	element=routech_insert_node(search[1], &id_ptr, 0);
	pq_set_parent(search[1]->pq, element, NULL, 0);

	mr[0]=map_rect_new(map[0], NULL);
	mr[1]=map_rect_new(map[0], NULL);
	int search_id=0;
	int i;
	for (i=0 ; i < 5000 ; i++) {
		if (pq_is_empty(search[0]->pq) && pq_is_empty(search[1]->pq)) 
			break;
		if (!pq_is_empty(search[1-search_id]->pq)) {
			search_id=1-search_id;
		}
		if (search[0]->finished)
			search_id=1;
		if (search[1]->finished)
			search_id=0;
		curr=search[search_id];
		opposite=search[1-search_id];
		if (pq_is_empty(curr->pq)) {
			dbg(0,"empty\n");
			break;
		}
		routech_relax(mr, curr, opposite);
		if (pq_min(curr->pq) > curr->upper) {
			dbg(0,"min %d upper %d\n",pq_min(curr->pq), curr->upper);
			curr->finished=1;
		}
		if (curr->finished && opposite->finished) {
			dbg(0,"finished\n");
			break;
		}
	}
	
	printf("finished %d\n",search[0]->upper);
	profile(0,"finished\n");
	}
	routefile=fopen("route.txt","w");
	routech_find_path(mr[0], search[0]);
	routech_find_path(mr[0], search[1]);
	fclose(routefile);
        printf("Heap size %d vs %d\n",search[0]->pq->size,search[1]->pq->size);
        printf("Element size %d vs %d\n",search[0]->pq->elements_size,search[1]->pq->elements_size);

}
