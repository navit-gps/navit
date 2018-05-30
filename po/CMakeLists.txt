FIND_PROGRAM(XGETTEXT xgettext)

set(POTFILES
	${PROJECT_SOURCE_DIR}/navit/main.c
	${PROJECT_SOURCE_DIR}/navit/navigation.c
	${PROJECT_SOURCE_DIR}/navit/navit.c
	${PROJECT_SOURCE_DIR}/navit/popup.c
	${PROJECT_SOURCE_DIR}/navit/country.c
	${PROJECT_SOURCE_DIR}/navit/start_real.c
	${PROJECT_SOURCE_DIR}/navit/route.c
	${PROJECT_SOURCE_DIR}/navit/vehicle.c
	${PROJECT_SOURCE_DIR}/navit/bookmarks.c
	${PROJECT_SOURCE_DIR}/navit/gui/gtk/destination.c
	${PROJECT_SOURCE_DIR}/navit/gui/gtk/gui_gtk_action.c
	${PROJECT_SOURCE_DIR}/navit/gui/gtk/gui_gtk_poi.c
	${PROJECT_SOURCE_DIR}/navit/gui/gtk/gui_gtk_statusbar.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_bookmark.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_command.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_html.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_menu.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_poi.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_search.c
	${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal_widget.c
	${PROJECT_SOURCE_DIR}/navit/osd/core/osd_core.c
	${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
	${CMAKE_CURRENT_BINARY_DIR}/strings_xml.c
	)

# Trick gettext to get translateable strings out of navit_shipped.xml

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
	DEPENDS ${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml
	COMMAND ${CMAKE_COMMAND} -D SRC=${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml -D DST=${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
	-P ${PROJECT_SOURCE_DIR}/cmake/navit_xml_parser.cmake
	)

# Trick gettext to get translateable strings out of android strings.xml

ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/strings_xml.c
	DEPENDS ${PROJECT_SOURCE_DIR}/navit/android/res/values/strings.xml
	COMMAND ${CMAKE_COMMAND} -D SRC=${PROJECT_SOURCE_DIR}/navit/android/res/values/strings.xml -D DST=${CMAKE_CURRENT_BINARY_DIR}/strings_xml.c
	-P ${PROJECT_SOURCE_DIR}/cmake/strings_xml_parser.cmake
	)


ADD_CUSTOM_COMMAND(
	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/navit.pot
	DEPENDS ${POTFILES}
	COMMAND ${XGETTEXT} --no-location --default-domain=navit_tmp --add-comments --keyword=_ --keyword=_n --keyword=navit_nls_gettext --keyword=navit_nls_ngettext:1,2 ${POTFILES}
	COMMAND ${CMAKE_COMMAND} -DMATCHES="CHARSET" -DREPLACE="utf-8"
	-DSRC=${CMAKE_CURRENT_BINARY_DIR}/navit_tmp.po -DDST=${CMAKE_CURRENT_BINARY_DIR}/navit.pot
	-P ${PROJECT_SOURCE_DIR}/cmake/replace.cmake
	)

FILE(GLOB POIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.po.in")

string(REPLACE ".po.in" "" LANGUAGES "${POIN_FILES}")

foreach (LANG ${LANGUAGES})
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/navit.pot ${CMAKE_CURRENT_SOURCE_DIR}/${LANG}.po.in
		COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/${LANG}.po.in -D DST=${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po.tmp -DPACKAGE_STRING=${PACKAGE_STRING} -P ${PROJECT_SOURCE_DIR}/cmake/navit_po_version.cmake
		COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --no-fuzzy-matching --verbose --quiet -o "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po" "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po.tmp" ${CMAKE_CURRENT_BINARY_DIR}/navit.pot
		COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po.tmp"
		)
endforeach(LANG ${LANGUAGES})

foreach (LANG ${LANGUAGES})
	ADD_CUSTOM_COMMAND(
		OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check --verbose --statistics --output-file="${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo" "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/locale/${LANG}/LC_MESSAGES"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo" "${PROJECT_BINARY_DIR}/locale/${LANG}/LC_MESSAGES/navit.mo"
		)
	SET(MOFILES ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo ${MOFILES})
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo DESTINATION ${LOCALE_DIR}/${LANG}/LC_MESSAGES RENAME ${PACKAGE}.mo)
endforeach(LANG ${LANGUAGES})

add_custom_target(locales ALL DEPENDS  ${MOFILES})
