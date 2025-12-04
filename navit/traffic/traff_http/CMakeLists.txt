include_directories(${LIBCURL_INCLUDE_DIRS})
set(traffic_traff_http_LIBS -l${LIBCURL_LIBRARIES})
module_add_library(traffic_traff_http traffic_traff_http.c)


