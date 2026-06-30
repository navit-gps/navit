# POI unit tests (independent of driver_break plugin sources)

add_executable(test_driver_break_poi
    test_driver_break_poi.c
)

target_link_libraries(test_driver_break_poi PRIVATE
    driver_poi
    driver_break_geo
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_poi PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_poi
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_geo
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_test(NAME driver_break_poi_test COMMAND test_driver_break_poi)
