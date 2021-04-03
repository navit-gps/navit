macro(convert_to_png IMAGE_INPUT IMAGE_OUTPUT SCALE)
	get_filename_component(FILE_EXT ${IMAGE_INPUT} EXT)
	string(TOUPPER ${FILE_EXT} FILE_TYPE)
	string(REGEX REPLACE "^\\." "" FILE_TYPE ${FILE_TYPE})
	if(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "rsvg-convert")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS --width=${SCALE} --height=${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS ${SCALE_ARGS} --output ${IMAGE_OUTPUT} ${IMAGE_INPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "convert")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS -resize ${SCALE}x${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS -background none ${IMAGE_INPUT} ${SCALE_ARGS} ${IMAGE_OUTPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "ksvgtopng")
		set (NEW_SCALE ${SCALE})
		if (${SCALE} EQUAL 0)
			file(STRINGS  ${IMAGE_INPUT} NEW_SCALE_LINE REGEX "[^-]width=\"[0-9pxt.]*\"")
			string(REGEX MATCH "width=\"([0-9]*)[pxt]*\"" NEW_SCALE_LINE ${NEW_SCALE_LINE})
			set(NEW_SCALE ${CMAKE_MATCH_1})
		endif()
		set(COMMAND_ARGS ${NEW_SCALE} ${NEW_SCALE}  ${IMAGE_INPUT} ${IMAGE_OUTPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "inkscape")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS --export-width=${SCALE} --export-height=${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS --export-type=png --export-background-opacity=0 ${SCALE_ARGS} --export-filename=${IMAGE_OUTPUT} ${IMAGE_INPUT})
	else()
		message(FATAL_ERROR "no svg2png converter defined here.")
	endif()

	add_custom_command (
		OUTPUT ${IMAGE_OUTPUT}
		DEPENDS ${IMAGE_INPUT}
		COMMAND ${IMAGE_CONVERTER_${FILE_TYPE}} ${COMMAND_ARGS}
		)
endmacro()

macro(add_scale_targets IMAGE_SRC IMAGE_NAME)
	string(REPLACE "," ";" SCALES "${ARGN}")
	set(add_scale_targets_is_first_scale 1)
	foreach (CURRENT_SCALE ${SCALES})
		if(add_scale_targets_is_first_scale EQUAL 1)
			set (IMAGE_DST ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}.png)
			set(add_scale_targets_is_first_scale 0)
		elseif (${CURRENT_SCALE} EQUAL 0)
			set (IMAGE_DST ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}.png)
			message_error( "Image size value of 0 to leave image unscaled can only be given as the first value in row, e.g. -D svg2png_scaling=0,16,32 " )
		else()
			set (IMAGE_DST ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}_${CURRENT_SCALE}_${CURRENT_SCALE}.png)
		endif()

		if(NOT CURRENT_SCALE EQUAL -1)
			convert_to_png(
				${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_SRC}
				${IMAGE_DST}
				${CURRENT_SCALE}
				)
			list(APPEND IMAGE_TARGETS ${IMAGE_DST})
		endif()
	endforeach()
endmacro()

if(NOT DEFINED NAVIT_BINARY)
	set(NAVIT_BINARY navit CACHE STRING "Navit binary name  not defined")
endif()
#patch binary name in desktop file
configure_file(desktop_icons/navit.desktop.in desktop_icons/${PACKAGE}.desktop)
install(FILES  ${CMAKE_CURRENT_BINARY_DIR}/desktop_icons/${PACKAGE}.desktop DESTINATION share/applications)
install(FILES  desktop_icons/22x22/navit.png DESTINATION share/icons/hicolor/22x22/apps RENAME ${PACKAGE}.png)
install(FILES  desktop_icons/86x86/navit.png DESTINATION share/icons/hicolor/86x86/apps RENAME ${PACKAGE}.png)
install(FILES  desktop_icons/108x108/navit.png DESTINATION share/icons/hicolor/108x108/apps RENAME ${PACKAGE}.png)
install(FILES  desktop_icons/128x128/navit.png DESTINATION share/icons/hicolor/128x128/apps RENAME ${PACKAGE}.png)
install(FILES  desktop_icons/256x256/navit.png DESTINATION share/icons/hicolor/256x256/apps RENAME ${PACKAGE}.png)
#install(FILES  bench.xpm DESTINATION )

if (USE_SVG OR SVG2PNG)
	FILE(GLOB SVG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg" "*.svgz")

	if(USE_SVG)
		list(APPEND IMAGES_TO_COPY ${SVG_FILES})
	endif(USE_SVG)
	if(SVG2PNG)
		list(APPEND FILES_TO_PNG ${SVG_FILES})
	endif(SVG2PNG)
endif()

FILE(GLOB XPM_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.xpm")
if(XPM2PNG)
	foreach (IMAGE_SRC ${SVG_FILES})
		string(REGEX REPLACE ".[a-z]*\$" "" IMAGE ${IMAGE_SRC})
		list(APPEND SVG_FILES_BASENAMES ${IMAGE})
	endforeach()
	foreach (IMAGE_SRC ${XPM_FILES})
		string(REGEX REPLACE ".[a-z]*\$" "" IMAGE ${IMAGE_SRC})
		list(FIND SVG_FILES_BASENAMES ${IMAGE} DUP_IMG)
		if ( ${DUP_IMG} LESS 0 )
			list(APPEND FILES_TO_PNG ${IMAGE_SRC})
		else()
			message(STATUS "image '${IMAGE}': XPM version skipped in favor of SVG")
		endif()
	endforeach()
else()
	list(APPEND IMAGES_TO_COPY ${XPM_FILES})
endif(XPM2PNG)

foreach (IMAGE_SRC ${FILES_TO_PNG})
	string(REGEX REPLACE ".[a-z]*\$" "" IMAGE ${IMAGE_SRC})

	if ( IMAGE_SRC MATCHES ".*svgz*" )
		if ( DEFINED svg2png_scaling_nav AND ${IMAGE} MATCHES "^nav_")
			add_scale_targets(${IMAGE_SRC} ${IMAGE} ${svg2png_scaling_nav})
		elseif( DEFINED svg2png_scaling_flag AND ${IMAGE} MATCHES "^country_")
			add_scale_targets(${IMAGE_SRC} ${IMAGE} ${svg2png_scaling_flag})
		elseif( DEFINED svg2png_scaling)
			add_scale_targets(${IMAGE_SRC} ${IMAGE} ${svg2png_scaling})
		endif()
	else()
		add_scale_targets(${IMAGE_SRC} ${IMAGE} 0)
	endif()

endforeach()

foreach (IMAGE_FILE ${IMAGES_TO_COPY})
	add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_FILE}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_FILE}"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_FILE}"
		"${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_FILE}"
		)
	list(APPEND IMAGE_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_FILE}")
endforeach()

add_custom_target(images ALL DEPENDS ${IMAGE_TARGETS})
add_custom_target(images_resource COMMAND mkdir -p ../resources/share/navit/xpm COMMAND cp ${IMAGE_TARGETS} ../resources/share/navit/xpm)
add_dependencies(images_resource images)

install(
	FILES ${IMAGE_TARGETS}
	DESTINATION ${IMAGE_DIR}
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
