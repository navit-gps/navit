add_library(driver_break_srtm STATIC
    driver_break_srtm_tiles.c
    driver_break_srtm_io.c
    driver_break_srtm_download.c
    driver_break_srtm_osd.c
    driver_break_gui_shim.c
)

target_include_directories(driver_break_srtm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/navit
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(driver_break_srtm PUBLIC glib-2.0 navit_core)

if(LIBTIFF_FOUND AND LIBTIFF_LIBRARIES)
    target_compile_definitions(driver_break_srtm PUBLIC HAVE_GEOTIFF)
    target_link_libraries(driver_break_srtm PUBLIC ${LIBTIFF_LIBRARIES})
    target_include_directories(driver_break_srtm PUBLIC ${LIBTIFF_INCLUDE_DIRS})
endif()

if(CURL_FOUND AND CURL_LIBRARIES)
    target_compile_definitions(driver_break_srtm PUBLIC HAVE_CURL)
    target_link_libraries(driver_break_srtm PUBLIC ${CURL_LIBRARIES})
    target_include_directories(driver_break_srtm PUBLIC ${CURL_INCLUDE_DIRS})
endif()
