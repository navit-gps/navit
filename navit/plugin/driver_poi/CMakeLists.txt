add_library(driver_poi STATIC
    driver_break_poi.c
    driver_break_poi_map.c
    driver_break_poi_hiking.c
    driver_break_glacier.c
)

target_include_directories(driver_poi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/navit
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(driver_poi PUBLIC glib-2.0 driver_break_geo)

if(CURL_FOUND AND CURL_LIBRARIES)
    target_link_libraries(driver_poi PRIVATE ${CURL_LIBRARIES})
    target_include_directories(driver_poi PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(driver_poi PRIVATE HAVE_CURL)
endif()
