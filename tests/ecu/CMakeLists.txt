# ECU plugin unit tests (independent of driver_break.c)

add_executable(test_ecu_obd_j1939
    test_ecu_obd_j1939.c
)

target_link_libraries(test_ecu_obd_j1939
    m
)

target_include_directories(test_ecu_obd_j1939 PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_ecu
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_executable(test_ecu_megasquirt
    test_ecu_megasquirt.c
)

target_link_libraries(test_ecu_megasquirt
    m
)

target_include_directories(test_ecu_megasquirt PRIVATE
    ${CMAKE_SOURCE_DIR}/navit/plugin/driver_ecu
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

add_test(NAME ecu_obd_j1939_test COMMAND test_ecu_obd_j1939)
add_test(NAME ecu_megasquirt_test COMMAND test_ecu_megasquirt)
