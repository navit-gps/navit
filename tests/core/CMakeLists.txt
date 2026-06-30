# Driver Break core slice unit tests

add_executable(test_driver_break_db
    test_driver_break_db.c
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core/driver_break_db_schema.c
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core/driver_break_db_history.c
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core/driver_break_db_config.c
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core/driver_break_db_fuel.c
)

target_link_libraries(test_driver_break_db PRIVATE
    ${SQLITE3_LIBRARIES}
    glib-2.0
    m
    navit_core
)

target_include_directories(test_driver_break_db PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

add_executable(test_driver_break_config
    test_driver_break_config.c
)

target_link_libraries(test_driver_break_config PRIVATE
    glib-2.0
    m
)

target_include_directories(test_driver_break_config PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_break_core
    ${CMAKE_SOURCE_DIR}/navit
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_test(NAME driver_break_db_test COMMAND test_driver_break_db)
add_test(NAME driver_break_config_test COMMAND test_driver_break_config)
