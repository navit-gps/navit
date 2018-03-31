include_directories( "${PROJECT_BINARY_DIR}")
include_directories( "${PROJECT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_BINARY_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/support")

# navit core
set(NAVIT_SRC announcement.c atom.c attr.c cache.c callback.c command.c config_.c coord.c country.c data_window.c debug.c
   event.c file.c geom.c graphics.c gui.c item.c layout.c log.c main.c map.c maps.c
   linguistics.c mapset.c maptype.c menu.c messages.c bookmarks.c navit.c navit_nls.c navigation.c osd.c param.c phrase.c plugin.c popup.c
   profile.c profile_option.c projection.c roadprofile.c route.c script.c search.c speech.c start_real.c sunriset.c transform.c track.c
   search_houseno_interpol.c util.c vehicle.c vehicleprofile.c xmlconfig.c )

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
endif()


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
foreach (CURRENT_MODULE ${ALL_PLUGINS} ${ALL_MODULE_PLUGINS} ${ALL_MODULES})
   if (${CURRENT_MODULE})
      add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_MODULE}")
   endif()
endforeach()

add_subdirectory (fib-1.1)

if(NOT ANDROID)
   set(NAVIT_START_SRC start.c)
   if(WIN32 OR WINCE AND NOT WIN_OMIT_RESOURCES)
      list(APPEND NAVIT_START_SRC ${CMAKE_CURRENT_SOURCE_DIR}/gui/win32/resources/resource.rc)
   else()
      if (APPLE)
          set(NAVIT_START_SRC start_apple.m)
      endif()
   endif()
   if (BUILD_BUNDLE)
      list(APPEND NAVIT_START_SRC resources/share resources/Icon.png resources/Default.png)
      add_custom_command(OUTPUT resources/Icon.png COMMAND convert -scale 79x79 -crop 60x60+8+19 ${CMAKE_CURRENT_SOURCE_DIR}/icons/desktop_icons/128x128/navit.png resources/Icon.png)
      add_custom_command(OUTPUT resources/Default.png COMMAND convert -scale 79x79 -crop 60x60+8+19 ${CMAKE_CURRENT_SOURCE_DIR}/icons/desktop_icons/128x128/navit.png resources/Default.png)
   endif()
   add_executable(navit ${NAVIT_START_SRC})
   target_link_libraries (navit ${NAVIT_LIBNAME})
   if(DEFINED NAVIT_BINARY)
      set_target_properties(navit PROPERTIES OUTPUT_NAME ${NAVIT_BINARY})
   endif(DEFINED NAVIT_BINARY)
   if (BUILD_BUNDLE)
      add_custom_command(OUTPUT resources/share COMMAND mkdir -p resources/share)
      set_source_files_properties(resources/share resources/Icon.png resources/Default.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
      set_target_properties(navit PROPERTIES MACOSX_BUNDLE TRUE)
      set_target_properties(navit PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
      add_dependencies(navit navit_config_xml_resource)
      add_dependencies(navit images_resource)
      add_dependencies(navit locale_resource)
   endif()
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

ADD_CUSTOM_TARGET(
   git_version
   ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
                    -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.h
                    -D NAME="GIT_VERSION"
                    -P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
)

add_custom_command(
   OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
   COMMENT "Copy navit.dtd to ${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
   COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/navit/navit.dtd" "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
)

# additional parameter are passed to the stylesheet processor as parameter
macro(process_xslt SRC_DIR SRC_XML DEST_XML)
   set(XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/navit/${SRC_XML}" "${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML}" )
   if(XSL_PROCESSING)
      if(NOT XSLTS STREQUAL "")
         string(REPLACE "," ";" XSLTS "${XSLTS}")
         foreach(tmp ${XSLTS})
            set(XSLT_FILE "${PROJECT_SOURCE_DIR}/navit/xslt/${tmp}.xslt")
            list(APPEND XSLT_FILES "${XSLT_FILE}")
            list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E echo  Applying ${tmp}.xslt)
            compose_xslt_transform_command(CMD "${XSLT_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML}" "${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML}.tmp" "${ARGN}")
            list(APPEND XSLT_COMMANDS ${CMD})
            list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML}.tmp" ${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML})
         endforeach()
      endif()
   endif(XSL_PROCESSING)
   # Depend on all XSLT files, because the main XSLT file may pull in other files.
   # Ideally we'd parse the main XSLT file for includes, but that is tricky to do reliably.
   # Note that this list of files is only updated when (re)running CMake, so if files are
   # added/deleted, CMake must be re-run manually.
   file(GLOB ALL_XSLT_FILES "${PROJECT_SOURCE_DIR}/navit/xslt/*.xslt")
   #message(FATAL_ERROR ${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML})
   ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/navit/xslt/
      DEPENDS "${SRC_DIR}/${SRC_XML}" "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd" ${ALL_XSLT_FILES}
      ${XSLT_COMMANDS}
   )
endmacro()

if(ANDROID)
   set(XSLT_XXXHDPI_ARGS DPI_SUFFIX=xxxhdpi OSD_SIZE=5.33 ICON_SMALL=128 ICON_MEDIUM=192 ICON_BIG=256)
   set(XSLT_XXHDPI_ARGS DPI_SUFFIX=xxhdpi OSD_SIZE=4 ICON_SMALL=96 ICON_MEDIUM=128 ICON_BIG=192)
   set(XSLT_XHDPI_ARGS DPI_SUFFIX=xhdpi OSD_SIZE=2.67 ICON_SMALL=64 ICON_MEDIUM=96 ICON_BIG=128)
   set(XSLT_HDPI_ARGS DPI_SUFFIX=hdpi OSD_SIZE=2 ICON_SMALL=48 ICON_MEDIUM=64 ICON_BIG=96)
   set(XSLT_MDPI_ARGS DPI_SUFFIX=mdpi OSD_SIZE=1.33 ICON_SMALL=32 ICON_MEDIUM=48 ICON_BIG=64)
   set(XSLT_LDPI_ARGS DPI_SUFFIX=ldpi OSD_SIZE=1 ICON_SMALL=24 ICON_MEDIUM=32 ICON_BIG=48)
   
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike_ldpi.xml ${XSLT_LDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_xxxhdpi.xml ${XSLT_XXXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_xxhdpi.xml ${XSLT_XXHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_xhdpi.xml ${XSLT_XHDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_hdpi.xml ${XSLT_HDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_mdpi.xml ${XSLT_MDPI_ARGS})
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th_ldpi.xml ${XSLT_LDPI_ARGS})
   add_custom_target( navit_config_xml ALL DEPENDS navit_xxxhdpi.xml navit_xxhdpi.xml navit_xhdpi.xml navit_hdpi.xml navit_mdpi.xml navit_ldpi.xml navit_layout_car_xxxhdpi.xml navit_layout_car_xxhdpi.xml navit_layout_car_xhdpi.xml navit_layout_car_hdpi.xml navit_layout_car_mdpi.xml navit_layout_car_ldpi.xml navit_layout_car_dark_xxxhdpi.xml navit_layout_car_dark_xxhdpi.xml navit_layout_car_dark_xhdpi.xml navit_layout_car_dark_hdpi.xml navit_layout_car_dark_mdpi.xml navit_layout_car_dark_ldpi.xml navit_layout_car_android_xxxhdpi.xml navit_layout_car_android_xxhdpi.xml navit_layout_car_android_xhdpi.xml navit_layout_car_android_hdpi.xml navit_layout_car_android_mdpi.xml navit_layout_car_android_ldpi.xml navit_layout_car_simple_xxxhdpi.xml navit_layout_car_simple_xxhdpi.xml navit_layout_car_simple_xhdpi.xml navit_layout_car_simple_hdpi.xml navit_layout_car_simple_mdpi.xml navit_layout_car_simple_ldpi.xml navit_layout_bike_xxxhdpi.xml navit_layout_bike_xxxhdpi.xml navit_layout_bike_xxhdpi.xml navit_layout_bike_xhdpi.xml navit_layout_bike_hdpi.xml navit_layout_bike_mdpi.xml navit_layout_bike_ldpi.xml navit_layout_th_xxxhdpi.xml navit_layout_th_xxhdpi.xml navit_layout_th_xhdpi.xml navit_layout_th_hdpi.xml navit_layout_th_mdpi.xml navit_layout_th_ldpi.xml)
else()
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_shipped.xml navit.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_shipped.xml navit_layout_car.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_dark_shipped.xml navit_layout_car_dark.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_android_shipped.xml navit_layout_car_android.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_car_simple_shipped.xml navit_layout_car_simple.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_bike_shipped.xml navit_layout_bike.xml "")
   process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ navit_layout_th_shipped.xml navit_layout_th.xml "")
   add_custom_target( navit_config_xml_resource DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/navit.xml COMMAND mkdir -p resources/share/navit COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/navit.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_car.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_car_dark.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_car_android.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_car_simple.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_bike.xml ${CMAKE_CURRENT_BINARY_DIR}/navit_layout_th.xml resources/share/navit)
   add_custom_target( locale_resource DEPENDS locales COMMAND mkdir -p resources/share COMMAND cp -a ${CMAKE_CURRENT_BINARY_DIR}/../locale resources/share/locale)
   add_custom_target( navit_config_xml ALL DEPENDS navit.xml navit_layout_car.xml navit_layout_car_dark.xml navit_layout_car_android.xml navit_layout_car_simple.xml navit_layout_bike.xml navit_layout_th.xml)
endif()

ADD_DEPENDENCIES(${NAVIT_LIBNAME} git_version)
if (USE_LIBGNUINTL AND NOT HAVE_GLIB)
   ADD_DEPENDENCIES(support_glib support_gettext_intl)
endif()


add_subdirectory (maptool)
add_subdirectory (icons)
add_subdirectory (maps)
if(ANDROID)
   add_subdirectory (android)
endif()

install(TARGETS navit
        DESTINATION ${BIN_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/navit.xml
        DESTINATION ${SHARE_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

get_directory_property(INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
WRITE_FILE("cmake_plugin_settings.txt"
   "set(APPLE ${APPLE} CACHE BOOL init)\n" 
   "set(ANDROID ${ANDROID} CACHE BOOL init)\n"
   "set(USE_PLUGINS ${USE_PLUGINS} CACHE BOOL init)\n"
   "set(MODULE_BUILD_TYPE \"${MODULE_BUILD_TYPE}\" CACHE STRING init)\n"
   "set(NAVIT_COMPILE_FLAGS \"${NAVIT_COMPILE_FLAGS}\" CACHE STRING init)\n"
   "set(navit_SOURCE_DIR \"${navit_SOURCE_DIR}\" CACHE STRING init)\n"
   "set(NAVIT_LIBNAME \"${NAVIT_LIBNAME}\" CACHE STRING init)\n"
   "set(ANDROID_API_VERSION \"${ANDROID_API_VERSION}\" CACHE STRING init)\n"
   "set(ANDROID_NDK_API_VERSION \"${ANDROID_NDK_API_VERSION}\" CACHE STRING init)\n"
   "set(CMAKE_TOOLCHAIN_FILE \"${CMAKE_TOOLCHAIN_FILE}\" CACHE STRING init)\n"
   "set(INCLUDE_DIRECTORIES \"${INCLUDE_DIRECTORIES}\" CACHE STRING init)\n"
   "set(LIB_DIR \"${LIB_DIR}\" CACHE STRING init)\n"
   "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\" CACHE STRING init)\n"
)
