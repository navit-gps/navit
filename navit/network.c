#include "curl/curl.h"
#include <navit/debug.h>
#include "unistd.h"
#include "stddef.h"
#include "network.h"


int progressCallBack(struct map_download_info *dl_info, double dltotal, double dlnow, double 
ultotal, double ulnow) 
{ 
  dl_info->dl_now = (curl_off_t)dlnow;
  dl_info->dl_total = (curl_off_t)dltotal;
  return 0; 
  // FIXME : return non-0 to cancel the download
  // Could be use to trigger a cancel from the UI
} 


void * download_map(void * arguments)
{
  struct map_download_info * dl_info = arguments;
  dl_info->downloading = 1;
  CURL *handle;
  CURLcode res = CURLE_OK;
  fprintf(stderr, "url = %s\n", dl_info->url);

  handle = curl_easy_init();

  curl_easy_setopt(handle, CURLOPT_URL, dl_info->url);

  curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, dl_info);
  curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0); 
  curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progressCallBack); 

  FILE* file = fopen( "out.bin", "w");

  curl_easy_setopt( handle, CURLOPT_WRITEDATA, file) ;
  curl_easy_perform( handle );

  if(res != CURLE_OK)
      fprintf(stderr, "%s\n", curl_easy_strerror(res));

  curl_easy_cleanup(handle);

  dl_info->downloading = 0;
  return (int)res;;
}
