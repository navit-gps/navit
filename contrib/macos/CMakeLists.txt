cmake_minimum_required(VERSION 3.2)
project(macosbundle C)

include(ExternalProject)
ExternalProject_Add(navit CMAKE_ARGS
		-Dbinding/python=false  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/../)
ExternalProject_Get_Property(navit BINARY_DIR)
ExternalProject_Get_Property(navit SOURCE_DIR)

install(DIRECTORY /usr/local/lib/navit/ DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources)
install(DIRECTORY ${BINARY_DIR}/navit/resources/share/navit/ DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit//resources/share/locale DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/icons DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/textures DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit/maps DESTINATION ${BINARY_DIR}/navit.app/Contents/Resources/share)
install(DIRECTORY ${BINARY_DIR}/navit.app DESTINATION /usr/local/bin)
Install(FILES ${SOURCE_DIR}navit/navit_shipped_gtkgui.xml DESTINATION ~/.navit)