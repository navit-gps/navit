add_executable(gentypes gentypes.c)

# add the command to generate the source code
add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/g2nbuiltin.h
  COMMAND gentypes ${CMAKE_CURRENT_SOURCE_DIR}/garmintypes.txt ${CMAKE_CURRENT_BINARY_DIR}/g2nbuiltin.h
  DEPENDS gentypes
  )

module_add_library(map_garmin garmin.c gar2navit.c g2nbuiltin.h)
set_property(
   TARGET map_garmin
   APPEND PROPERTY COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}"
)
