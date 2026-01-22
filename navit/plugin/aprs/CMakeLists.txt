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

module_add_library(plugin_aprs aprs.c aprs_db.c aprs_decode.c aprs_osd.c aprs_nmea.c aprs_symbols.c)

# Export symbols for inter-plugin communication
set_target_properties(plugin_aprs PROPERTIES
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN 0
)

# Add unit tests if enabled
option(BUILD_APRS_TESTS "Build APRS plugin unit tests" ON)
if(BUILD_APRS_TESTS)
    add_subdirectory(tests)
endif()

# Install APRS symbols if available
# Symbols should be placed in navit/plugin/aprs/symbols/48x48/primary/ and alternate/
set(APRS_SYMBOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/symbols")
if(EXISTS "${APRS_SYMBOLS_DIR}/48x48/primary")
    install(
        DIRECTORY "${APRS_SYMBOLS_DIR}/48x48/"
        DESTINATION "${IMAGE_DIR}/../aprs-symbols/48x48"
        FILES_MATCHING PATTERN "*.png"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
    message(STATUS "APRS symbols will be installed from ${APRS_SYMBOLS_DIR}")
else()
    message(STATUS "APRS symbols directory not found at ${APRS_SYMBOLS_DIR}/48x48/primary")
    message(STATUS "To include symbols, download from https://github.com/hessu/aprs-symbols")
    message(STATUS "and place 48x48/primary/ and 48x48/alternate/ in navit/plugin/aprs/symbols/")
endif()

