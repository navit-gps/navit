file(GLOB TEST_FILES_SOURCES "*.cpp")

set_property(TARGET navit PROPERTY ENABLE_EXPORTS 1)

include_directories( "${PROJECT_BINARY_DIR}")
include_directories( "${PROJECT_SOURCE_DIR}")
include_directories( "${PROJECT_BINARY_DIR}/navit")
include_directories( "${PROJECT_SOURCE_DIR}/navit")

add_executable(unit_tests "")

target_sources(
	unit_tests
	PRIVATE
		${TEST_FILES_SOURCES}
)

target_link_libraries(unit_tests PRIVATE navit GTest::gtest_main )

include(GoogleTest)
gtest_discover_tests(unit_tests)
