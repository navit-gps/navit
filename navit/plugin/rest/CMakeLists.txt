# SQLite3 and libcurl dependencies are checked in main CMakeLists.txt
# Variables SQLITE3_*, CURL_* are set there
if(SQLITE3_FOUND)
    set(plugin_rest_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_rest_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    if(SQLITE3_CFLAGS_OTHER)
        set(plugin_rest_FLAGS ${SQLITE3_CFLAGS_OTHER})
    endif()
endif()

if(CURL_FOUND)
    list(APPEND plugin_rest_LIBS ${CURL_LIBRARIES})
    list(APPEND plugin_rest_INCLUDES ${CURL_INCLUDE_DIRS})
    if(CURL_CFLAGS_OTHER)
        list(APPEND plugin_rest_FLAGS ${CURL_CFLAGS_OTHER})
    endif()
endif()

# Note: gui_internal is a MODULE library and cannot be linked directly.
# The functions will be resolved at runtime when the plugin is loaded by Navit,
# as long as gui_internal is loaded before this plugin.

module_add_library(plugin_rest 
    rest.c 
    rest_db.c 
    rest_finder.c 
    rest_poi.c
    rest_osd.c
    rest_energy.c
    rest_hiking.c
    rest_cycling.c
    rest_poi_hiking.c
    rest_route_validator.c
    rest_glacier.c
    rest_srtm.c
    rest_srtm_osd.c
    rest_poi_map.c
)

# Export symbols for inter-plugin communication
# Set output name to match Navit's plugin naming convention: lib{category}_{name}.so
# For OSD category with name "rest", this becomes libosd_rest.so
set_target_properties(plugin_rest PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
    OUTPUT_NAME "osd_rest"
    PREFIX "lib"
)

# Add unit tests if enabled
option(BUILD_REST_TESTS "Build Rest plugin unit tests" ON)
if(BUILD_REST_TESTS)
    add_subdirectory(tests)
endif()
