include_directories(${LIBCURL_INCLUDE_DIRS})
module_add_library(traffic_traff_http traffic_traff_http.c)
target_link_libraries(traffic_traff_http ${LIBCURL_LIBRARIES})