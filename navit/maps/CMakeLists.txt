if(SAMPLE_MAP)
	set(SAMPLE_MAP_NAME osm_bbox_11.3,47.9,11.7,48.2)
	set(maptool_args "--attr-debug-level=5")

	add_custom_target(sample_map ALL DEPENDS ${SAMPLE_MAP_NAME}.xml)
	add_custom_command (
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.xml
		COMMAND ${CMAKE_COMMAND} -D TYPE=binfile -D DATA=${SAMPLE_MAP_NAME}.bin
		-D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.xml
		-P ${PROJECT_SOURCE_DIR}/cmake/navit_writemapxml.cmake
		VERBATIM
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
		)
	if(DOWNLOAD_SAMPLE_MAP)
		add_custom_command (
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
			COMMAND ${CMAKE_COMMAND}
			-D URL=http://www1.navit-project.org/maps/${SAMPLE_MAP_NAME}.bin
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
			-P ${PROJECT_SOURCE_DIR}/cmake/navit_download.cmake
			VERBATIM
			)
	else()
		add_custom_command (
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
			COMMAND bzcat ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
			| $<TARGET_FILE:maptool> ${maptool_args} ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
			VERBATIM
			DEPENDS maptool ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
			)
		add_custom_command (
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
			COMMAND ${CMAKE_COMMAND}
			-D URL=http://www1.navit-project.org/maps/${SAMPLE_MAP_NAME}.osm.bz2
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.osm.bz2
			-P ${PROJECT_SOURCE_DIR}/cmake/navit_download.cmake
			VERBATIM
			)
	endif()
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SAMPLE_MAP_NAME}.bin
		DESTINATION ${SHARE_DIR}/maps
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
endif()
