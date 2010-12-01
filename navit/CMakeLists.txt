include_directories( "${PROJECT_BINARY_DIR}")
include_directories( "${PROJECT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_BINARY_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/support")

message("" )
message("Summary:\n")

foreach ( PLUGIN_PATH ${ALL_PLUGINS})
   if (${PLUGIN_PATH})
      message("Enabled   ${PLUGIN_PATH} ( ${${PLUGIN_PATH}_REASON} )")
      
      include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_PATH}")
      
      string(REPLACE "/" "_" MODULE_INC ${PLUGIN_PATH})
      list(APPEND NAVIT_SUPPORT_LIBS ${MODULE_INC})
   else()
      message("Disabled  ${PLUGIN_PATH} ( ${${PLUGIN_PATH}_REASON} )")
   endif()
endforeach()

foreach ( MODULE_PATH ${ALL_MODULES})
   if ( ${MODULE_PATH} )
      message("Enabled   ${MODULE_PATH} ( ${${MODULE_PATH}_REASON} )")
      
      include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_PATH}")
      
      string(REPLACE "/" "_" MODULE_INC ${MODULE_PATH})
      list(APPEND MODULES_PROTOTYPE "\nvoid\tmodule_${MODULE_INC}_init(void)")
      list(APPEND MODULES_FUNC "\nmodule_${MODULE_INC}_init()")
      
      if (NOT USE_PLUGINS)
         list(APPEND MODULES_NAME ${MODULE_INC})
      else()
         # set plugin_type
         string(REGEX REPLACE "(^[^/]*)/.*" "\\1" ${MODULE_INC}_TYPE ${MODULE_PATH})
      endif()
   else()
      message("Disabled  ${MODULE_PATH} ( ${${MODULE_PATH}_REASON} )")
   endif()
endforeach()
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/builtin.c.in ${CMAKE_CURRENT_BINARY_DIR}/builtin.c)


message( "")

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/fib-1.1")

# Add module paths
foreach (CURRENT_MODULE ${ALL_PLUGINS} ${ALL_MODULES})
   if (${CURRENT_MODULE})
      add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_MODULE}")
   endif()
endforeach()

add_subdirectory (fib-1.1)

# navit cre
set(NAVIT_SRC announcement.c atom.c attr.c cache.c callback.c command.c compass.c config_.c coord.c country.c data_window.c debug.c 
	event.c file.c graphics.c gui.c item.c layout.c log.c main.c map.c 
	linguistics.c mapset.c maptype.c menu.c messages.c bookmarks.c navit.c navigation.c osd.c param.c phrase.c plugin.c popup.c
	profile.c projection.c roadprofile.c route.c routech.c search.c speech.c start_real.c sunriset.c transform.c track.c 
	util.c vehicle.c vehicleprofile.c xmlconfig.c )

if(NOT USE_PLUGINS)
  list(APPEND NAVIT_SRC  ${CMAKE_CURRENT_BINARY_DIR}/builtin.c)
endif(NOT USE_PLUGINS)

if (${HAVE_GLIB})
  list(APPEND NAVIT_SRC event_glib.c)
endif()

add_library (navit_core ${NAVIT_SRC} )
target_link_libraries(navit_core  ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} fib ${NAVIT_LIBS} m) # support_glib support_zlib support_win32 support_libpng support_wordexp 
set_target_properties(navit_core PROPERTIES COMPILE_DEFINITIONS "MODULE=navit;LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${LIB_DIR}\";PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
if (DEFINED NAVIT_COMPILE_FLAGS)
   set_target_properties(navit_core PROPERTIES COMPILE_FLAGS ${NAVIT_COMPILE_FLAGS})
endif()

# Subversion revision
ADD_CUSTOM_TARGET(
    version
    cd ${CMAKE_CURRENT_SOURCE_DIR} && ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
                     -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.h
                     -P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
)

ADD_CUSTOM_COMMAND(
  OUTPUT navit.xml
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/navit_shipped.xml
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/navit_shipped.xml ${CMAKE_CURRENT_BINARY_DIR}/navit.xml
)


#configure_file (${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
ADD_DEPENDENCIES(navit_core version)

add_executable (navit start.c navit.xml)
target_link_libraries (navit navit_core)
set_target_properties(navit PROPERTIES COMPILE_DEFINITIONS "MODULE=${MODULE_NAME}")

add_subdirectory (maptool)
add_subdirectory (xpm)

install(TARGETS navit
        DESTINATION ${BIN_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/navit.xml
        DESTINATION ${SHARE_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
