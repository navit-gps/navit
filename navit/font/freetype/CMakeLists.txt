module_add_library(font_freetype font_freetype.c)

if (FRIBIDI_FOUND)
	include_directories(${FRIBIDI_INCLUDE_DIRS})
	set_property(
		TARGET font_freetype
		APPEND PROPERTY COMPILE_DEFINITIONS USE_FRIBIDI=1
		)
	if (NOT FRIBIDI2_FOUND)
		set_property(
			TARGET font_freetype
			APPEND PROPERTY COMPILE_DEFINITIONS FRIBIDIOLD=1
			)
	endif (NOT FRIBIDI2_FOUND)
endif (FRIBIDI_FOUND)
