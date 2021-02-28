cmake_minimum_required(VERSION 3.2)
project(macosbundle C)

include(ExternalProject)
ExternalProject_Add(navit CMAKE_ARGS
		-Dbinding/python=false  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/../)
ExternalProject_Get_Property(navit BINARY_DIR)
<<<<<<< HEAD
ExternalProject_Get_Property(navit SOURCE_DIR)

install(DIRECTORY /usr/local/lib/navit/ DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources)
install(DIRECTORY ${BINARY_DIR}/navit/resources/share/navit/ DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit//resources/share/locale DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/icons DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/textures DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/maps DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit.app DESTINATION /usr/local/bin)
Install(FILES ${SOURCE_DIR}navit/navit_shipped_gtkgui.xml DESTINATION ~/.navit)
=======
add_custom_target(bundle ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory /usr/local/lib/navit

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	/usr/local/lib/navit ${BINARY_DIR}/navit.app/Contents/Resources

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${BINARY_DIR}/navit/resources/share/navit ${BINARY_DIR}/navit.app/Contents/Resources/share
 	
 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${BINARY_DIR}/navit//resources/share/locale ${BINARY_DIR}/navit.app/Contents/Resources/share/locale

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${BINARY_DIR}/navit/icons ${BINARY_DIR}/navit.app/Contents/Resources/share/icons

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${BINARY_DIR}/navit/textures ${BINARY_DIR}/navit.app/Contents/Resources/share/textures

 	COMMAND ${CMAKE_COMMAND} -E copy_directory
 	${BINARY_DIR}/navit/maps ${BINARY_DIR}/navit.app/Contents/Resources/share/maps
	
	COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${BINARY_DIR}/navit.app /usr/local/bin/
)

>>>>>>> ba9cd87350c8e4e07dd030906b205dcc007c2326
