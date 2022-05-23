if (ANDROID AND NOT ${CMAKE_GENERATOR} STREQUAL Ninja)

	FILE(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*/*/*/*")

	foreach ( PERMISSION ${ANDROID_PERMISSIONS})
		set(ANDROID_PERMISSIONS_EXPANDED "${ANDROID_PERMISSIONS_EXPANDED}<uses-permission\tandroid:name=\\\"android.permission.${PERMISSION}\\\"/>" )
	endforeach()

	# Generate Android string resource files from gettext .po.in files.
	# Note: Android language resources should have two letter ISO 639-1 code, and, optionally, iso-3166-1-alpha-2 region code,
	# so we skip all translation files which do not conform these requirements.
	FILE(GLOB POIN_FILES1 RELATIVE ${CMAKE_SOURCE_DIR}/po "${CMAKE_SOURCE_DIR}/po/??.po.in")
	FILE(GLOB POIN_FILES2 RELATIVE ${CMAKE_SOURCE_DIR}/po "${CMAKE_SOURCE_DIR}/po/??_??.po.in")
	set(POIN_FILES ${POIN_FILES1} ${POIN_FILES2})
	foreach (POIN_FILE ${POIN_FILES})
		string(REPLACE ".po.in" "" LANG ${POIN_FILE})
		string(REPLACE "_" "-r" LANG ${LANG})
		set(LANG_RESOURCES ${LANG_RESOURCES} "${CMAKE_CURRENT_BINARY_DIR}/res/values-${LANG}/strings.xml")
		compose_xslt_transform_command(TRANS "${CMAKE_CURRENT_SOURCE_DIR}/po2xml.xslt" "${CMAKE_CURRENT_SOURCE_DIR}/res/values/strings.xml" "${CMAKE_CURRENT_BINARY_DIR}/res/values-${LANG}/strings.xml" "po_file=${CMAKE_SOURCE_DIR}/po/${POIN_FILE}")
		ADD_CUSTOM_COMMAND(
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/res/values-${LANG}/strings.xml"
			DEPENDS "${CMAKE_BINARY_DIR}/po/navit.pot" "${CMAKE_SOURCE_DIR}/po/${POIN_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/po2xml.xslt" "${CMAKE_CURRENT_SOURCE_DIR}/res/values/strings.xml"
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res/values-${LANG}
			${TRANS}
			)
	endforeach(POIN_FILE ${POIN_FILES})

	add_custom_command (
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake
		COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake
		-D DST=${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
		-D NAME=ANDROID_VERSION_INT
		-D ANDROID_VERSION_NAME=${PACKAGE_STRING}
		-D ANDROID_API_VERSION=${ANDROID_API_VERSION}
		-D ANDROID_PERMISSIONS:string="${ANDROID_PERMISSIONS_EXPANDED}"
		-D STRIP_M="1"
		)

	add_custom_command (
		OUTPUT build.xml
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml ${SRC_FILES}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND android update project -t android-${ANDROID_API_VERSION} -p ${CMAKE_CURRENT_BINARY_DIR}
		)

	add_custom_target( android_resources
		DEPENDS build.xml ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} ${LANG_RESOURCES}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res/raw/
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/ldpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/mdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/hdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets/config/xxxhdpi
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ARCH}
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/navit/libnavit.so ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ARCH}/
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

	add_dependencies (android_resources images locales ${NAVIT_LIBNAME})

	add_custom_target ( apkg
		COMMAND ant debug
		)

	add_custom_target ( apkg-release
		COMMAND ant release
		)

	add_dependencies(apkg android_resources)
	add_dependencies(apkg-release android_resources)

endif(ANDROID AND NOT ${CMAKE_GENERATOR} STREQUAL Ninja)
