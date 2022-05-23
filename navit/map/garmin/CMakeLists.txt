add_executable(gentypes gentypes.c)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})

# add the command to generate the source code
add_custom_command (
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/g2nbuiltin.h
	COMMAND gentypes ${CMAKE_CURRENT_SOURCE_DIR}/garmintypes.txt ${CMAKE_CURRENT_BINARY_DIR}/g2nbuiltin.h
	DEPENDS gentypes
	)

module_add_library(map_garmin garmin.c gar2navit.c g2nbuiltin.h)
