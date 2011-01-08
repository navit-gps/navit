# - Try to find Glib-2.0 (with gobject)
# Once done, this will define
#
#  Glib_FOUND - system has Glib
#  Glib_INCLUDE_DIRS - the Glib include directories
#  Glib_LIBRARIES - link these to use Glib

include(LibFindMacros)

libfind_pkg_check_modules(Gmodule_PKGCONF gmodule-2.0)
# Main include dir
find_path(Gmodule_INCLUDE_DIR
  NAMES gmodule.h
  PATHS ${Gmodule_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES gmodule-2.0
)

# Finally the modulerary itself
find_library(Gmodule_LIBRARY
  NAMES gmodule-2.0
  PATHS ${Gmodule_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Gmodule_PROCESS_INCLUDES Gmodule_INCLUDE_DIR)
set(Gmodule_PROCESS_LIBS Gmodule_LIBRARY)
libfind_process(Gmodule)
