add_definitions(-DLOCALE_ALIAS_PATH=LOCALEDIR -DIN_LIBINTL=1 -DHAVE_CONFIG_H=1 -DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DNO_XMALLOC -Dset_relocation_prefix=libintl_set_relocation_prefix -Drelocate=libintl_relocate -DDEPENDS_ON_LIBICONV=1)
supportlib_add_library(intl_cmake bindtextdom.c dcgettext.c dcigettext.c dcngettext.c dgettext.c dngettext.c explodename.c finddomain.c gettext.c intl-compat.c l10nflist.c loadmsgcat.c localcharset.c localealias.c localename.c log.c ngettext.c osdep.c plural-exp.c plural.c printf-args.c printf.c relocatable.c textdomain.c vasnprintf.c)
ADD_CUSTOM_TARGET(libintl.h DEPENDS libgnuintl.h COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/libgnuintl.h ${CMAKE_CURRENT_BINARY_DIR}/libintl.h)
ADD_DEPENDENCIES(intl_cmake libintl.h)
