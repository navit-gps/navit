# OSD slice unit tests (config/db smoke tests and static library link check)

add_executable(test_driver_break_config
    test_driver_break_config.c
)

target_link_libraries(test_driver_break_config PRIVATE
    glib-2.0
    m
)

target_include_directories(test_driver_break_config PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_osd
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_executable(test_driver_break_db
    test_driver_break_db.c
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_osd/driver_break_db.c
)

target_link_libraries(test_driver_break_db PRIVATE
    ${SQLITE3_LIBRARIES}
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_db PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_osd
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

add_test(NAME driver_break_config_test COMMAND test_driver_break_config)
add_test(NAME driver_break_db_test COMMAND test_driver_break_db)
