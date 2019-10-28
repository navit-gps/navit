if(BUILD_MAPTOOL)

	find_package(Protobuf-c REQUIRED)
	include_directories(${PROTOBUF-C_INCLUDE_DIR})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	#PROTOBUF_C_GENERATE_C(PROTO_SRCS PROTO_HDRS zfs.proto)

	add_definitions( -DMODULE=maptool ${NAVIT_COMPILE_FLAGS})

	add_executable (maptool maptool.c)
	add_library (maptool_core boundaries.c buffer.c ch.c coastline.c itembin.c
		itembin_buffer.c itembin_slicer.c misc.c osm.c osm_o5m.c osm_psql.c
		osm_relations.c sourcesink.c tempfile.c tile.c zip.c osm_xml.c)

	if(NOT MSVC)
		PROTOBUF_C_GENERATE_C (PROTO_SRCS PROTO_HDRS osmformat.proto)
		PROTOBUF_C_GENERATE_C (PROTO_SRCS PROTO_HDRS fileformat.proto)
		target_sources(maptool_core PRIVATE osm_protobuf.c osm_protobufdb.c
			fileformat.pb-c.c osmformat.pb-c.c)
		target_link_libraries(maptool_core ${PROTOBUF_C_LIBRARY})
	endif(NOT MSVC)

	if(NOT MSVC)
		SET(NAVIT_LIBS ${NAVIT_LIBS} m)
	endif(NOT MSVC)

	target_link_libraries(maptool maptool_core ${NAVIT_LIBNAME} ${NAVIT_LIBS})

	install(TARGETS maptool
		DESTINATION ${BIN_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endif()
