include_directories(${LIBCURL_INCLUDE_DIRS})
# LIBCURL_LIBRARIES is either a plain name (host, from pkg-config) or the
# curl::curl_static imported target (Android, from the curl prefab). As a
# CMake library item it no longer needs an explicit -l prefix.
set(traffic_traff_http_LIBS ${LIBCURL_LIBRARIES})
module_add_library(traffic_traff_http traffic_traff_http.c)

