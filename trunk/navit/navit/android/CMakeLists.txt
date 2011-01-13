if (ANDROID)

   CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)


   add_custom_command ( 
      OUTPUT build.xml
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
      COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} 
      COMMAND android update project -t android-${ANDROID_API_VERSION} -p ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res/raw/
      COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/armeabi
      COMMAND cmake -E copy ${PROJECT_BINARY_DIR}/navit/navit.xml ${CMAKE_CURRENT_BINARY_DIR}/res/raw/
      COMMAND cmake -E copy ${PROJECT_BINARY_DIR}/navit/libnavit.so ${CMAKE_CURRENT_BINARY_DIR}/libs/armeabi/
      COMMAND ${CMAKE_COMMAND} 
                     -D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/xpm/*.png" 
                     -D DST=${CMAKE_CURRENT_BINARY_DIR}/res/drawable
                     -D LOWER=1
                     -P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
      COMMAND ${CMAKE_COMMAND} 
                     -D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/*/*/.libs/*.so"
                     -D DST=${CMAKE_CURRENT_BINARY_DIR}/libs/armeabi
                     -P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
      COMMAND ${CMAKE_COMMAND} 
                     -D GLOB_EXP="${PROJECT_BINARY_DIR}/po/*.mo" 
                     -D DST=${CMAKE_CURRENT_BINARY_DIR}/res/raw
                     -D LOWER=1
                     -P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
   )

   add_custom_command ( 
      OUTPUT Navit-debug.apk
      DEPENDS build.xml
      COMMAND ant debug
   )
   
   add_custom_command ( 
      OUTPUT Navit-unsigned.apk
      DEPENDS build.xml
      COMMAND ant release
   )

   add_custom_target(apkg DEPENDS Navit-debug.apk)
   add_custom_target(apkg-release DEPENDS Navit-unsigned.apk)
   
endif(ANDROID)
