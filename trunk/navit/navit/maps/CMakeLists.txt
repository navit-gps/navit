if(SAMPLE_MAP)
   set(SAMPLE_MAP_NAME osm_bbox_11.3,47.9,11.7,48.2)
   set(maptool_args "--attr-debug-level=5")

   GET_TARGET_PROPERTY(MAPTOOL_PATH maptool LOCATION)
   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.xml
      COMMAND ${CMAKE_COMMAND} -D TYPE=binfile -D DATA=${SAMPLE_MAP_NAME}.bin -D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.xml -P ${PROJECT_SOURCE_DIR}/cmake/navit_writemapxml.cmake
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
   )
   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
      COMMAND ${CMAKE_COMMAND} -D URL=http://www.navit-project.org/maps/${SAMPLE_MAP_NAME}.osm.bz2 -D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 -P ${PROJECT_SOURCE_DIR}/cmake/navit_download.cmake
   )
   if(CMAKE_CROSSCOMPILING OR NOT BZCAT)
      add_custom_command (
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
         COMMAND ${CMAKE_COMMAND} -D URL=http://www.navit-project.org/maps/${SAMPLE_MAP_NAME}.bin -D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin -P ${PROJECT_SOURCE_DIR}/cmake/navit_download.cmake
      )
   else()
      add_custom_command (
         OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
         COMMAND bzcat ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2 | \"${MAPTOOL_PATH}\" ${maptool_args} ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
         DEPENDS maptool ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
      )
   endif()
   add_custom_target(sample_map ALL DEPENDS ${SAMPLE_MAP_NAME}.xml)
   add_dependencies(sample_map ${SAMPLE_MAP_NAME}.bin)
   if(NOT CMAKE_CROSSCOMPILING AND BZCAT)
      add_dependencies(sample_map ${SAMPLE_MAP_NAME}.osm.bz2)
   endif()
endif()
