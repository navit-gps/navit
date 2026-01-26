# Unit tests for Rest plugin

# Test executable for database operations
add_executable(test_rest_db
    test_rest_db.c
    ../rest_db.c
)

target_link_libraries(test_rest_db
    ${SQLITE3_LIBRARIES}
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_db PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SQLITE3_INCLUDE_DIRS}
)

# Test executable for configuration
add_executable(test_rest_config
    test_rest_config.c
)

target_link_libraries(test_rest_config
    glib-2.0
    m
)

target_include_directories(test_rest_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for rest stop finder
add_executable(test_rest_finder
    test_rest_finder.c
    ../rest_finder.c
    ../rest_poi.c
    ../rest_glacier.c
)

target_link_libraries(test_rest_finder
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_finder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for SRTM HGT file handling
add_executable(test_rest_srtm
    test_rest_srtm.c
    ../rest_srtm.c
)

target_link_libraries(test_rest_srtm
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_srtm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Test executable for routing with profiles
add_executable(test_rest_routing
    test_rest_routing.c
    ../rest_route_validator.c
    ../rest_poi_map.c
)

target_link_libraries(test_rest_routing
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_routing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Integration test executable (uses actual Navit/plugin functions)
add_executable(test_rest_integration
    test_rest_integration.c
    ../rest_hiking.c
    ../rest_cycling.c
    ../rest_poi_hiking.c
    ../rest_poi.c
    ../rest_poi_map.c
    ../rest_route_validator.c
    ../rest_srtm.c
)

target_link_libraries(test_rest_integration
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Route integration test (tests rest intervals and POI discovery)
add_executable(test_rest_route_integration
    test_rest_route_integration.c
    ../rest_hiking.c
    ../rest_cycling.c
    ../rest_poi_hiking.c
    ../rest_poi.c
    ../rest_poi_map.c
    ../rest_route_validator.c
    ../rest_srtm.c
)

target_link_libraries(test_rest_route_integration
    glib-2.0
    m
    navit_core
)

target_include_directories(test_rest_route_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add test targets
enable_testing()

add_test(NAME rest_db_test COMMAND test_rest_db)
add_test(NAME rest_config_test COMMAND test_rest_config)
add_test(NAME rest_finder_test COMMAND test_rest_finder)
add_test(NAME rest_srtm_test COMMAND test_rest_srtm)
add_test(NAME rest_routing_test COMMAND test_rest_routing)
add_test(NAME rest_integration_test COMMAND test_rest_integration)
add_test(NAME rest_route_integration_test COMMAND test_rest_route_integration)
