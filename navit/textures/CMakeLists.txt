macro(convert_to_png TEXTURE_INPUT TEXTURE_OUTPUT SCALE)
	get_filename_component(FILE_EXT ${TEXTURE_INPUT} EXT)
	string(TOUPPER ${FILE_EXT} FILE_TYPE)
	string(REGEX REPLACE "^\\." "" FILE_TYPE ${FILE_TYPE})
	if(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "rsvg-convert")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS --width=${SCALE} --height=${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS ${SCALE_ARGS} --output ${TEXTURE_OUTPUT} ${TEXTURE_INPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "convert")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS -resize ${SCALE}x${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS -background none ${TEXTURE_INPUT} ${SCALE_ARGS} ${TEXTURE_OUTPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "ksvgtopng")
		set (NEW_SCALE ${SCALE})
		if (${SCALE} EQUAL 0)
			file(STRINGS  ${TEXTURE_INPUT} NEW_SCALE_LINE REGEX "[^-]width=\"[0-9pxt.]*\"")
			string(REGEX MATCH "width=\"([0-9]*)[pxt]*\"" NEW_SCALE_LINE ${NEW_SCALE_LINE})
			set(NEW_SCALE ${CMAKE_MATCH_1})
		endif()
		set(COMMAND_ARGS ${NEW_SCALE} ${NEW_SCALE}  ${TEXTURE_INPUT} ${TEXTURE_OUTPUT})
	elseif(${IMAGE_CONVERTER_${FILE_TYPE}} MATCHES "inkscape")
		if (${SCALE} GREATER 0)
			set(SCALE_ARGS --export-width=${SCALE} --export-height=${SCALE})
		else()
			set(SCALE_ARGS)
		endif()
		set(COMMAND_ARGS --without-gui --export-background-opacity=0 --export-png ${TEXTURE_OUTPUT} ${SCALE_ARGS} ${TEXTURE_INPUT})
	else()
		message(FATAL_ERROR "no svg2png converter defined here.")
	endif()

	add_custom_command (
		OUTPUT ${TEXTURE_OUTPUT}
		DEPENDS ${TEXTURE_INPUT}
		COMMAND ${IMAGE_CONVERTER_${FILE_TYPE}} ${COMMAND_ARGS}
		)
endmacro()

macro(add_scale_targets TEXTURE_SRC TEXTURE_NAME)
	string(REPLACE "," ";" SCALES "${ARGN}")
	set(add_scale_targets_is_first_scale 1)
	foreach (CURRENT_SCALE ${SCALES})
		if(add_scale_targets_is_first_scale EQUAL 1)
			set (TEXTURE_DST ${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_NAME}.png)
			set(add_scale_targets_is_first_scale 0)
		elseif (${CURRENT_SCALE} EQUAL 0)
			set (TEXTURE_DST ${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_NAME}.png)
			message_error( "Image size value of 0 to leave texture unscaled can only be given as the first value in row, e.g. -D svg2png_scaling=0,16,32 " )
		else()
			set (TEXTURE_DST ${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_NAME}_${CURRENT_SCALE}_${CURRENT_SCALE}.png)
		endif()

		if(NOT CURRENT_SCALE EQUAL -1)
			convert_to_png(
				${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_SRC}
				${TEXTURE_DST}
				${CURRENT_SCALE}
				)
			list(APPEND TEXTURE_TARGETS ${TEXTURE_DST})
		endif()
	endforeach()
endmacro()

if (USE_SVG OR SVG2PNG)
	FILE(GLOB SVG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg" "*.svgz")

	if(USE_SVG)
		list(APPEND TEXTURES_TO_COPY ${SVG_FILES})
	endif(USE_SVG)
	if(SVG2PNG)
		list(APPEND FILES_TO_PNG ${SVG_FILES})
	endif(SVG2PNG)
endif()

FILE(GLOB XPM_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.xpm")
if(XPM2PNG)
	foreach (TEXTURE_SRC ${SVG_FILES})
		string(REGEX REPLACE ".[a-z]*\$" "" TEXTURE ${TEXTURE_SRC})
		list(APPEND SVG_FILES_BASENAMES ${TEXTURE})
	endforeach()
	foreach (TEXTURE_SRC ${XPM_FILES})
		string(REGEX REPLACE ".[a-z]*\$" "" TEXTURE ${TEXTURE_SRC})
		list(FIND SVG_FILES_BASENAMES ${TEXTURE} DUP_IMG)
		if ( ${DUP_IMG} LESS 0 )
			list(APPEND FILES_TO_PNG ${TEXTURE_SRC})
		else()
			message(STATUS "texture '${TEXTURE}': XPM version skipped in favor of SVG")
		endif()
	endforeach()
else()
	list(APPEND TEXTURES_TO_COPY ${XPM_FILES})
endif(XPM2PNG)

foreach (TEXTURE_SRC ${FILES_TO_PNG})
	string(REGEX REPLACE ".[a-z]*\$" "" TEXTURE ${TEXTURE_SRC})

	if ( TEXTURE_SRC MATCHES ".*svgz*" )
		if( DEFINED svg2png_scaling_textures)
			add_scale_targets(${TEXTURE_SRC} ${TEXTURE} ${svg2png_scaling_textures})
		endif()
	else()
		add_scale_targets(${TEXTURE_SRC} ${TEXTURE} 0)
	endif()

endforeach()

foreach (TEXTURE_FILE ${TEXTURES_TO_COPY})
	add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_FILE}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_FILE}"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_FILE}"
		"${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_FILE}"
		)
	list(APPEND TEXTURE_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_FILE}")
endforeach()

add_custom_target(textures ALL DEPENDS ${TEXTURE_TARGETS})
add_custom_target(textures_resource COMMAND mkdir -p ../resources/share/navit/textures COMMAND cp ${TEXTURE_TARGETS} ../resources/share/navit/textures)
add_dependencies(textures_resource textures)

install(
	FILES ${TEXTURE_TARGETS}
	DESTINATION ${TEXTURE_DIR}
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
