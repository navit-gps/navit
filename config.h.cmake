#cmakedefine ENABLE_NLS 1
#cmakedefine HAVE_MALLOC_H 1
#cmakedefine HAVE_SYS_MOUNT_H 1
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_API_WIN32_BASE 1
#cmakedefine HAVE_API_WIN32 1
#cmakedefine HAVE_API_WIN32_CE 1
#cmakedefine HAVE_GLIB 1
#cmakedefine HAVE_GETCWD 1
#cmakedefine CACHE_SIZE @CACHE_SIZE@
#cmakedefine AVOID_FLOAT 1
#cmakedefine AVOID_UNALIGNED 1
#cmakedefine USE_LIBGNUINTL 1

/* Versions */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"
#cmakedefine PACKAGE "@PACKAGE@"
#cmakedefine LOCALEDIR "@LOCALEDIR@"

#cmakedefine HAVE_ZLIB 1

#cmakedefine USE_ROUTING 1

#cmakedefine HAVE_GTK2 1

#cmakedefine USE_PLUGINS 1

#cmakedefine SDL_TTF 1

#cmakedefine SDL_IMAGE 1

#cmakedefine HAVE_QT_SVG 1

#cmakedefine HAVE_UNISTD_H 1

#cmakedefine DBUS_USE_SYSTEM_BUS 1

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FILE__":" << __LINE__
#endif
