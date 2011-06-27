struct cache_entry;
struct cache;
/* prototypes */
struct cache *cache_new(int id_size, int size);
void *cache_entry_new(struct cache *cache, void *id, int size);
void cache_entry_destroy(struct cache *cache, void *data);
void *cache_lookup(struct cache *cache, void *id);
void cache_insert(struct cache *cache, void *data);
void *cache_insert_new(struct cache *cache, void *id, int size);
void cache_flush(struct cache *cache, void *id);
void cache_dump(struct cache *cache);
void cache_flush_data(struct cache *cache, void *data);
/* end of prototypes */
