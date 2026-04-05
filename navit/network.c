#include "curl/curl.h"
#include <cjson/cJSON.h>
#include <math.h>
#include <string.h>
#include <navit/debug.h>
#include <glib.h>
#include "unistd.h"
#include "stddef.h"
#include "navit.h"
#include <navit/coord.h>
#include <navit/color.h>
#include <navit/item.h>
#include <navit/point.h>
#include <pthread.h>
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
}


// Helper to add to listdata
void add_to_listdata(char *name, long long size, int* listdata_count, MapData *listdata) {
    char size_buf[32];
    format_filesize(size, size_buf);
    listdata[*listdata_count].name = strdup(name);
    listdata[*listdata_count].size_str = strdup(size_buf);
    (*listdata_count)++;
}

// Helper to find size in maps_size
long long get_size(char *key, int *maps_count, MapData *maps_size) {
    for(int i=0; i<*maps_count; i++) {
        if(strcmp(maps_size[i].name, key) == 0) {
            return maps_size[i].size_val;
        }
    }
    return 0;
}

/**
 * Download a map using libcurl
 *
 * @param arguments a void * that we can cast to a map_download_info * as this function is intended to be run
 *        within a thread
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
    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, progressCallBack);

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
//        curl_easy_setopt(handle, CURLOPT_RESUME_FROM, current_size);
//        dl_info->resume = 1;
    } else {
        dbg(lvl_debug, "Downloading from scratch\n");
    }

    // Map server might redirect to a mirror
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, dl_info);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, progressCallBack);

    file = fopen( dl_info->path, current_size > 0 ? "ab":"wb");
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, file) ;
    res = curl_easy_perform( handle );
    fflush(file);
    fclose(file); // needed to prevent EOC errors

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
    fflush(file);
    fclose(file);

    dl_info->downloading = 0;

       // Free dl_info and its strings
    if (dl_info) {
        g_free(dl_info->name);
        g_free(dl_info->path);
        g_free(dl_info->url);
        g_free(dl_info);
    }

    // Free dl_data (safe because only used in threads)
    g_free(dl_data);
    return NULL; //(int)res;
}


void * download_map3(void * data) {
    // check name
    // if name directly in list, skip it and only take elements which are not part of others
    // build download list
    // download list one by one
    // make dl_info and call download_map2

    char buffer[256];
    FILE *f;
    GThreadPool *pool = g_thread_pool_new(
    (GFunc)download_map2,
        NULL,
        5,                     // Max 5 threads
        FALSE,
        NULL
    );
    // --- Define Lists ---
    const char *continents_arr[] = {"africa", "asia", "australia-oceania", "central-america", "europe", "north-america", "south-america", NULL};
    const char *countries_europe_subregion_arr[] = {"europe-france", "europe-germany", "europe-great-britain", "europe-italy", "europe-netherlands", "europe-poland", NULL};
    const char *countries_north_america_subregion_arr[] = {"north-america-canada", "north-america-us", NULL};
    const char *countries_south_america_subregion_arr[] = {"south-america-brazil", NULL};
    struct gui_download_data *dl_data = data;
    struct map_download_info * dl_info = dl_data->data;
    char * requested_name = dl_info->name;

    f = fopen("maps/menu.tsv", "r");




    while (f && fgets(buffer, sizeof(buffer), f)) {
        char* name = g_strsplit(buffer,"\t",0)[0];
        struct map_download_info *dl_info2 = g_malloc(sizeof(struct map_download_info));
        struct gui_download_data *dl_data2 = g_malloc(sizeof(struct gui_download_data));

	dl_data2->this = dl_data->this;
	dl_data2->wm = dl_data->wm;
	dl_data2->data = dl_info2;

        if (strcmp(requested_name, "planet") == 0) {
            if (g_strv_contains(continents_arr, name) == 1)  {
                continue;
            } else if (g_strv_contains(countries_europe_subregion_arr, name) == 1) {
                continue;
            } else if (g_strv_contains(countries_north_america_subregion_arr, name) == 1) {
                continue;
            } else if (g_strv_contains(countries_south_america_subregion_arr, name) == 1) {
                continue;
            }
            dl_info2->name = g_strdup(name);
            dl_info2->path = g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/maps/", name, ".bin",  NULL);
            dl_info2->xml = g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/maps/", name, ".xml",  NULL);
            dl_info2->url = g_strjoin(NULL, "https://github.com/navit-gps/gh-actions-mapserver/releases/download/", g_date_time_format(g_date_time_new_now_utc(), "%Y-%m-%d"), "/", name, "-", g_date_time_format(g_date_time_new_now_utc(), "%Y-%m-%d"), ".bin", NULL);
        } else {
        if (g_strstr_len(name, -1, requested_name)!=NULL){
                if (g_strv_contains(continents_arr, name) == 1) {
                    continue;
                } else if (g_strv_contains(countries_europe_subregion_arr, name) == 1) {
                    continue;
                } else if (g_strv_contains(countries_north_america_subregion_arr, name) == 1) {
                    continue;
                } else if (g_strv_contains(countries_south_america_subregion_arr, name) == 1) {
                   continue;
                }
                dl_info2->name = g_strdup(name);
                dl_info2->path = g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/maps/", name, ".bin",  NULL);
                dl_info2->xml = g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/maps/", name, ".xml",  NULL);
                dl_info2->url = g_strjoin(NULL, "https://github.com/navit-gps/gh-actions-mapserver/releases/download/", g_date_time_format(g_date_time_new_now_local(), "%Y-%m-%d"), "/", name, "-", g_date_time_format(g_date_time_new_now_local(), "%Y-%m-%d"), ".bin", NULL);
            } else {
                continue;
            }
        }

        g_thread_pool_push(pool, dl_data2, NULL); // Push to thread pool

    }

    curl_global_cleanup();
    fclose(f);
    g_thread_pool_free(pool, FALSE, TRUE); // Wait for all tasks to finish
    return NULL;
}


// --- Helper Functions ---

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **response_ptr = (char **)userp;
    *response_ptr = realloc(*response_ptr, strlen(*response_ptr) + realsize + 1);
    if (*response_ptr == NULL) {

        dbg(lvl_error, "Not enough memory (realloc returned NULL)");
        return 0;
    }
    strncat(*response_ptr, (char *)contents, realsize);
    return realsize;
}

void format_filesize(long long map_size, char *buffer) {
    if (map_size < 1024) {
        sprintf(buffer, "%lld B", map_size);
    } else if (map_size < 1024 * 1024) {
        sprintf(buffer, "%.0f KiB", round((double)map_size / 1024));
    } else if (map_size < 1024 * 1024 * 1024) {
        sprintf(buffer, "%.0f MiB", round((double)map_size / (1024 * 1024)));
    } else {
        sprintf(buffer, "%.0f GiB", round((double)map_size / (1024 * 1024 * 1024)));
    }
}


/**
 * Update map download table, checks whether internet connection exists
 *
 * @param nothing
 *
 * @returns nothing
 */

void * update_download_table(){
    CURL *curl;
    CURLcode res;
    char *response = NULL;
    cJSON *json = NULL;
    cJSON *assets = NULL;
    cJSON *asset = NULL;

    response = malloc(1);
    response[0] = '\0';

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repositories/384098365/releases/latest");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            dbg(lvl_warning, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
            free(response);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return NULL;
        }
        curl_easy_cleanup(curl);
    }

    json = cJSON_Parse(response);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            dbg(lvl_warning, "Error before: %s\n", error_ptr);
        }
        free(response);
        curl_global_cleanup();
        return NULL;
    }
    free(response);

    assets = cJSON_GetObjectItemCaseSensitive(json, "assets");
    if (!cJSON_IsArray(assets)) {
        dbg(lvl_warning, "Assets not found or not an array");
        cJSON_Delete(json);
        return NULL;
    }

    // Build maps_size dictionary
    MapData *maps_size = malloc(cJSON_GetArraySize(assets) * sizeof(MapData));
    int maps_count = 0;

    cJSON_ArrayForEach(asset, assets) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(asset, "name");
        cJSON *size = cJSON_GetObjectItemCaseSensitive(asset, "size");

        if (cJSON_IsString(name) && cJSON_IsNumber(size)) {
            char *asset_name = name->valuestring;
            char *dash_pos = strstr(asset_name, "-2");
            int key_len = dash_pos ? (int)(dash_pos - asset_name) : (int)strlen(asset_name);

            char *key = malloc(key_len + 1);
            strncpy(key, asset_name, key_len);
            key[key_len] = '\0';

            for(int k=0; k<key_len; k++) {
                if(key[k] == '-') key[k] = '_';
            }

            maps_size[maps_count].name = key;
            maps_size[maps_count].size_val = size->valueint;
            maps_count++;
        }
    }

    // --- Define Lists ---
    const char *continents_arr[] = {"africa", "antarctica", "asia", "australia_oceania", "central_america", "europe", "north_america", "south_america", NULL};

    const char *countries_europe_subregion_arr[] = {"france", "germany", "great_britain", "italy", "netherlands", "poland", NULL};

    const char *countries_europe_arr[] = {"albania", "andorra", "austria", "azores", "belarus", "belgium", "bosnia_herzegovina", "bulgaria", "croatia", "cyprus", "czech_republic", "denmark", "estonia", "faroe_islands", "finland", "france", "georgia", "germany", "great_britain", "greece", "guernsey_jersey", "hungary", "iceland", "ireland_and_northern_ireland", "isle_of_man", "italy", "kosovo", "latvia", "liechtenstein", "lithuania", "luxembourg", "macedonia", "malta", "moldova", "monaco", "montenegro", "netherlands", "norway", "poland", "portugal", "romania", "serbia", "slovakia", "slovenia", "spain", "sweden", "switzerland", "turkey", "ukraine", NULL};

    const char *countries_africa_arr[] = {"algeria", "angola", "benin", "botswana", "burkina_faso", "burundi", "cameroon", "canary_islands", "cape_verde", "central_african_republic", "chad", "comores", "congo_brazzaville", "congo_democratic_republic", "djibouti", "egypt", "equatorial_guinea", "eritrea", "ethiopia", "gabon", "ghana", "guinea", "guinea_bissau", "ivory_coast", "kenya", "lesotho", "liberia", "libya", "madagascar", "malawi", "mali", "mauritania", "mauritius", "morocco", "mozambique", "namibia", "niger", "nigeria", "rwanda", "saint_helena_ascension_and_tristan_da_cunha", "sao_tome_and_principe", "senegal_and_gambia", "seychelles", "sierra_leone", "somalia", "south_africa", "south_sudan", "sudan", "swaziland", "tanzania", "togo", "tunisia", "uganda", "zambia", "zimbabwe", NULL};

    const char *countries_asia_arr[] = {"afghanistan", "armenia", "azerbaijan", "bangladesh", "bhutan", "cambodia", "china", "gcc_states", "india", "indonesia", "iran", "iraq", "israel_and_palestine", "japan", "jordan", "kazakhstan", "kyrgyzstan", "laos", "lebanon", "malaysia_singapore_brunei", "maldives", "mongolia", "myanmar", "nepal", "north_korea", "pakistan", "philippines", "south_korea", "sri_lanka", "syria", "taiwan", "tajikistan", "thailand", "turkmenistan", "uzbekistan", "vietnam", "yemen", NULL};

    const char *countries_australia_oceania_arr[] = {"american_oceania", "australia", "cook_islands", "fiji", "ile_de_clipperton", "kiribati", "marshall_islands", "micronesia", "nauru", "new_caledonia", "new_zealand", "niue", "palau", "papua_new_guinea", "pitcairn_islands", "polynesie_francaise", "samoa", "solomon_islands", "tokelau", "tonga", "tuvalu", "vanuatu", "wallis_et_futuna", NULL};

    const char *countries_central_america_arr[] = {"bahamas", "belize", "costa_rica", "cuba", "el_salvador", "guatemala", "haiti_and_domrep", "honduras", "jamaica", "nicaragua", NULL};

    const char *countries_north_america_subregion_arr[] = {"canada", "us", NULL};

    const char *countries_north_america_arr[] = {"canada", "greenland", "mexico", "us", NULL};

    const char *countries_south_america_subregion_arr[] = {"brazil", NULL};

    const char *countries_south_america_arr[] = {"argentina", "bolivia", "brazil", "chile", "colombia", "ecuador", "paraguay", "peru", "suriname", "uruguay", "venezuela", NULL};

    // Regions
    const char *regions_france_arr[] = {"alsace", "aquitaine", "auvergne", "basse_normandie", "bourgogne", "bretagne", "centre", "champagne_ardenne", "corse", "franche_comte", "guadeloupe", "guyane", "haute_normandie", "ile_de_france", "languedoc_roussillon", "limousin", "lorraine", "martinique", "mayotte", "midi_pyrenees", "nord_pas_de_calais", "pays_de_la_loire", "picardie", "poitou_charentes", "provence_alpes_cote_d_azur", "reunion", "rhone_alpes", NULL};

    const char *regions_germany_arr[] = {"baden_wuerttemberg", "bayern", "berlin", "brandenburg", "bremen", "hamburg", "hessen", "mecklenburg_vorpommern", "niedersachsen", "nordrhein_westfalen", "rheinland_pfalz", "saarland", "sachsen_anhalt", "sachsen", "schleswig_holstein", "thueringen", NULL};

    const char *regions_great_britain_arr[] = {"england", "scotland", "wales", NULL};

    const char *regions_italy_arr[] = {"centro", "isole", "nord_est", "nord_ovest", "sud", NULL};

    const char *regions_netherlands_arr[] = {"drenthe", "flevoland", "friesland", "gelderland", "groningen", "limburg", "noord_brabant", "noord_holland", "overijssel", "utrecht", "zeeland", "zuid_holland", NULL};

    const char *regions_poland_arr[] = {"dolnoslaskie", "kujawsko_pomorskie", "lodzkie", "lubelskie", "lubuskie", "malopolskie", "mazowieckie", "opolskie", "podkarpackie", "podlaskie", "pomorskie", "slaskie", "swietokrzyskie", "warminsko_mazurskie", "wielkopolskie", "zachodniopomorskie", NULL};

    const char *regions_canada_arr[] = {"alberta", "british_columbia", "manitoba", "new_brunswick", "newfoundland_and_labrador", "northwest_territories", "nova_scotia", "nunavut", "ontario", "prince_edward_island", "quebec", "saskatchewan", "yukon", NULL};

    const char *regions_us_arr[] = {"alabama", "alaska", "arizona", "arkansas", "california", "colorado", "connecticut", "delaware", "district_of_columbia", "florida", "georgia", "hawaii", "idaho", "illinois", "indiana", "iowa", "kansas", "kentucky", "louisiana", "maine", "maryland", "massachusetts", "michigan", "minnesota", "mississippi", "missouri", "montana", "nebraska", "nevada", "new_hampshire", "new_jersey", "new_mexico", "new_york", "north_carolina", "north_dakota", "ohio", "oklahoma", "oregon", "pennsylvania", "puerto_rico", "rhode_island", "south_carolina", "south_dakota", "tennessee", "texas", "utah", "vermont", "virginia", "washington", "west_virginia", "wisconsin", "wyoming", NULL};

    const char *regions_brazil_arr[] = {"centro_oeste", "nordeste", "norte", "sudeste", "sul", NULL};

    // --- Processing Logic ---
    MapData *listdata = malloc(1000 * sizeof(MapData));
    int listdata_count = 0;
    long long size_planet = 0;


    // Iterate Continents
    for (int c = 0; continents_arr[c]!=NULL; c++) {
        const char *continent = continents_arr[c];
        long long size_continent = 0;

        if (strcmp(continent, "europe") == 0) {
            long long size_europe = 0;
            for (int i = 0; countries_europe_arr[i]!=NULL; i++) {
                const char *country = countries_europe_arr[i];
                long long size_country = 0;

                if (g_strv_contains(countries_europe_subregion_arr, country)) {
		    const char ** regions;
                    if (strcmp(country, "france") == 0) regions = regions_france_arr;
                    else if (strcmp(country, "germany") == 0) regions = regions_germany_arr;
                    else if (strcmp(country, "great_britain") == 0) regions = regions_great_britain_arr;
                    else if (strcmp(country, "italy") == 0) regions = regions_italy_arr;
                    else if (strcmp(country, "netherlands") == 0) regions = regions_netherlands_arr;
                    else if (strcmp(country, "poland") == 0) regions = regions_poland_arr;
                    else continue; 

                    for (int r = 0; regions[r]!=NULL; r++) {
                        const char *region = regions[r];
                        char key[256];
                        sprintf(key, "%s_%s_%s", continent, country, region);
                        long long map_size = get_size(key, &maps_count, maps_size);
                        size_country += map_size;
                        add_to_listdata(key, map_size, &listdata_count, listdata);
                    }
                } else {
                    char key[256];
                    sprintf(key, "%s_%s", continent, country);
                    size_country = get_size(key, &maps_count, maps_size);
                }
                add_to_listdata(country, size_country, &listdata_count, listdata); 
                char key[256];
                sprintf(key, "%s_%s", continent, country);
                add_to_listdata(key, size_country, &listdata_count, listdata);
                size_europe += size_country;
            }
            add_to_listdata("europe", size_europe, &listdata_count, listdata);
            size_planet += size_europe;
        }
        else if (strcmp(continent, "north_america") == 0) {
            long long size_north_america = 0;
            for (int i = 0; countries_north_america_arr[i]!=NULL; i++) {
                const char *country = countries_north_america_arr[i];
                long long size_country = 0;

                if (g_strv_contains(countries_north_america_subregion_arr, country)) {
		    const char ** regions;
                    if (strcmp(country, "canada") == 0) regions = regions_canada_arr;
                    else if (strcmp(country, "us") == 0) regions = regions_us_arr;
                    else continue;

                    for (int r = 0; regions[r]!=NULL; r++) {
                        const char *region = regions[r];
                        char key[256];
                        sprintf(key, "%s_%s_%s", continent, country, region);
                        long long map_size = get_size(key, &maps_count, maps_size);
                        size_country += map_size;
                        add_to_listdata(key, map_size, &listdata_count, listdata);
                    }
                } else {
                    char key[256];
                    sprintf(key, "%s_%s", continent, country);
                    size_country = get_size(key, &maps_count, maps_size);
                }
                char key[256];
                sprintf(key, "%s_%s", continent, country);
                add_to_listdata(key, size_country, &listdata_count, listdata);
                size_north_america += size_country;
            }
            add_to_listdata("north_america", size_north_america, &listdata_count, listdata);
            size_planet += size_north_america;
        }
        else if (strcmp(continent, "south_america") == 0) {
            long long size_south_america = 0;
            for (int i = 0; countries_south_america_arr[i]!=NULL; i++) {
                const char *country = countries_south_america_arr[i];
                long long size_country = 0;

                if (g_strv_contains(countries_south_america_subregion_arr, country)) {
		    const char ** regions;
                    if (strcmp(country, "brazil") == 0) regions = regions_brazil_arr;
                    else continue;

                    for (int r = 0; regions[r]!=NULL; r++) {
                        const char *region = regions[r];
                        char key[256];
                        sprintf(key, "%s_%s_%s", continent, country, region);
                        long long map_size = get_size(key, &maps_count, maps_size);
                        size_country += map_size;
                        add_to_listdata(key, map_size, &listdata_count, listdata);
                    }
                } else {
                    char key[256];
                    sprintf(key, "%s_%s", continent, country);
                    size_country = get_size(key, &maps_count, maps_size);
                }
                char key[256];
                sprintf(key, "%s_%s", continent, country);
                add_to_listdata(key, size_country, &listdata_count, listdata);
                size_south_america += size_country;
            }
            add_to_listdata("south_america", size_south_america, &listdata_count, listdata);
            size_planet += size_south_america;
        }
        else if (strcmp(continent, "antarctica") == 0) {
            long long map_size = get_size(continent, &maps_count, maps_size);
            add_to_listdata("antartica", map_size, &listdata_count, listdata); 
            size_planet += map_size;
        }
        else {
            // Generic continent logic (Africa, Asia, Australia_Oceania, Central_America)
	    const char ** countries;
            if (strcmp(continent, "africa") == 0) countries = countries_africa_arr;
            else if (strcmp(continent, "asia") == 0) countries = countries_asia_arr;
            else if (strcmp(continent, "australia_oceania") == 0) countries = countries_australia_oceania_arr;
            else if (strcmp(continent, "central_america") == 0) countries = countries_central_america_arr;
            else continue;

            for (int i = 0; countries[i]!=NULL; i++) {
                const char *country = countries[i];
                char key[256];
                sprintf(key, "%s_%s", continent, country);
                long long map_size = get_size(key, &maps_count, maps_size);
                size_continent += map_size;
                add_to_listdata(key, map_size, &listdata_count, listdata);
            }
            add_to_listdata(continent, size_continent, &listdata_count, listdata);
            size_planet += size_continent;
        }
    }

    // Add Planet
    add_to_listdata("planet", size_planet, &listdata_count, listdata);

    // --- Output to TSV ---
    FILE *tsv = fopen("maps/menu.tsv", "w");
    if (tsv) {
        for (int i = 0; i < listdata_count; i++) {
            fprintf(tsv, "%s\t%s\n", listdata[i].name, listdata[i].size_str);
        }
        fclose(tsv);
    }

    // Cleanup
    cJSON_Delete(json);
    curl_global_cleanup();
    // Free allocated memory...
    return NULL;

}
