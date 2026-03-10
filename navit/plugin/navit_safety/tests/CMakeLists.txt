# Unit tests for Navit Safety plugin

add_library(navit_safety_config_obj OBJECT
    ../navit_safety.c
)

target_compile_definitions(navit_safety_config_obj PRIVATE NAVIT_SAFETY_WITH_OSD=0)

target_include_directories(navit_safety_config_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(test_navit_safety_config
    test_navit_safety_config.c
    $<TARGET_OBJECTS:navit_safety_config_obj>
)

target_link_libraries(test_navit_safety_config
    glib-2.0
    m
)

target_include_directories(test_navit_safety_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

enable_testing()

add_test(NAME navit_safety_config_test COMMAND test_navit_safety_config)
