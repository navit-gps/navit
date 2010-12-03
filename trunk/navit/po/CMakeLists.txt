FIND_PROGRAM(XGETTEXT xgettext)

set(POTFILES
      ${PROJECT_SOURCE_DIR}/navit/main.c
      ${PROJECT_SOURCE_DIR}/navit/navigation.c
      ${PROJECT_SOURCE_DIR}/navit/navit.c
      ${PROJECT_SOURCE_DIR}/navit/popup.c
      ${PROJECT_SOURCE_DIR}/navit/country.c
      ${PROJECT_SOURCE_DIR}/navit/gui/gtk/destination.c
      ${PROJECT_SOURCE_DIR}/navit/gui/gtk/gui_gtk_action.c
      ${PROJECT_SOURCE_DIR}/navit/gui/gtk/gui_gtk_statusbar.c
      ${PROJECT_SOURCE_DIR}/navit/gui/internal/gui_internal.c
      ${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
)
if (XGETTEXT_GLADE)
      list(APPEND POTFILES ${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.glade)
endif()

ADD_CUSTOM_COMMAND(
   OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
   DEPENDS ${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml
   COMMAND ${CMAKE_COMMAND} -D SRC=${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml -D DST=${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.c
                     -P ${PROJECT_SOURCE_DIR}/cmake/navit_xml_parser.cmake
)

ADD_CUSTOM_COMMAND(
   OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.glade
   DEPENDS ${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml
   COMMAND ${CMAKE_COMMAND} -D SRC=${PROJECT_SOURCE_DIR}/navit/navit_shipped.xml -D DST=${CMAKE_CURRENT_BINARY_DIR}/navit_shipped.glade
                     -P ${PROJECT_SOURCE_DIR}/cmake/navit_xml_parser_glade.cmake
)

ADD_CUSTOM_COMMAND(
   OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/navit.po
   DEPENDS ${POTFILES}
   COMMAND ${XGETTEXT} --no-location --default-domain=navit --add-comments --keyword=_ --keyword=_n ${POTFILES}
)

FILE(GLOB POIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.po.in")

string(REPLACE ".po.in" "" LANGUAGES "${POIN_FILES}")

foreach (LANG ${LANGUAGES})
   ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/navit.po
      COMMAND ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/${LANG}.po.in -D DST=${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po.tmp -DPACKAGE_STRING=${PACKAGE_STRING} -P ${PROJECT_SOURCE_DIR}/cmake/navit_po_version.cmake
      COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --no-fuzzy-matching --verbose -o "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po" "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po.tmp" ${CMAKE_CURRENT_BINARY_DIR}/navit.po
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
endforeach(LANG ${LANGUAGES})

add_custom_target(locales ALL DEPENDS  ${MOFILES})
