include_directories( "${PROJECT_BINARY_DIR}")
include_directories( "${PROJECT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_BINARY_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/support")

foreach ( PLUGIN_PATH ${ALL_PLUGINS})
   if (${PLUGIN_PATH})
      include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_PATH}")
      
      string(REPLACE "/" "_" MODULE_INC ${PLUGIN_PATH})
      list(APPEND NAVIT_SUPPORT_LIBS ${MODULE_INC})
   endif()
endforeach()

foreach ( MODULE_PATH ${ALL_MODULES})
   if ( ${MODULE_PATH} )
      include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_PATH}")
      
      string(REPLACE "/" "_" MODULE_INC ${MODULE_PATH})
      list(APPEND MODULES_PROTOTYPE "\nvoid\tmodule_${MODULE_INC}_init(void)")
      list(APPEND MODULES_FUNC "\nmodule_${MODULE_INC}_init()")
      
      if (NOT USE_PLUGINS)
         list(APPEND MODULES_NAME ${MODULE_INC})
      else()
         string(REGEX REPLACE "(^[^/]*)/.*" "\\1" ${MODULE_INC}_TYPE ${MODULE_PATH})
      endif()
   endif()
endforeach()

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/builtin.c.in ${CMAKE_CURRENT_BINARY_DIR}/builtin.c)

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

if(ANDROID)
   list(APPEND NAVIT_SRC android.c)
   set(NAVIT_LIBNAME navit)
else()
   set(NAVIT_LIBNAME navit_core)
   add_executable (navit start.c)
   target_link_libraries (navit ${NAVIT_LIBNAME})
   set_target_properties(navit PROPERTIES COMPILE_DEFINITIONS "MODULE=${MODULE_NAME}")
endif()

if (SHARED_LIBNAVIT)
  add_library (${NAVIT_LIBNAME} SHARED ${NAVIT_SRC} )
else(SHARED_LIBNAVIT)
  add_library (${NAVIT_LIBNAME} STATIC ${NAVIT_SRC} )
endif(SHARED_LIBNAVIT)
if(NOT MSVC)
    SET(NAVIT_LIBS ${NAVIT_LIBS} m)
endif(NOT MSVC)
target_link_libraries(${NAVIT_LIBNAME}  ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} fib ${NAVIT_LIBS} ) 
set_target_properties(${NAVIT_LIBNAME} PROPERTIES COMPILE_DEFINITIONS "MODULE=navit;LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${LIB_DIR}\";PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
if (DEFINED NAVIT_COMPILE_FLAGS)
   set_target_properties(${NAVIT_LIBNAME} PROPERTIES COMPILE_FLAGS "${NAVIT_COMPILE_FLAGS}")
endif()

# Subversion revision
ADD_CUSTOM_TARGET(
   version
   ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
                    -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.h
		    -D NAME=SVN_VERSION
                    -P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
)


set(XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/navit_shipped.xml ${CMAKE_CURRENT_BINARY_DIR}/navit.xml.new)
list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/navit.dtd ${CMAKE_CURRENT_BINARY_DIR}/navit.dtd)
if(XSL_PROCESSING AND XSLTS)
    string(REPLACE "," ";" XSLTS "${XSLTS}")
    foreach(tmp ${XSLTS})
       set (XSLT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/xslt/${tmp}.xslt")
       list(APPEND XSLT_FILES "${XSLT_FILES}")
       list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E echo  Applying ${tmp}.xslt)
       list(APPEND XSLT_COMMANDS COMMAND ${SAXON} ${CMAKE_CURRENT_BINARY_DIR}/navit.xml.new ${XSLT_FILE} >${CMAKE_CURRENT_BINARY_DIR}/navit.xml.tmp)
       list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/navit.xml.tmp ${CMAKE_CURRENT_BINARY_DIR}/navit.xml.new)
    endforeach()
endif()
list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/navit.xml.new ${CMAKE_CURRENT_BINARY_DIR}/navit.xml)

ADD_CUSTOM_COMMAND(
   OUTPUT navit.xml
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/navit_shipped.xml ${XSLT_FILES}
   ${XSLT_COMMANDS}
)

add_custom_target( navit_config_xml ALL DEPENDS navit.xml)

ADD_DEPENDENCIES(${NAVIT_LIBNAME} version)
if (USE_LIBGNUINTL AND NOT HAVE_GLIB)
   ADD_DEPENDENCIES(support_glib intl_cmake)
endif()


add_subdirectory (maptool)
add_subdirectory (xpm)
add_subdirectory (maps)
add_subdirectory (android)

install(TARGETS navit
        DESTINATION ${BIN_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/navit.xml
        DESTINATION ${SHARE_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
