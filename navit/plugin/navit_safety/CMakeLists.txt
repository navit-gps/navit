# Navit Safety plugin: POI confidence, remote mode, resource planning for desert/arid travel.

# Pure planning engines (no Navit core or SQLite dependency).
set(NAVIT_SAFETY_SOURCES
    navit_safety.c
    navit_safety_confidence.c
    navit_safety_koppen.c
    navit_safety_lookahead.c
    navit_safety_plan.c
    navit_safety_heat.c
    navit_safety_route.c
)

# SQLite backs the optional user-confirmation cache.
if(SQLITE3_FOUND)
    list(APPEND NAVIT_SAFETY_SOURCES navit_safety_cache.c)
    set(plugin_navit_safety_LIBS ${SQLITE3_LIBRARIES})
    set(plugin_navit_safety_INCLUDES ${SQLITE3_INCLUDE_DIRS})
    if(SQLITE3_CFLAGS_OTHER)
        set(plugin_navit_safety_FLAGS ${SQLITE3_CFLAGS_OTHER})
    endif()
endif()

module_add_library(plugin_navit_safety ${NAVIT_SAFETY_SOURCES})

# Scope the OSD guard to the plugin target only so it does not leak into the
# tests subdirectory (which compiles navit_safety.c with NAVIT_SAFETY_WITH_OSD=0).
# An explicit value keeps the arithmetic guard `#if NAVIT_SAFETY_WITH_OSD` unambiguous.
target_compile_definitions(plugin_navit_safety PRIVATE NAVIT_SAFETY_WITH_OSD=1)
if(SQLITE3_FOUND)
    target_compile_definitions(plugin_navit_safety PRIVATE NAVIT_SAFETY_WITH_SQLITE=1)
endif()

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
