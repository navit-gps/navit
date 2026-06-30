# SRTM unit tests (independent of driver_break plugin sources)

add_executable(test_driver_break_srtm
    test_driver_break_srtm.c
)

target_link_libraries(test_driver_break_srtm PRIVATE
    driver_break_srtm
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_srtm PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_srtm
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_test(NAME driver_break_srtm_test COMMAND test_driver_break_srtm)
