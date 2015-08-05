#ifndef NAVIT_NETWORK_H
#define NAVIT_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

char * fetch_url_to_string(char * url);
char * post_to_string(char * url);
int download_icon (char *prefix, char *img_size, char *suffix);
#ifdef __cplusplus
}
#endif

#endif

