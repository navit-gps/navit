if(BUILD_MAPTOOL)

	enable_language(CXX)

	# ZLIB, EXPAT, BZip2 and Threads are located in the top level
	# CMakeLists.txt, which disables BUILD_MAPTOOL if one of them is absent.

	# libosmium and protozero are header-only. Prefer the system copies
	# (libosmium2-dev / libprotozero-dev on Debian and Ubuntu) so distro and
	# offline builds never hit the network; only fall back to downloading them.
	find_path(OSMIUM_INCLUDE_DIR osmium/version.hpp)
	find_path(PROTOZERO_INCLUDE_DIR protozero/version.hpp)
	if(OSMIUM_INCLUDE_DIR AND PROTOZERO_INCLUDE_DIR)
		include_directories(${OSMIUM_INCLUDE_DIR} ${PROTOZERO_INCLUDE_DIR})
	else()
		message(STATUS "libosmium or protozero not found on the system, downloading them")
		# SOURCE_SUBDIR points nowhere on purpose: we only want the headers,
		# not the libraries' own CMake projects.
		include(FetchContent)
		FetchContent_Declare(libosmium
			URL https://github.com/osmcode/libosmium/archive/refs/tags/v2.20.0.tar.gz
			SOURCE_SUBDIR non-existent)
		FetchContent_Declare(protozero
			URL https://github.com/mapbox/protozero/archive/refs/tags/v1.7.1.tar.gz
			SOURCE_SUBDIR non-existent)
		FetchContent_MakeAvailable(libosmium protozero)
		include_directories(${libosmium_SOURCE_DIR}/include ${protozero_SOURCE_DIR}/include)
	endif()

	add_definitions( -DMODULE=maptool ${NAVIT_COMPILE_FLAGS})

	add_executable (maptool maptool.c)
	add_library (maptool_core boundaries.c buffer.c ch.c coastline.c itembin.c
		itembin_buffer.c itembin_slicer.c misc.c osm.c osm_psql.c
		osm_relations.c sourcesink.c tempfile.c tile.c zip.c osm_libosmium.cpp)
	set_property(TARGET maptool_core PROPERTY CXX_STANDARD 14)
	target_link_libraries(maptool_core ZLIB::ZLIB EXPAT::EXPAT BZip2::BZip2 Threads::Threads)

	if(NOT MSVC)
		SET(NAVIT_LIBS ${NAVIT_LIBS} m)
	endif(NOT MSVC)

	target_link_libraries(maptool maptool_core ${NAVIT_LIBNAME} ${NAVIT_LIBS})

	install(TARGETS maptool
		DESTINATION ${BIN_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endif()
