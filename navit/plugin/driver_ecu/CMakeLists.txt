add_library(driver_ecu STATIC
    driver_ecu_obd.c
    driver_ecu_j1939.c
    driver_ecu_megasquirt.c
)

target_include_directories(driver_ecu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/navit
)

target_link_libraries(driver_ecu PUBLIC glib-2.0)
