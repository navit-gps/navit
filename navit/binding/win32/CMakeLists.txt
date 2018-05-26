if (USE_LIBGNUINTL AND NOT HAVE_GLIB)
	ADD_DEPENDENCIES(support_glib intl_cmake)
endif()

module_add_library(binding_win32 binding_win32.c)

add_executable(tell_navit tell_navit.c)
target_link_libraries(tell_navit  ${MODULES_NAME} ${NAVIT_SUPPORT_LIBS} fib ${NAVIT_LIBS} )
install(TARGETS tell_navit DESTINATION ${BIN_DIR} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
