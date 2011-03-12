if (ANDROID)

   FILE(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*/*/*/*")

   add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake
      COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake
                               -D DST=${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
                               -D ANDROID_VERSION_INT="0"
                               -D NAME=ANDROID_VERSION_INT
                               -D ANDROID_VERSION_NAME=${PACKAGE_STRING}
                               -D ANDROID_API_VERSION=${ANDROID_API_VERSION}
                               -P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
   )

   add_custom_command (
      OUTPUT build.xml
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml ${SRC_FILES}
      COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND android update project -t android-${ANDROID_API_VERSION} -p ${CMAKE_CURRENT_BINARY_DIR}
   )

   add_custom_target( android_resources
      DEPENDS build.xml ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS}
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
   
   add_dependencies (android_resources images locales ${NAVIT_LIBNAME})
   
   add_custom_target ( apkg
      COMMAND ant debug
   )
   
   add_custom_target ( apkg-release
      COMMAND ant release
   )

   add_dependencies(apkg android_resources)
   add_dependencies(apkg-release android_resources)
   
endif(ANDROID)
