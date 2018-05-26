
if(BUILD_MAPTOOL)
	add_definitions( -DMODULE=maptool ${NAVIT_COMPILE_FLAGS})
	include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	SET(MAPTOOL_SOURCE boundaries.c buffer.c ch.c coastline.c itembin.c itembin_buffer.c misc.c osm.c osm_o5m.c osm_psql.c osm_relations.c sourcesink.c tempfile.c tile.c zip.c osm_xml.c)
	if(NOT MSVC)
		SET(MAPTOOL_SOURCE ${MAPTOOL_SOURCE} osm_protobuf.c osm_protobufdb.c generated-code/fileformat.pb-c.c generated-code/osmformat.pb-c.c google/protobuf-c/protobuf-c.c)
	endif(NOT MSVC)
	add_library (maptool_core ${MAPTOOL_SOURCE})
	add_executable (maptool maptool.c)
	if(NOT MSVC)
		SET(NAVIT_LIBS ${NAVIT_LIBS} m)
	endif(NOT MSVC)
	target_link_libraries(maptool maptool_core ${NAVIT_LIBNAME} ${NAVIT_LIBS})

	install(TARGETS maptool
		DESTINATION ${BIN_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endif()
