if(SAMPLE_MAP)
   set(SAMPLE_MAP_NAME osm_bbox_11.3,47.9,11.7,48.2)
   set(maptool_args "--attr-debug-level=5")

   message(STATUS "Downloading osm map")
   file(DOWNLOAD http://www.navit-project.org/maps/${SAMPLE_MAP_NAME}.osm.bz2 ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 SHOW_PROGRESS)

   GET_TARGET_PROPERTY(MAPTOOL_PATH maptool LOCATION)
   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
      COMMAND bzcat ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 | \"${MAPTOOL_PATH}\" ${maptool_args} ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
      DEPENDS maptool ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
   )
   add_custom_target(sample_map ALL DEPENDS ${SAMPLE_MAP_NAME}.bin)
endif()
