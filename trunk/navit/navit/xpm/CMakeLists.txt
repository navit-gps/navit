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
         string(REGEX REPLACE ".*width=\"([0-9]*).*" "\\1" NEW_SCALE ${NEW_SCALE_LINE})
       endif()
      set(COMMAND_ARGS ${NEW_SCALE} ${NEW_SCALE}  ${IMAGE_INPUT} --output ${IMAGE_OUTPUT})
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
   foreach (CURRENT_SCALE ${SCALES})
      if (${CURRENT_SCALE} EQUAL 0)
         set (IMAGE_DST ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}.png)
      else()
         set (IMAGE_DST ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}_${CURRENT_SCALE}_${CURRENT_SCALE}.png)
      endif()

      convert_to_png(
         ${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_SRC} 
         ${IMAGE_DST}
         ${CURRENT_SCALE}
      )
      list(APPEND IMAGE_TARGETS ${IMAGE_DST})
   endforeach()
endmacro()

install(FILES  desktop_icons/navit.desktop DESTINATION share/applications)
install(FILES  desktop_icons/22x22/navit.png DESTINATION share/icons/hicolor/22x22/apps)
install(FILES  desktop_icons/128x128/navit.png DESTINATION share/icons/hicolor/128x128/apps)
#install(FILES  bench.xpm DESTINATION )

if (USE_SVG OR SVG2PNG)
   FILE(GLOB SVG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg" "*.svgz")
   
   if(USE_SVG)
      list(APPEND IMAGE_TARGETS ${SVG_FILES})
   endif(USE_SVG)
   if(SVG2PNG)
      list(APPEND FILES_TO_PNG ${SVG_FILES})
   endif(SVG2PNG)
endif()

FILE(GLOB XPM_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.xpm")
if(XPM2PNG)
   list(APPEND FILES_TO_PNG ${XPM_FILES})
else()
   list(APPEND IMAGE_TARGETS ${XPM_FILES})
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

add_custom_target(images ALL DEPENDS ${IMAGE_TARGETS})

install(
   FILES ${IMAGE_TARGETS}
   DESTINATION ${IMAGE_DIR}
   PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
