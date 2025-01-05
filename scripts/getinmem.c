#include <stdio.h>
#include <math.h>
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

size_t planet=0;
size_t africa=0;
size_t antarctica=0;
size_t asia=0;
size_t australia_oceania=0;
size_t central_america=0;
size_t europe=0;
size_t north_america=0;
size_t south_america=0;
size_t brazil=0;
size_t france=0;
size_t germany=0;
size_t great_britain=0;
size_t italy=0;
size_t netherlands=0;
size_t poland=0;
size_t us=0;
size_t canada=0;

// Helper function to format file sizes
double convert_filesize(long map_size) {
    if (map_size < 1024) {
        return map_size;
    } else if (map_size < 1024 * 1024) {
        return round(map_size / 1024.0);
    } else if (map_size < 1024 * 1024 * 1024) {
        return round(map_size / (1024.0 * 1024.0));
    } else {
        return round(map_size / (1024.0 * 1024.0 * 1024.0));
    }
}

// Helper function to format file sizes
const char *get_unit(long map_size, char *buffer) {
    if (map_size < 1024) {
        return "B";
    } else if (map_size < 1024 * 1024) {
        return "KiB";
    } else if (map_size < 1024 * 1024 * 1024) {
        return "MiB";
    } else {
        return "GiB";
    }
}

static size_t
mem_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// Function to fetch data from the API using libcurl
struct MemoryStruct fetch_data(const char *url) {
  CURL *curl;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);  /* grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl, CURLOPT_URL, url);

  /* send all data to this function  */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_cb);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers do not like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get it! */
  res = curl_easy_perform(curl);

  /* cleanup curl stuff */
  curl_easy_cleanup(curl);

  if(res != CURLE_OK)
      chunk.size = 0;
  return chunk;
}

// Function to process JSON and extract data
void process_json(const char *json_data) {
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        fprintf(stderr, "Error parsing JSON data.\n");
        return;
    }

    cJSON *assets = cJSON_GetObjectItem(root, "assets");
    if (!assets) {
        fprintf(stderr, "No assets found in JSON data.\n");
        cJSON_Delete(root);
        return;
    }

    FILE *tsv_file = fopen("menu.tsv", "w");

    //fprintf(tsv_file, "name\tsize\n");

    char buffer[64];
    cJSON *asset;
    cJSON_ArrayForEach(asset, assets) {
        cJSON *name = cJSON_GetObjectItem(asset, "name");
        cJSON *size = cJSON_GetObjectItem(asset, "size");

        if (name && size) {
	    name->valuestring[strlen(name->valuestring)-15] = 0;
	    if (strstr(name->valuestring, "africa") != NULL) {
                africa += size->valuedouble;
            }
	    if (strstr(name->valuestring, "antarctica" ) != NULL) {
                antarctica += size->valuedouble;
            }
	    if (strstr(name->valuestring, "asia") != NULL) {
                asia += size->valuedouble;
            }
	    if (strstr(name->valuestring, "australia-oceania") != NULL) {
                australia_oceania += size->valuedouble;
            }
	    if (strstr(name->valuestring, "central-america") != NULL) {
                central_america += size->valuedouble;
            }
	    if (strstr(name->valuestring, "europe") != NULL) {
                europe += size->valuedouble;
            }
	    if (strstr(name->valuestring, "north-america") != NULL) {
                north_america += size->valuedouble;
            }
	    if (strstr(name->valuestring, "south-america") != NULL) {
                south_america += size->valuedouble;
            }
	    if (strstr(name->valuestring, "brazil") != NULL) {
                brazil += size->valuedouble;
            }
	    if (strstr(name->valuestring, "france") != NULL) {
                france += size->valuedouble;
            }
	    if (strstr(name->valuestring, "germany") != NULL) {
                germany += size->valuedouble;
            }
	    if (strstr(name->valuestring, "great-britain") != NULL) {
                great_britain += size->valuedouble;
            }
	    if (strstr(name->valuestring, "italy") != NULL) {
                italy += size->valuedouble;
            }
	    if (strstr(name->valuestring, "netherlands") != NULL) {
                netherlands += size->valuedouble;
            }
	    if (strstr(name->valuestring, "poland") != NULL) {
                poland += size->valuedouble;
            }
	    if (strstr(name->valuestring, "-us-") != NULL) {
                us += size->valuedouble;
            }
	    if (strstr(name->valuestring, "canada") != NULL) {
                canada += size->valuedouble;
            }
	    planet += size->valuedouble;
            fprintf(tsv_file, "%s\t%3.1f\t%s\n", name->valuestring,
                    convert_filesize(size->valuedouble),
		    get_unit(size->valuedouble, buffer));
        }
    }

    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "africa",
            convert_filesize(africa),
            get_unit(africa, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "antarctica",
            convert_filesize(antarctica),
            get_unit(antarctica, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "asia",
            convert_filesize(asia),
            get_unit(asia, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "australia-oceania",
            convert_filesize(australia_oceania),
            get_unit(australia_oceania, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "central-america",
            convert_filesize(central_america),
            get_unit(central_america, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "north-america",
            convert_filesize(north_america),
            get_unit(north_america, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "south-america",
            convert_filesize(south_america),
            get_unit(south_america, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "south-america-brazil",
            convert_filesize(brazil),
            get_unit(brazil, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-france",
            convert_filesize(france),
            get_unit(france, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-germany",
            convert_filesize(germany),
            get_unit(germany, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-great-britain",
            convert_filesize(great_britain),
            get_unit(great_britain, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-italy",
            convert_filesize(italy),
            get_unit(italy, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-netherlands",
            convert_filesize(netherlands),
            get_unit(netherlands, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "europe-poland",
            convert_filesize(poland),
            get_unit(poland, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "north-america-canada",
            convert_filesize(canada),
            get_unit(canada, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "north-america-us",
            convert_filesize(us),
            get_unit(us, buffer));
    fprintf(tsv_file, "%s\t%3.1f\t%s\n", "planet",
            convert_filesize(planet),
            get_unit(planet, buffer));
    fclose(tsv_file);

    cJSON_Delete(root);
}

int main(void)
{
  char* url = "https://api.github.com/repositories/384098365/releases/latest";

  struct MemoryStruct chunk = fetch_data(url);
  /* check for errors */
  if(chunk.size == 0) {
    fprintf(stderr, "Download failed\n");
  }
  else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it
     */

    process_json(chunk.memory);
  }


  free(chunk.memory);

  /* we are done with libcurl, so clean it up */
  curl_global_cleanup();

  return 0;
}
