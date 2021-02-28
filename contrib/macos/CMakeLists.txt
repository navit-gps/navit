cmake_minimum_required(VERSION 3.2)
project(macosbundle C)

include(ExternalProject)
ExternalProject_Add(navit CMAKE_ARGS
		-Dbinding/python=false  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../)

add_custom_target(TARGET macosbundle POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/graphics
#    COMMAND ${CMAKE_COMMAND} -E copy
#    ${CMAKE_CURRENT_BINARY_DIR}/graphics/gtk_drawing_area/.libs/libgraphics_gtk_drawing_area.so ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/graphics

	COMMAND ${CMAKE_COMMAND} -E make_directory /usr/local/lib/navit

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	/usr/local/lib/navit ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${CMAKE_CURRENT_BINARY_DIR}/resources/share/navit ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/share
 	
 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${CMAKE_CURRENT_BINARY_DIR}/resources/share/locale ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/share/locale

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${CMAKE_CURRENT_BINARY_DIR}/icons ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/share/icons

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${CMAKE_CURRENT_BINARY_DIR}/textures ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/share/textures

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${CMAKE_CURRENT_BINARY_DIR}/maps ${CMAKE_CURRENT_BINARY_DIR}/navit.app/Contents/Resources/share/maps
	
	COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_BINARY_DIR}/navit.app /usr/local/bin/
)

