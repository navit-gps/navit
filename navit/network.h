#ifndef NAVIT_NETWORK_H
#define NAVIT_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

struct map_download_info{
	char url[256];
	int downloading;
	double dl_now;
	double dl_total;
};

void * download_map (void * dl_info);
#ifdef __cplusplus
}
#endif

#endif

