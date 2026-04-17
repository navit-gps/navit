#ifndef NAVIT_NETWORK_H
#define NAVIT_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

struct map_download_info{
	char *url;
	char *name;
	char *path;
	char *xml;
	int downloading;
	int resume;
	FILE * stream;
	double dl_now;
	double dl_total;
};

typedef struct {
    char *name;
    long long size_val;
    char *size_str;
} MapData;

int progressCallBack(struct map_download_info *dl_info, double dltotal, double dlnow, double ultotal, double ulnow);
void add_to_listdata(char *name, long long size, GArray *maps_data, int pos); 
long long get_size(char *key, int* maps_count, MapData *maps_size);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void format_filesize(long long map_size, char *buffer);
void * download_map (struct gui_priv *this, struct widget *wm, struct map_download_info * dl_info);
void * download_map (void * data);
void * download_maps(void * data);
void * update_download_table();
#ifdef __cplusplus
}
#endif

#endif

