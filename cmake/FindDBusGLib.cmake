FIND_PACKAGE(DBus)

include(LibFindMacros)

libfind_pkg_check_modules(DBUS_GLIB_PKGCONFIG dbus-glib-1)

FIND_PATH(DBusGLib_INCLUDE_DIR dbus/dbus-glib.h
   PATHS
      ${DBUS_GLIB_PKGCONFIG_INCLUDE_DIRS}
      /usr/include/dbus-1.0
#   PATH_SUFFIXES dbus
)

FIND_LIBRARY(DBusGLib_LIBRARY
   NAMES
      dbus-glib-1
   PATHS
      ${DBUS_GLIB_PKGCONFIG_LIBRARY_DIRS}
)

set(DBusGLib_PROCESS_INCLUDES DBusGLib_INCLUDE_DIR DBus_INCLUDE_DIRS)
set(DBusGLib_PROCESS_LIBS DBusGLib_LIBRARY DBus_LIBRARIES)
libfind_process(DBusGLib)
