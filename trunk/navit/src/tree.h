struct block_offset {
        unsigned short offset;
        unsigned short block;
};


int tree_compare_string(unsigned char *s1, unsigned char **s2);
int tree_compare_string_partial(unsigned char *s1, unsigned char **s2);
#if 0
int tree_search(struct file *file, unsigned char *search, int (*tree_func)(int, unsigned char *, unsigned char **, struct map_data *, void *), struct map_data *mdat, void *data2);
#endif
int tree_search_map(struct map_data *mdat, int map, char *ext, int (*tree_func)(int, int, unsigned char **, struct map_data *, void *), void *data);
int tree_search_hv_map(struct map_data *mdat, int map, unsigned int search1, unsigned int search2, int *result, struct map_data **mdat_result);
