# SQLite3 and libcurl dependencies are checked in main CMakeLists.txt
# Variables SQLITE3_*, CURL_* are set there
if(SQLITE3_FOUND)
    set(plugin_driver_break_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_driver_break_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    if(SQLITE3_CFLAGS_OTHER)
        set(plugin_driver_break_FLAGS ${SQLITE3_CFLAGS_OTHER})
    endif()
endif()

if(CURL_FOUND)
    list(APPEND plugin_driver_break_LIBS ${CURL_LIBRARIES})
    list(APPEND plugin_driver_break_INCLUDES ${CURL_INCLUDE_DIRS})
    if(CURL_CFLAGS_OTHER)
        list(APPEND plugin_driver_break_FLAGS ${CURL_CFLAGS_OTHER})
    endif()
endif()

# Note: gui_internal is a MODULE library and cannot be linked directly.
# The functions will be resolved at runtime when the plugin is loaded by Navit,
# as long as gui_internal is loaded before this plugin.

module_add_library(plugin_driver_break 
    driver_break.c 
    driver_break_db.c 
    driver_break_finder.c 
    driver_break_poi.c
    driver_break_osd.c
    driver_break_energy.c
    driver_break_hiking.c
    driver_break_cycling.c
    driver_break_poi_hiking.c
    driver_break_route_validator.c
    driver_break_glacier.c
    driver_break_srtm.c
    driver_break_srtm_osd.c
    driver_break_poi_map.c
)

# Export symbols for inter-plugin communication
# Set output name to match Navit's plugin naming convention: lib{category}_{name}.so
# For OSD category with name "driver_break", this becomes libosd_driver_break.so
set_target_properties(plugin_driver_break PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
    OUTPUT_NAME "osd_driver_break"
    PREFIX "lib"
)

# Add unit tests if enabled
option(BUILD_DRIVER_BREAK_TESTS "Build Driver Break plugin unit tests" ON)
if(BUILD_DRIVER_BREAK_TESTS)
    add_subdirectory(tests)
endif()
