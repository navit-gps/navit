add_library(driver_break_osd STATIC
    driver_break_osd_common.c
    driver_break_osd_core.c
    driver_break_osd_fuel.c
    driver_break_osd_car.c
    driver_break_osd_truck.c
    driver_break_osd_motorcycle.c
    driver_break_osd_hiking.c
    driver_break_osd_cycling.c
    driver_break_osd_overnight.c
    driver_break_gui_shim.c
    driver_break_db.c
    driver_break_finder_stub.c
)

target_include_directories(driver_break_osd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/navit
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(driver_break_osd PUBLIC glib-2.0 navit_core)

if(SQLITE3_FOUND AND SQLITE3_LIBRARIES)
    target_link_libraries(driver_break_osd PUBLIC ${SQLITE3_LIBRARIES})
    target_include_directories(driver_break_osd PUBLIC ${SQLITE3_INCLUDE_DIRS})
endif()
