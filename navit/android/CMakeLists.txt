if (ANDROID)

	FILE(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*/*/*/*")


	add_custom_target( android_resources
		DEPENDS ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} images locales navit_config_xml
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res/raw/
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/ldpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/mdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/hdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxxhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ARCH}
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/ldpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/ldpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/mdpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/mdpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/hdpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/hdpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/xhdpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/xhdpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/xxhdpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxhdpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/config/xxxhdpi/*.xml"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxxhdpi
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/icons/*.png"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/res/drawable-nodpi
			-D LOWER=1
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/navit/*/*/.libs/lib[b-z]*.so"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ARCH}
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		COMMAND ${CMAKE_COMMAND}
			-D GLOB_EXP="${PROJECT_BINARY_DIR}/po/*.mo"
			-D DST=${CMAKE_CURRENT_BINARY_DIR}/res/raw
			-D LOWER=1
			-P ${PROJECT_SOURCE_DIR}/cmake/copy_files.cmake
		)

    add_custom_target( copy_to_gradle
            DEPENDS android_resources
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GRADLE_BUILD_DIR}/android/
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/assets ${GRADLE_BUILD_DIR}/android/assets
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/res ${GRADLE_BUILD_DIR}/android/res

    )

endif(ANDROID)
