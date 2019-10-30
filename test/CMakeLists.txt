file(GLOB TEST_FILES_SOURCES "*.cpp")

add_executable(unit_tests ${TEST_FILES_SOURCES})

target_link_libraries(unit_tests navit)

add_test(
	NAME unit
	COMMAND ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
	)
