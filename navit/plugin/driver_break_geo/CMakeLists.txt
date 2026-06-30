add_library(driver_break_geo STATIC
    driver_break_geo.c
)

target_include_directories(driver_break_geo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/navit
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(driver_break_geo PUBLIC glib-2.0)
