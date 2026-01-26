find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 sqlite3)
pkg_check_modules(CURL libcurl)

if(SQLITE3_FOUND)
    set(plugin_rest_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_rest_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    set(plugin_rest_FLAGS ${SQLITE3_CFLAGS_OTHER})
else()
    find_library(SQLITE3_LIBRARIES NAMES sqlite3)
    find_path(SQLITE3_INCLUDE_DIRS NAMES sqlite3.h)
    if(SQLITE3_LIBRARIES AND SQLITE3_INCLUDE_DIRS)
        set(plugin_rest_LIBS ${SQLITE3_LIBRARIES})
        set(plugin_rest_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "SQLite3 not found. Please install libsqlite3-dev or equivalent.")
    endif()
endif()

if(CURL_FOUND)
    list(APPEND plugin_rest_LIBS ${CURL_LIBRARIES})
    list(APPEND plugin_rest_INCLUDES ${CURL_INCLUDE_DIRS})
    list(APPEND plugin_rest_FLAGS ${CURL_CFLAGS_OTHER})
else()
    find_library(CURL_LIBRARIES NAMES curl)
    find_path(CURL_INCLUDE_DIRS NAMES curl/curl.h)
    if(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)
        list(APPEND plugin_rest_LIBS ${CURL_LIBRARIES})
        list(APPEND plugin_rest_INCLUDES ${CURL_INCLUDE_DIRS})
    else()
        message(WARNING "libcurl not found. POI discovery will be disabled.")
        message(WARNING "Install libcurl-dev or equivalent to enable POI discovery.")
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
