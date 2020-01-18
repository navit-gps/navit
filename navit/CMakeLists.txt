include_directories( "${PROJECT_BINARY_DIR}")
include_directories( "${PROJECT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories( "${CMAKE_CURRENT_BINARY_DIR}")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/support")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/THIS_IS_THE_NAVIT_WORKING_DIR "\r\nThis File should never appear on binary distributions\r\n" )
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/THIS_IS_THE_NAVIT_WORKING_DIR "\r\nOnly Devs should ever see this file" )

# navit core
set(NAVIT_SRC announcement.c atom.c attr.c cache.c callback.c command.c config_.c coord.c country.c data_window.c debug.c
	event.c file.c geom.c graphics.c gui.c item.c layout.c log.c main.c map.c maps.c
	linguistics.c mapset.c maptype.c menu.c messages.c bookmarks.c navit.c navit_nls.c navigation.c osd.c param.c phrase.c plugin.c popup.c
	profile.c profile_option.c projection.c roadprofile.c route.c script.c search.c speech.c start_real.c sunriset.c transform.c track.c
	search_houseno_interpol.c traffic.c util.c vehicle.c vehicleprofile.c xmlconfig.c )

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
target_link_libraries(${NAVIT_LIBNAME} ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} fib ${NAVIT_LIBS} )
set_target_properties(${NAVIT_LIBNAME} PROPERTIES COMPILE_DEFINITIONS "MODULE=navit;LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${LIB_DIR}\";PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
if (DEFINED NAVIT_COMPILE_FLAGS)
	set_target_properties(${NAVIT_LIBNAME} PROPERTIES COMPILE_FLAGS "${NAVIT_COMPILE_FLAGS}")
endif()

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
	COMMENT "Copy navit.dtd to ${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
	COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/navit/navit.dtd" "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd"
	)

# additional parameter are passed to the stylesheet processor as parameter
macro(process_xslt SRC_DIR SRC_XML DEST_DIR DEST_XML)
	set(TMPFOLDER "${DEST_DIR}/${DEST_XML}")
	# Create a unique temporary name based on the PATH and filename of the target xml file
	string(REPLACE "/" "_" TMPFOLDER ${TMPFOLDER})
	string(REPLACE ":" "_" TMPFOLDER ${TMPFOLDER})
	string(REPLACE "\\" "_" TMPFOLDER ${TMPFOLDER})
	# Create this unique temporary directory inside ${CMAKE_CURRENT_BINARY_DIR}. This is where we will store our intermediate xml file (called ${SRC_XML}.temp after each xslt pass
	set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/${TMPFOLDER}")
	set(XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${TMPDIR}")
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/navit/${SRC_XML}" "${TMPDIR}/${SRC_XML}.temp")
	# Also copy the dtd file in the same directory as the xml file (required by the xslt processor)
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/navit/navit.dtd" "${TMPDIR}/" )
	if(XSL_PROCESSING)
		if(NOT XSLTS STREQUAL "")
			string(REPLACE "," ";" XSLTS "${XSLTS}")
			foreach(tmp ${XSLTS})
				# We will run this loop below for each xslt file provided in the XSLTS variable
				set(XSLT_FILE "${PROJECT_SOURCE_DIR}/navit/xslt/${tmp}.xslt")
				list(APPEND XSLT_FILES "${XSLT_FILE}")
				list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E echo  Applying ${tmp}.xslt)
				compose_xslt_transform_command(CMD "${XSLT_FILE}" "${TMPDIR}/${SRC_XML}.temp" "${TMPDIR}/${SRC_XML}.output" "${ARGN}")
				list(APPEND XSLT_COMMANDS ${CMD})
				list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E rename "${TMPDIR}/${SRC_XML}.output" "${TMPDIR}/${SRC_XML}.temp")
			endforeach()
		endif()
	endif(XSL_PROCESSING)
	# Move the final resulting xml file to the target filename and directory (provided in arguments)
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR})
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E rename "${TMPDIR}/${SRC_XML}.temp" "${DEST_DIR}/${DEST_XML}")
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E remove "${TMPDIR}/navit.dtd")
	list(APPEND XSLT_COMMANDS COMMAND ${CMAKE_COMMAND} -E remove_directory "${TMPDIR}")
	# Depend on all XSLT files, because the main XSLT file may pull in other files.
	# Ideally we'd parse the main XSLT file for includes, but that is tricky to do reliably.
	# Note that this list of files is only updated when (re)running CMake, so if files are
	# added/deleted, CMake must be re-run manually.
	file(GLOB ALL_XSLT_FILES "${PROJECT_SOURCE_DIR}/navit/xslt/*.xslt")
	#message(FATAL_ERROR ${CMAKE_CURRENT_BINARY_DIR}/${DEST_XML})
	ADD_CUSTOM_COMMAND(
		OUTPUT ${DEST_DIR}/${DEST_XML}
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/navit/xslt/
		DEPENDS "${SRC_DIR}/${SRC_XML}" "${CMAKE_CURRENT_BINARY_DIR}/navit.dtd" ${ALL_XSLT_FILES}
		${XSLT_COMMANDS}
		)
endmacro()

# Give instructions on how to build all navit_*.xml navit_layout_*.xml files, obtained applying relevant xslt tranformation on the source navit_shipped_*.xml files
# The following XMLCFG_ALL_FILE_PREFIXES variable below lists all xml config files, semicolon is the delimiter
# For example, it contains layout_navit_car, that will then be used to guess the source filename navit_layout_car_shipped.xml, installed as navit_layout_car.xml (or on android DPI-related files like navit_layout_car_xxxhdpi.xml)
set(XMLCFG_ALL_FILE_PREFIXES "navit;navit_layout_car;navit_layout_car_dark;navit_layout_car_android;navit_layout_car_simple;navit_layout_bike;navit_layout_th")
if(ANDROID)
	# On android, additional xlst transformations are applied to scale the OSD, icons etc... and the size is selected depending on the definition of the screen of the device (xxxhdpi, xxhdpi, ... ldpi)
	# Files for all screen definitions are shipped inside the android package (suffixed with _xxxhdpi, _xxhdpi etc.), and it is only at runtime that the correct navit_*.xml (corresponding to the current device's resolution) is selected
	set(XSLT_XXXHDPI_ARGS OSD_SIZE=5.33 ICON_SMALL=128 ICON_MEDIUM=192 ICON_BIG=256)
	set(XSLT_XXHDPI_ARGS OSD_SIZE=4 ICON_SMALL=96 ICON_MEDIUM=128 ICON_BIG=192)
	set(XSLT_XHDPI_ARGS OSD_SIZE=2.67 ICON_SMALL=64 ICON_MEDIUM=96 ICON_BIG=128)
	set(XSLT_HDPI_ARGS OSD_SIZE=2 ICON_SMALL=48 ICON_MEDIUM=64 ICON_BIG=96)
	set(XSLT_MDPI_ARGS OSD_SIZE=1.33 ICON_SMALL=32 ICON_MEDIUM=48 ICON_BIG=64)
	set(XSLT_LDPI_ARGS OSD_SIZE=1 ICON_SMALL=24 ICON_MEDIUM=32 ICON_BIG=48)

	set(XMLCFG_OUTPUT_LIST "")	# Used to build the list of generated *xml filenames (only file basename, directory part excluded)
	foreach(XMLCFG_FILE_PREFIX ${XMLCFG_ALL_FILE_PREFIXES})
		set(XMLCFG_FILE_SHIPPED ${XMLCFG_FILE_PREFIX}_shipped.xml)
		set(XMLCFG_DIR_OUTPUT_XXXHDPI ${CMAKE_CURRENT_BINARY_DIR}/config/xxxhdpi)
		set(XMLCFG_DIR_OUTPUT_XXHDPI ${CMAKE_CURRENT_BINARY_DIR}/config/xxhdpi)
		set(XMLCFG_DIR_OUTPUT_XHDPI ${CMAKE_CURRENT_BINARY_DIR}/config/xhdpi)
		set(XMLCFG_DIR_OUTPUT_HDPI ${CMAKE_CURRENT_BINARY_DIR}/config/hdpi)
		set(XMLCFG_DIR_OUTPUT_MDPI ${CMAKE_CURRENT_BINARY_DIR}/config/mdpi)
		set(XMLCFG_DIR_OUTPUT_LDPI ${CMAKE_CURRENT_BINARY_DIR}/config/ldpi)
		set(XMLCFG_FILE_OUTPUT ${XMLCFG_FILE_PREFIX}.xml)
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_XXXHDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_XXXHDPI_ARGS})
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_XXHDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_XXHDPI_ARGS})
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_XHDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_XHDPI_ARGS})
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_HDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_HDPI_ARGS})
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_MDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_MDPI_ARGS})
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${XMLCFG_DIR_OUTPUT_LDPI}/ ${XMLCFG_FILE_PREFIX}.xml ${XSLT_LDPI_ARGS})
		set(XMLCFG_OUTPUT_LIST ${XMLCFG_OUTPUT_LIST} ${XMLCFG_DIR_OUTPUT_XXXHDPI}/${XMLCFG_FILE_PREFIX}.xml ${XMLCFG_DIR_OUTPUT_XXHDPI}/${XMLCFG_FILE_PREFIX}.xml ${XMLCFG_DIR_OUTPUT_XHDPI}/${XMLCFG_FILE_PREFIX}.xml ${XMLCFG_DIR_OUTPUT_HDPI}/${XMLCFG_FILE_PREFIX}.xml ${XMLCFG_DIR_OUTPUT_MDPI}/${XMLCFG_FILE_PREFIX}.xml ${XMLCFG_DIR_OUTPUT_LDPI}/${XMLCFG_FILE_PREFIX}.xml)
	endforeach()
	add_custom_target( navit_config_xml ALL DEPENDS ${XMLCFG_OUTPUT_LIST})
else()
	set(XMLCFG_OUTPUT_LIST "")	# Used to build the list of generated *xml filenames (only file basename, directory part excluded)
	set(XMLCFG_INSTALLED_PATH_LIST "")	# A list of full path to installed xml files
	foreach(XMLCFG_FILE_PREFIX ${XMLCFG_ALL_FILE_PREFIXES})
		set(XMLCFG_FILE_SHIPPED ${XMLCFG_FILE_PREFIX}_shipped.xml)
		process_xslt(${CMAKE_CURRENT_SOURCE_DIR}/ ${XMLCFG_FILE_SHIPPED} ${CMAKE_CURRENT_BINARY_DIR}/ ${XMLCFG_FILE_PREFIX}.xml "")
		set(XMLCFG_OUTPUT_LIST ${XMLCFG_OUTPUT_LIST} ${XMLCFG_FILE_PREFIX}.xml)
		set(XMLCFG_INSTALLED_PATH_LIST ${XMLCFG_INSTALLED_PATH_LIST} ${CMAKE_CURRENT_BINARY_DIR}/${XMLCFG_FILE_PREFIX}.xml)
	endforeach()
	add_custom_target( navit_config_xml_resource DEPENDS ${XMLCFG_INSTALLED_PATH_LIST} COMMAND mkdir -p resources/share/navit COMMAND cp ${XMLCFG_INSTALLED_PATH_LIST} resources/share/navit)
	add_custom_target( locale_resource DEPENDS locales COMMAND mkdir -p resources/share COMMAND cp -a ${CMAKE_CURRENT_BINARY_DIR}/../locale resources/share/locale)
	add_custom_target( navit_config_xml ALL DEPENDS ${XMLCFG_OUTPUT_LIST})
endif()

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

foreach(XMLCFG_INSTALLED_PATH ${XMLCFG_INSTALLED_PATH_LIST})
	install(FILES ${XMLCFG_INSTALLED_PATH}
		DESTINATION ${SHARE_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
endforeach()

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
