find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 sqlite3)

if(SQLITE3_FOUND)
    set(plugin_aprs_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_aprs_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    set(plugin_aprs_FLAGS ${SQLITE3_CFLAGS_OTHER})
else()
    find_library(SQLITE3_LIBRARIES NAMES sqlite3)
    find_path(SQLITE3_INCLUDE_DIRS NAMES sqlite3.h)
    if(SQLITE3_LIBRARIES AND SQLITE3_INCLUDE_DIRS)
        set(plugin_aprs_LIBS ${SQLITE3_LIBRARIES})
        set(plugin_aprs_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "SQLite3 not found. Please install libsqlite3-dev or equivalent.")
    endif()
endif()

module_add_library(plugin_aprs aprs.c aprs_db.c aprs_decode.c aprs_osd.c aprs_nmea.c)

# Export symbols for inter-plugin communication
set_target_properties(plugin_aprs PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
)

