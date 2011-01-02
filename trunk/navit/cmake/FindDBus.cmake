include(LibFindMacros)

libfind_pkg_check_modules(DBUS_PKGCONFIG dbus-1)

FIND_PATH(DBus_INCLUDE_DIR dbus/dbus.h
   PATHS
      ${DBUS_PKGCONFIG_INCLUDE_DIRS}
      /usr/include/dbus-1.0
#   PATH_SUFFIXES dbus
)

FIND_PATH(DBus_INCLUDE_DIR_ARCH dbus/dbus-arch-deps.h
   PATHS
      ${DBUS_PKGCONFIG_INCLUDE_DIRS}
      /usr/lib/dbus-1.0/include
#   PATH_SUFFIXES dbus
)

FIND_LIBRARY(DBus_LIBRARY
   NAMES dbus-1
   PATHS ${DBUS_PKGCONFIG_LIBRARY_DIRS}
)

set(DBus_PROCESS_INCLUDES DBus_INCLUDE_DIR DBus_INCLUDE_DIR_ARCH)
set(DBus_PROCESS_LIBS DBus_LIBRARY)
libfind_process(DBus)
