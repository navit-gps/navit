module_add_library(binding_dbus binding_dbus.c) 
install(FILES org.navit_project.navit.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services/)