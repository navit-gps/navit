if(SQLITE3_FOUND)
    set(plugin_driver_break_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_driver_break_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    if(SQLITE3_CFLAGS_OTHER)
        set(plugin_driver_break_FLAGS ${SQLITE3_CFLAGS_OTHER})
    endif()
endif()

module_add_library(plugin_driver_break
    driver_break.c
    driver_break_config.c
    driver_break_db_schema.c
    driver_break_db_history.c
    driver_break_db_config.c
    driver_break_db_fuel.c
    driver_break_fuel_core.c
    driver_break_osd_lifecycle.c
    driver_break_route_stops.c
    driver_break_finder.c
    driver_break_energy.c
    driver_break_hiking.c
    driver_break_cycling.c
    driver_break_route_validator.c
    driver_break_glacier.c
    driver_break_slice_stubs.c
)

target_include_directories(plugin_driver_break PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/navit
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

if(SQLITE3_FOUND)
    target_link_libraries(plugin_driver_break PRIVATE ${SQLITE3_LIBRARIES})
    target_include_directories(plugin_driver_break PUBLIC ${SQLITE3_INCLUDE_DIRS})
endif()

set_target_properties(plugin_driver_break PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
    OUTPUT_NAME "osd_driver_break"
    PREFIX "lib"
)
