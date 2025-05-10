module_add_library(map_textfile textfile.c)
target_link_libraries(map_textfile PUBLIC ${GLIB2_LIBRARIES})