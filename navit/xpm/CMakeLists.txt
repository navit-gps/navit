macro(convert_svg2png IMAGE_INPUT IMAGE_OUTPUT SCALE)
   if(${IMAGE_CONVERTER} MATCHES "rsvg-convert")
      if (${SCALE} GREATER 0)
         set(SCALE_ARGS --width=${SCALE} --height=${SCALE})
      endif()
      set(COMMAND_ARGS ${SCALE_ARGS} --output ${IMAGE_OUTPUT} ${IMAGE_INPUT})
   elseif(${IMAGE_CONVERTER} MATCHES "ksvgtopng")
      set (NEW_SCALE ${SCALE})
      if (${SCALE} EQUAL 0)
         file(STRINGS  ${IMAGE_INPUT} NEW_SCALE_LINE REGEX "width=\"[0-9pxt.]*\"")
         string(REGEX REPLACE ".*width=\"([0-9]*).*\"" "\\1" NEW_SCALE ${NEW_SCALE_LINE})
         message(STATUS "newscale='${NEW_SCALE}'")
       endif()
      set(COMMAND_ARGS ${NEW_SCALE} ${NEW_SCALE}  ${IMAGE_INPUT} --output ${IMAGE_OUTPUT})
   else()
      message(FATAL_ERROR "no svg2png converter defined here.")
   endif()

   add_custom_command (
      OUTPUT ${IMAGE_OUTPUT}
      DEPENDS ${IMAGE_INPUT}
      COMMAND ${IMAGE_CONVERTER} ${COMMAND_ARGS}
   )
endmacro()

macro(add_scale_targets IMAGE_NAME)
   foreach (CURRENT_SCALE ${ARGN})
      convert_svg2png(
         ${CMAKE_CURRENT_SOURCE_DIR}/${SVG} 
         ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}_${CURRENT_SCALE}_${CURRENT_SCALE}.png 
         ${CURRENT_SCALE}
      )
      list(APPEND IMAGE_TARGETS ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_NAME}_${CURRENT_SCALE}_${CURRENT_SCALE}.png)
   endforeach()
endmacro()

#install(FILES  desktop_icons/navit.desktop DESTINATION share/applications)
#install(FILES  desktop_icons/22x22/navit.png DESTINATION share/icons/hicolor/22x22/apps)
#install(FILES  desktop_icons/128x128/navit.png DESTINATION share/icons/hicolor/128x128/apps)
#install(FILES  bench.xpm DESTINATION )

FILE(GLOB XPM_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.xpm")
list(APPEND IMAGE_TARGETS ${XPM_FILES})

if (USE_SVG OR SVG2PNG)
   FILE(GLOB SVG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg")
   
   if(USE_SVG)
      list(APPEND IMAGE_TARGETS ${SVG_FILES})
   endif(USE_SVG)

   if (SVG2PNG)
      foreach (SVG ${SVG_FILES})
            string(REPLACE ".svg" "" IMAGE ${SVG})
            
            if ( DEFINED svg2png_scaling_nav AND ${IMAGE} MATCHES "^nav_")
               add_scale_targets(${IMAGE} ${svg2png_scaling_nav})
            elseif( DEFINED svg2png_scaling_flag AND ${IMAGE} MATCHES "^country_")
               add_scale_targets(${IMAGE} ${svg2png_scaling_flag})
            elseif( DEFINED svg2png_scaling)
               add_scale_targets(${IMAGE} ${svg2png_scaling})
            endif()
            
            convert_svg2png (
               "${CMAKE_CURRENT_SOURCE_DIR}/${SVG}"
               "${CMAKE_CURRENT_BINARY_DIR}/${IMAGE}.png"
               0
            )
            list(APPEND IMAGE_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/${IMAGE}.png")
      endforeach()
   endif(SVG2PNG)
endif(USE_SVG OR SVG2PNG)

add_custom_target(images ALL DEPENDS ${IMAGE_TARGETS})

install(
   FILES ${IMAGE_TARGETS}
   DESTINATION ${IMAGE_DIR}
   PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
