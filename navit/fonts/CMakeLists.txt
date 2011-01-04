set( NAVIT_FONTS
      LiberationMono-Bold.ttf
      LiberationMono-BoldItalic.ttf
      LiberationMono-Italic.ttf
      LiberationMono-Regular.ttf
      LiberationSans-Bold.ttf
      LiberationSans-BoldItalic.ttf
      LiberationSans-Italic.ttf
      LiberationSans-Regular.ttf
      LiberationSerif-Bold.ttf
      LiberationSerif-BoldItalic.ttf
      LiberationSerif-Italic.ttf
      LiberationSerif-Regular.ttf
)

file(COPY ${NAVIT_FONTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(fonts ALL DEPENDS ${NAVIT_FONTS})

install(
   FILES ${NAVIT_FONTS}
   DESTINATION ${SHARE_DIR}/fonts
   PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
