if(SAMPLE_MAP)
   set(SAMPLE_MAP_NAME osm_bbox_11.3,47.9,11.7,48.2)
   set(maptool_args "--attr-debug-level=5")

   GET_TARGET_PROPERTY(MAPTOOL_PATH maptool LOCATION)
   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
      COMMAND ${CMAKE_COMMAND} -D URL=http://www.navit-project.org/maps/${SAMPLE_MAP_NAME}.osm.bz2 -D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 -P ${PROJECT_SOURCE_DIR}/cmake/navit_download.cmake
   )
   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
      COMMAND bzcat ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 | \"${MAPTOOL_PATH}\" ${maptool_args} ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
      DEPENDS maptool ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
   )
   add_custom_target(sample_map ALL DEPENDS ${SAMPLE_MAP_NAME}.bin)
   add_dependencies(sample_map ${SAMPLE_MAP_NAME}.osm.bz2)
endif()
