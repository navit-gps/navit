# Unit tests for Driver Break plugin

# Test executable for database operations
add_executable(test_driver_break_db
    test_driver_break_db.c
    ../driver_break_db.c
)

target_link_libraries(test_driver_break_db
    ${SQLITE3_LIBRARIES}
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_db PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SQLITE3_INCLUDE_DIRS}
)

# Test executable for configuration
add_executable(test_driver_break_config
    test_driver_break_config.c
)

target_link_libraries(test_driver_break_config
    glib-2.0
    m
)

target_include_directories(test_driver_break_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for rest stop finder
add_executable(test_driver_break_finder
    test_driver_break_finder.c
    ../driver_break_finder.c
    ../driver_break_poi.c
    ../driver_break_glacier.c
)

target_link_libraries(test_driver_break_finder
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_finder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for SRTM HGT file handling (and Copernicus GLO-30 when libtiff/curl available)
add_executable(test_driver_break_srtm
    test_driver_break_srtm.c
    ../driver_break_srtm.c
)

target_link_libraries(test_driver_break_srtm PRIVATE
    glib-2.0
    m
    navit_core
)

if(LIBTIFF_FOUND AND LIBTIFF_LIBRARIES)
    target_compile_definitions(test_driver_break_srtm PRIVATE HAVE_GEOTIFF)
    target_link_libraries(test_driver_break_srtm PRIVATE ${LIBTIFF_LIBRARIES})
    target_include_directories(test_driver_break_srtm PRIVATE ${LIBTIFF_INCLUDE_DIRS})
endif()
if(CURL_FOUND AND CURL_LIBRARIES)
    target_compile_definitions(test_driver_break_srtm PRIVATE HAVE_CURL)
    target_link_libraries(test_driver_break_srtm PRIVATE ${CURL_LIBRARIES})
    target_include_directories(test_driver_break_srtm PRIVATE ${CURL_INCLUDE_DIRS})
endif()

target_include_directories(test_driver_break_srtm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for routing with profiles
add_executable(test_driver_break_routing
    test_driver_break_routing.c
    ../driver_break_route_validator.c
    ../driver_break_poi_map.c
)

target_link_libraries(test_driver_break_routing
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_routing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Integration test executable (uses actual Navit/plugin functions)
add_executable(test_driver_break_integration
    test_driver_break_integration.c
    ../driver_break_hiking.c
    ../driver_break_cycling.c
    ../driver_break_poi_hiking.c
    ../driver_break_poi.c
    ../driver_break_poi_map.c
    ../driver_break_route_validator.c
    ../driver_break_srtm.c
)

target_link_libraries(test_driver_break_integration
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Route integration test (tests rest intervals and POI discovery)
add_executable(test_driver_break_route_integration
    test_driver_break_route_integration.c
    ../driver_break_hiking.c
    ../driver_break_cycling.c
    ../driver_break_poi_hiking.c
    ../driver_break_poi.c
    ../driver_break_poi_map.c
    ../driver_break_route_validator.c
    ../driver_break_srtm.c
)

target_link_libraries(test_driver_break_route_integration PRIVATE
    glib-2.0
    m
    navit_core
)

if(LIBTIFF_FOUND AND LIBTIFF_LIBRARIES)
    target_compile_definitions(test_driver_break_route_integration PRIVATE HAVE_GEOTIFF)
    target_link_libraries(test_driver_break_route_integration PRIVATE ${LIBTIFF_LIBRARIES})
    target_include_directories(test_driver_break_route_integration PRIVATE ${LIBTIFF_INCLUDE_DIRS})
endif()

target_include_directories(test_driver_break_route_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# POI/map tests: map_data lives next to this CMakeLists.txt; binfile driver is built as target map_binfile.
target_compile_definitions(test_driver_break_route_integration PRIVATE
    "DRIVER_BREAK_TEST_MAP_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/map_data\"")
if(USE_PLUGINS AND TARGET map_binfile)
    add_dependencies(test_driver_break_route_integration map_binfile)
    target_compile_definitions(test_driver_break_route_integration PRIVATE
        "DRIVER_BREAK_TEST_BINFILE_SO=\"$<TARGET_FILE:map_binfile>\"")
endif()

# OBD-II and J1939 backend tests (pure logic, no hardware required)
add_executable(test_driver_break_obd_j1939
    test_driver_break_obd_j1939.c
)

target_link_libraries(test_driver_break_obd_j1939
    glib-2.0
    m
)

target_include_directories(test_driver_break_obd_j1939 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# MegaSquirt backend tests (formula, parsing, malformed data; no hardware)
add_executable(test_driver_break_megasquirt
    test_driver_break_megasquirt.c
)

target_link_libraries(test_driver_break_megasquirt
    glib-2.0
    m
)

target_include_directories(test_driver_break_megasquirt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add test targets
enable_testing()

add_test(NAME driver_break_db_test COMMAND test_driver_break_db)
add_test(NAME driver_break_config_test COMMAND test_driver_break_config)
add_test(NAME driver_break_finder_test COMMAND test_driver_break_finder)
add_test(NAME driver_break_srtm_test COMMAND test_driver_break_srtm)
add_test(NAME driver_break_routing_test COMMAND test_driver_break_routing)
add_test(NAME driver_break_integration_test COMMAND test_driver_break_integration)
add_test(NAME driver_break_route_integration_test COMMAND test_driver_break_route_integration)
add_test(NAME driver_break_obd_j1939_test COMMAND test_driver_break_obd_j1939)
add_test(NAME driver_break_megasquirt_test COMMAND test_driver_break_megasquirt)
