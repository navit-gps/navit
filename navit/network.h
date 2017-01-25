#ifndef NAVIT_NETWORK_H
#define NAVIT_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

struct map_download_info{
	char url[256];
	char *name;
	char *path;
	char *xml;
	int downloading;
	int resume;
	FILE * stream;
	double dl_now;
	double dl_total;
};

void * download_map (void * dl_info);
#ifdef __cplusplus
}
#endif

#endif

