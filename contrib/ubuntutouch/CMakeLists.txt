
configure_file(manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.json DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES navit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES navit.apparmor DESTINATION ${CMAKE_INSTALL_PREFIX} RENAME ${NAVIT_BINARY}.apparmor)
