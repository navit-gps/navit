# Unit tests for Navit Safety plugin

# navit_safety.c with the OSD path compiled out, exposing navit_safety_config_default
# without the Navit OSD/core symbols.
add_library(navit_safety_config_obj OBJECT
    ../navit_safety.c
)

target_compile_definitions(navit_safety_config_obj PRIVATE NAVIT_SAFETY_WITH_OSD=0)

target_include_directories(navit_safety_config_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Navit plugin modules are not added through CMake's normal add_subdirectory
# chain, so enable_testing() must be called here for ctest to pick up the tests
# from this directory (matches the other Navit plugin test directories).
enable_testing()

# Declare a Navit Safety test executable and register it with ctest.
function(navit_safety_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

navit_safety_add_test(test_navit_safety_config
    test_navit_safety_config.c
    $<TARGET_OBJECTS:navit_safety_config_obj>
)

navit_safety_add_test(test_navit_safety_confidence
    test_navit_safety_confidence.c
    ../navit_safety_confidence.c
)

navit_safety_add_test(test_navit_safety_koppen
    test_navit_safety_koppen.c
    ../navit_safety_koppen.c
)

navit_safety_add_test(test_navit_safety_lookahead
    test_navit_safety_lookahead.c
    ../navit_safety_lookahead.c
    ../navit_safety_confidence.c
)

navit_safety_add_test(test_navit_safety_plan
    test_navit_safety_plan.c
    ../navit_safety_plan.c
    ../navit_safety_koppen.c
    ../navit_safety_lookahead.c
    ../navit_safety_confidence.c
    $<TARGET_OBJECTS:navit_safety_config_obj>
)

navit_safety_add_test(test_navit_safety_heat
    test_navit_safety_heat.c
    ../navit_safety_heat.c
    $<TARGET_OBJECTS:navit_safety_config_obj>
)

navit_safety_add_test(test_navit_safety_route
    test_navit_safety_route.c
    ../navit_safety_route.c
    $<TARGET_OBJECTS:navit_safety_config_obj>
)

# The confirmation cache and its test only build when SQLite3 is available.
if(SQLITE3_FOUND)
    add_executable(test_navit_safety_cache
        test_navit_safety_cache.c
        ../navit_safety_cache.c
    )
    target_include_directories(test_navit_safety_cache PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${SQLITE3_INCLUDE_DIRS}
    )
    target_link_libraries(test_navit_safety_cache ${SQLITE3_LIBRARIES})
    add_test(NAME test_navit_safety_cache COMMAND test_navit_safety_cache)
endif()
