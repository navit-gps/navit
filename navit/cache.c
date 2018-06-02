#include "glib_slice.h"
#ifdef DEBUG_CACHE
#include <stdio.h>
#endif
#include <string.h>
#include "debug.h"
#include "cache.h"

struct cache_entry {
    int usage;
    unsigned int size;
    struct cache_entry_list *where;
    struct cache_entry *next;
    struct cache_entry *prev;
    int id[0];
};

struct cache_entry_list {
    struct cache_entry *first, *last;
    int size;
};

struct cache {
    struct cache_entry_list t1,b1,t2,b2,*insert;
    int size,id_size,entry_size;
    int t1_target;
    unsigned int misses;
    unsigned int hits;
    GHashTable *hash;
};

static void cache_entry_dump(struct cache *cache, struct cache_entry *entry) {
    int i,size;
    dbg(lvl_debug,"Usage: %d size %d",entry->usage, entry->size);
    if (cache)
        size=cache->id_size;
    else
        size=5;
    for (i = 0 ; i < size ; i++) {
        dbg(lvl_debug,"0x%x", entry->id[i]);
    }
}

static void cache_list_dump(char *str, struct cache *cache, struct cache_entry_list *list) {
    struct cache_entry *first=list->first;
    dbg(lvl_debug,"dump %s %d",str, list->size);
    while (first) {
        cache_entry_dump(cache, first);
        first=first->next;
    }
}

static guint cache_hash4(gconstpointer key) {
    int *id=(int *)key;
    return id[0];
}

static guint cache_hash20(gconstpointer key) {
    int *id=(int *)key;
    return id[0]^id[1]^id[2]^id[3]^id[4];
}

static gboolean cache_equal4(gconstpointer a, gconstpointer b) {
    int *ida=(int *)a;
    int *idb=(int *)b;

    return ida[0] == idb[0];
}

static gboolean cache_equal20(gconstpointer a, gconstpointer b) {
    int *ida=(int *)a;
    int *idb=(int *)b;

    return(ida[0] == idb[0] &&
           ida[1] == idb[1] &&
           ida[2] == idb[2] &&
           ida[3] == idb[3] &&
           ida[4] == idb[4]);
}

struct cache *
cache_new(int id_size, int size) {
    struct cache *cache=g_new0(struct cache, 1);

    cache->id_size=id_size/4;
    cache->entry_size=cache->id_size*sizeof(int)+sizeof(struct cache_entry);
    cache->size=size;
    switch (id_size) {
    case 4:
        cache->hash=g_hash_table_new(cache_hash4, cache_equal4);
        break;
    case 20:
        cache->hash=g_hash_table_new(cache_hash20, cache_equal20);
        break;
    default:
        dbg(lvl_error,"cache with id_size of %d not supported", id_size);
        g_free(cache);
        cache=NULL;
    }
    return cache;
}

void cache_resize(struct cache *cache, int size) {
    cache->size=size;
}

static void cache_insert_mru(struct cache *cache, struct cache_entry_list *list, struct cache_entry *entry) {
    entry->prev=NULL;
    entry->next=list->first;
    entry->where=list;
    if (entry->next)
        entry->next->prev=entry;
    list->first=entry;
    if (! list->last)
        list->last=entry;
    list->size+=entry->size;
    if (cache)
        g_hash_table_insert(cache->hash, (gpointer)entry->id, entry);
}

static void cache_remove_from_list(struct cache_entry_list *list, struct cache_entry *entry) {
    if (entry->prev)
        entry->prev->next=entry->next;
    else
        list->first=entry->next;
    if (entry->next)
        entry->next->prev=entry->prev;
    else
        list->last=entry->prev;
    list->size-=entry->size;
}

static void cache_remove(struct cache *cache, struct cache_entry *entry) {
    dbg(lvl_debug,"remove 0x%x 0x%x 0x%x 0x%x 0x%x", entry->id[0], entry->id[1], entry->id[2], entry->id[3], entry->id[4]);
    g_hash_table_remove(cache->hash, (gpointer)(entry->id));
    g_slice_free1(entry->size, entry);
}

static struct cache_entry *cache_remove_lru_helper(struct cache_entry_list *list) {
    struct cache_entry *last=list->last;
    if (! last)
        return NULL;
    list->last=last->prev;
    if (last->prev)
        last->prev->next=NULL;
    else
        list->first=NULL;
    list->size-=last->size;
    return last;
}

static struct cache_entry *cache_remove_lru(struct cache *cache, struct cache_entry_list *list) {
    struct cache_entry *last;
    int seen=0;
    while (list->last && list->last->usage && seen < list->size) {
        last=cache_remove_lru_helper(list);
        cache_insert_mru(NULL, list, last);
        seen+=last->size;
    }
    last=list->last;
    if (! last || last->usage || seen >= list->size)
        return NULL;
    dbg(lvl_debug,"removing %d", last->id[0]);
    cache_remove_lru_helper(list);
    if (cache) {
        cache_remove(cache, last);
        return NULL;
    }
    return last;
}

void *cache_entry_new(struct cache *cache, void *id, int size) {
    struct cache_entry *ret;
    size+=cache->entry_size;
    cache->misses+=size;
    ret=(struct cache_entry *)g_slice_alloc0(size);
    ret->size=size;
    ret->usage=1;
    memcpy(ret->id, id, cache->id_size*sizeof(int));
    return &ret->id[cache->id_size];
}

void cache_entry_destroy(struct cache *cache, void *data) {
    struct cache_entry *entry=(struct cache_entry *)((char *)data-cache->entry_size);
    dbg(lvl_debug,"destroy 0x%x 0x%x 0x%x 0x%x 0x%x", entry->id[0], entry->id[1], entry->id[2], entry->id[3], entry->id[4]);
    entry->usage--;
}

static struct cache_entry *cache_trim(struct cache *cache, struct cache_entry *entry) {
    struct cache_entry *new_entry;
    dbg(lvl_debug,"trim 0x%x 0x%x 0x%x 0x%x 0x%x", entry->id[0], entry->id[1], entry->id[2], entry->id[3], entry->id[4]);
    dbg(lvl_debug,"Trim %x from %d -> %d", entry->id[0], entry->size, cache->size);
    if ( cache->entry_size < entry->size ) {
        g_hash_table_remove(cache->hash, (gpointer)(entry->id));

        new_entry = g_slice_alloc0(cache->entry_size);
        memcpy(new_entry, entry, cache->entry_size);
        g_slice_free1( entry->size, entry);
        new_entry->size = cache->entry_size;

        g_hash_table_insert(cache->hash, (gpointer)new_entry->id, new_entry);
    } else {
        new_entry = entry;
    }

    return new_entry;
}

static struct cache_entry *cache_move(struct cache *cache, struct cache_entry_list *old, struct cache_entry_list *new) {
    struct cache_entry *entry;
    entry=cache_remove_lru(NULL, old);
    if (! entry)
        return NULL;
    entry=cache_trim(cache, entry);
    cache_insert_mru(NULL, new, entry);
    return entry;
}

static int cache_replace(struct cache *cache) {
    if (cache->t1.size >= MAX(1,cache->t1_target)) {
        dbg(lvl_debug,"replace 12");
        if (!cache_move(cache, &cache->t1, &cache->b1))
            cache_move(cache, &cache->t2, &cache->b2);
    } else {
        dbg(lvl_debug,"replace t2");
        if (!cache_move(cache, &cache->t2, &cache->b2))
            cache_move(cache, &cache->t1, &cache->b1);
    }
#if 0
    if (! entry) {
        cache_dump(cache);
        exit(0);
    }
#endif
    return 1;
}

void cache_flush(struct cache *cache, void *id) {
    struct cache_entry *entry=g_hash_table_lookup(cache->hash, id);
    if (entry) {
        cache_remove_from_list(entry->where, entry);
        cache_remove(cache, entry);
    }
}

void cache_flush_data(struct cache *cache, void *data) {
    struct cache_entry *entry=(struct cache_entry *)((char *)data-cache->entry_size);
    if (entry) {
        cache_remove_from_list(entry->where, entry);
        cache_remove(cache, entry);
    }
}


void *cache_lookup(struct cache *cache, void *id) {
    struct cache_entry *entry;

    dbg(lvl_debug,"get %d", ((int *)id)[0]);
    entry=g_hash_table_lookup(cache->hash, id);
    if (entry == NULL) {
        cache->insert=&cache->t1;
#ifdef DEBUG_CACHE
        fprintf(stderr,"-");
#endif
        dbg(lvl_debug,"not in cache");
        return NULL;
    }
    dbg(lvl_debug,"found 0x%x 0x%x 0x%x 0x%x 0x%x", entry->id[0], entry->id[1], entry->id[2], entry->id[3], entry->id[4]);
    if (entry->where == &cache->t1 || entry->where == &cache->t2) {
        cache->hits+=entry->size;
#ifdef DEBUG_CACHE
        if (entry->where == &cache->t1)
            fprintf(stderr,"h");
        else
            fprintf(stderr,"H");
#endif
        dbg(lvl_debug,"in cache %s", entry->where == &cache->t1 ? "T1" : "T2");
        cache_remove_from_list(entry->where, entry);
        cache_insert_mru(NULL, &cache->t2, entry);
        entry->usage++;
        return &entry->id[cache->id_size];
    } else {
        if (entry->where == &cache->b1) {
#ifdef DEBUG_CACHE
            fprintf(stderr,"m");
#endif
            dbg(lvl_debug,"in phantom cache B1");
            cache->t1_target=MIN(cache->t1_target+MAX(cache->b2.size/cache->b1.size, 1),cache->size);
            cache_remove_from_list(&cache->b1, entry);
        } else if (entry->where == &cache->b2) {
#ifdef DEBUG_CACHE
            fprintf(stderr,"M");
#endif
            dbg(lvl_debug,"in phantom cache B2");
            cache->t1_target=MAX(cache->t1_target-MAX(cache->b1.size/cache->b2.size, 1),0);
            cache_remove_from_list(&cache->b2, entry);
        } else {
            dbg(lvl_error,"**ERROR** invalid where");
        }
        cache_replace(cache);
        cache_remove(cache, entry);
        cache->insert=&cache->t2;
        return NULL;
    }
}

void cache_insert(struct cache *cache, void *data) {
    struct cache_entry *entry=(struct cache_entry *)((char *)data-cache->entry_size);
    dbg(lvl_debug,"insert 0x%x 0x%x 0x%x 0x%x 0x%x", entry->id[0], entry->id[1], entry->id[2], entry->id[3], entry->id[4]);
    if (cache->insert == &cache->t1) {
        if (cache->t1.size + cache->b1.size >= cache->size) {
            if (cache->t1.size < cache->size) {
                cache_remove_lru(cache, &cache->b1);
                cache_replace(cache);
            } else {
                cache_remove_lru(cache, &cache->t1);
            }
        } else {
            if (cache->t1.size + cache->t2.size + cache->b1.size + cache->b2.size >= cache->size) {
                if (cache->t1.size + cache->t2.size + cache->b1.size + cache->b2.size >= 2*cache->size)
                    cache_remove_lru(cache, &cache->b2);
                cache_replace(cache);
            }
        }
    }
    cache_insert_mru(cache, cache->insert, entry);
}

void *cache_insert_new(struct cache *cache, void *id, int size) {
    void *data=cache_entry_new(cache, id, size);
    cache_insert(cache, data);
    return data;
}

static void cache_stats(struct cache *cache) {
    dbg(lvl_debug,"hits %d misses %d hitratio %d size %d entry_size %d id_size %d T1 target %d", cache->hits, cache->misses,
        cache->hits*100/(cache->hits+cache->misses), cache->size, cache->entry_size, cache->id_size, cache->t1_target);
    dbg(lvl_debug,"T1:%d B1:%d T2:%d B2:%d", cache->t1.size, cache->b1.size, cache->t2.size, cache->b2.size);
    cache->hits=0;
    cache->misses=0;
}

void cache_dump(struct cache *cache) {
    cache_stats(cache);
    cache_list_dump("T1", cache, &cache->t1);
    cache_list_dump("B1", cache, &cache->b1);
    cache_list_dump("T2", cache, &cache->t2);
    cache_list_dump("B2", cache, &cache->b2);
    dbg(lvl_debug,"dump end");
}

