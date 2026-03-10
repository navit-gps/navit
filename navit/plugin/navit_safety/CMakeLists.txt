# Navit Safety plugin: POI confidence, remote mode, resource planning for desert/arid travel.
# Optional SQLite for caching and user confirmations (same as driver_break).
if(SQLITE3_FOUND)
    set(plugin_navit_safety_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_navit_safety_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    if(SQLITE3_CFLAGS_OTHER)
        set(plugin_navit_safety_FLAGS ${SQLITE3_CFLAGS_OTHER})
    endif()
endif()

add_definitions(-DNAVIT_SAFETY_WITH_OSD)

module_add_library(plugin_navit_safety
    navit_safety.c
)

# Output name for Navit OSD lookup: libosd_navit_safety.so
set_target_properties(plugin_navit_safety PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
    OUTPUT_NAME "osd_navit_safety"
    PREFIX "lib"
)

# Unit tests for Navit Safety plugin
option(BUILD_NAVIT_SAFETY_TESTS "Build Navit Safety plugin unit tests" ON)
if(BUILD_NAVIT_SAFETY_TESTS)
    add_subdirectory(tests)
endif()
