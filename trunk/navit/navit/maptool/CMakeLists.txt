
if(BUILD_MAPTOOL)
   add_definitions( -DMODULE=maptool )
   add_library (maptool_core boundaries.c buffer.c ch.c coastline.c geom.c itembin.c itembin_buffer.c misc.c osm.c sourcesink.c tempfile.c tile.c zip.c )
   add_executable (maptool maptool.c)
   target_link_libraries(maptool maptool_core navit_core ${NAVIT_LIBS} m)

   install(TARGETS maptool
           DESTINATION ${BIN_DIR}
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
   
endif()
