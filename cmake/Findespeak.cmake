# Locate espeak-ng (preferred) or classic espeak.
#
# Defines:
#   espeak_FOUND
#   espeak_INCLUDE_DIRS
#   espeak_LIBRARIES
#   espeak_LIBRARY_DIRS
#   espeak_IS_NG

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(ESPEAK_NG QUIET espeak-ng)
endif()

if(ESPEAK_NG_FOUND)
	set(espeak_INCLUDE_DIRS ${ESPEAK_NG_INCLUDE_DIRS})
	set(espeak_LIBRARIES ${ESPEAK_NG_LIBRARIES})
	set(espeak_LIBRARY_DIRS ${ESPEAK_NG_LIBRARY_DIRS})
	set(espeak_IS_NG TRUE)
else()
	find_path(espeak_INCLUDE_DIR NAMES espeak/speak_lib.h)
	find_library(espeak_LIBRARY NAMES espeak)
	if(espeak_INCLUDE_DIR AND espeak_LIBRARY)
		set(espeak_INCLUDE_DIRS ${espeak_INCLUDE_DIR})
		set(espeak_LIBRARIES ${espeak_LIBRARY})
	endif()
	set(espeak_IS_NG FALSE)
	mark_as_advanced(espeak_INCLUDE_DIR espeak_LIBRARY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(espeak DEFAULT_MSG espeak_LIBRARIES)
