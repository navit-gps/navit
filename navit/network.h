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

void * download_map (struct gui_priv *this, struct widget *wm, struct map_download_info * dl_info);
void * download_map2 (void * data);
#ifdef __cplusplus
}
#endif

#endif

