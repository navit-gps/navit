module_add_library(binding_dbus binding_dbus.c)
set(bindir ${CMAKE_INSTALL_PREFIX}/${BIN_DIR})

configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/org.navit_project.navit.service.in"
	"${CMAKE_CURRENT_BINARY_DIR}/org.navit_project.navit.service"
	)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.navit_project.navit.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services/)

