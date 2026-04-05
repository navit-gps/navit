#include "curl/curl.h"
#include <navit/debug.h>
#include <glib.h>
#include "unistd.h"
#include "stddef.h"
#include <navit/coord.h>
#include <navit/color.h>
#include <navit/item.h>
#include <navit/point.h>
#include "gui/internal/gui_internal.h"
#include "gui/internal/gui_internal_map_downloader.h"
#include "gui/internal/gui_internal_widget.h"
#include "gui/internal/gui_internal_priv.h"
#include "network.h"


#include "sys/stat.h"


/**
 * Curl callback to provide download updates
 *
 * @param dl_info a pointer to a map_download_info where the download updates will be stored
 * @param dltotal number of total bytes to download, required by libcurl
 * @param dlnow number of bytes currently downloaded, required by libcurl
 * @param ultotal number of total bytes to upload, required by libcurl
 * @param ulnow number of bytes currently downloaded, required by libcurl
 *
 * @returns Returns 0 by default, until we implement 'cancel'
 */

int progressCallBack(struct map_download_info *dl_info, double dltotal, double dlnow, double ultotal, double ulnow) {
    dl_info->dl_now = (curl_off_t)dlnow;
    dl_info->dl_total = (curl_off_t)dltotal;
    return 0;
    // FIXME : return non-0 to cancel the download
    // Could be use to trigger a cancel from the UI
}


/**
 * Download a map using libcurl
 *
 * @param arguments a void * that we can cast to a map_download_info * as this function is intended to be run
 * 		  within a thread
 *
 * @returns Returns curl's result
 */

void * download_map(struct gui_priv *this, struct widget *wm, struct map_download_info * dl_info) {
    CURL *handle;
    FILE* file;
    int current_size = 0;

    //gui_internal_map_downloader(this, wm, dl_info);
    dl_info->downloading = 1;
    CURLcode res = CURLE_OK;
    dbg(lvl_debug, "downloading %s to %s\n", dl_info->url, dl_info->path);

#ifndef WINDOWS
    struct stat st;
    stat(dl_info->path, &st);
    current_size = st.st_size;
#else
    // Implement resuming on windows via https://msdn.microsoft.com/en-us/library/windows/desktop/aa364955(v=vs.85).aspx
#endif

    handle = curl_easy_init();
    // here i have to make the switch
    curl_easy_setopt(handle, CURLOPT_URL, dl_info->url);

    if (current_size>0) {
        dbg(lvl_debug, "Current size : %i, will resume\n", current_size);
        curl_easy_setopt(handle, CURLOPT_RESUME_FROM, current_size);
        dl_info->resume = 1;
    } else {
        dbg(lvl_debug, "Downloading from scratch\n");
    }

    // Map server might redirect to a mirror
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, dl_info);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progressCallBack);

    file = fopen( dl_info->path, current_size > 0 ? "ab":"wb");
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, file) ;
    res = curl_easy_perform( handle );
    fclose(file);

    if(res != CURLE_OK)
        dbg(lvl_error, "%s\n", curl_easy_strerror(res));

    curl_easy_cleanup(handle);


    dbg(lvl_debug, "download complete, creating XML\n");

    file = fopen( dl_info->xml, "w");
    if(file == NULL) {
        dbg(lvl_error, "Could not open %s for writing", dl_info->xml);
        return NULL;
    }

    fprintf(file, "<map type=\"binfile\" data=\"$NAVIT_SHAREDIR/maps/%s.bin\" />", dl_info->name);
    fclose(file);

    dl_info->downloading = 0;
    return NULL; //(int)res;
}

void * download_map2(void *data) {
    struct gui_download_data *dl_data = data;
    struct gui_priv *this = dl_data->this;
    struct widget *wm = dl_data->wm;
    struct map_download_info * dl_info = dl_data->data;
    CURL *handle;
    FILE* file;
    int current_size = 0;

    //gui_internal_map_downloader(this, wm, dl_info);
    dl_info->downloading = 1;
    CURLcode res = CURLE_OK;
    dbg(lvl_debug, "downloading %s to %s\n", dl_info->url, dl_info->path);

#ifndef WINDOWS
    struct stat st;
    stat(dl_info->path, &st);
    current_size = st.st_size;
#else
    // Implement resuming on windows via https://msdn.microsoft.com/en-us/library/windows/desktop/aa364955(v=vs.85).aspx
#endif

    handle = curl_easy_init();
    // here i have to make the switch
    curl_easy_setopt(handle, CURLOPT_URL, dl_info->url);

    if (current_size>0) {
        dbg(lvl_debug, "Current size : %i, will resume\n", current_size);
        curl_easy_setopt(handle, CURLOPT_RESUME_FROM, current_size);
        dl_info->resume = 1;
    } else {
        dbg(lvl_debug, "Downloading from scratch\n");
    }

    // Map server might redirect to a mirror
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, dl_info);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progressCallBack);

    file = fopen( dl_info->path, current_size > 0 ? "ab":"wb");
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, file) ;
    res = curl_easy_perform( handle );
    fclose(file);

    if(res != CURLE_OK)
        dbg(lvl_error, "%s\n", curl_easy_strerror(res));

    curl_easy_cleanup(handle);


    dbg(lvl_debug, "download complete, creating XML\n");

    file = fopen( dl_info->xml, "w");
    if(file == NULL) {
        dbg(lvl_error, "Could not open %s for writing", dl_info->xml);
        return NULL;
    }

    fprintf(file, "<map type=\"binfile\" data=\"$NAVIT_SHAREDIR/maps/%s.bin\" />", dl_info->name);
    fclose(file);

    dl_info->downloading = 0;
    return NULL; //(int)res;
}

