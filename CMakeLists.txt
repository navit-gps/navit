project(navit)

cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

set(PACKAGE_VERSION "0.2.0")
set(PACKAGE_NAME "navit-svn")

include("${PROJECT_SOURCE_DIR}/cmake/navit_macros.cmake")

### set defaults
# modules
add_module(binding/dbus TRUE)
add_module(graphics/android FALSE)
add_module(graphics/gd FALSE)
add_module(graphics/gtk_drawing_area FALSE)
add_module(graphics/null FALSE)
add_module(graphics/opengl TRUE)
add_module(graphics/qt_qpainter FALSE)
add_module(graphics/sdl TRUE)
add_module(graphics/win32 FALSE)
add_module(gui/gtk FALSE)
add_module(gui/internal TRUE)
add_module(gui/qml FALSE)
add_module(gui/win32 FALSE)
add_module(map/binfile TRUE)
add_module(map/filter TRUE)
add_module(map/mg TRUE)
add_module(map/shapefile TRUE)
add_module(map/textfile TRUE)
add_module(osd/core TRUE)
add_module(plugin/pedestrian FALSE)
add_module(speech/android FALSE)
add_module(speech/cmdline TRUE)
add_module(speech/dbus FALSE)
add_module(speech/espeak FALSE)
add_module(speech/speech_dispatcher TRUE)
add_plugin(support/espeak FALSE)
add_plugin(support/ezxml TRUE)
add_plugin(support/glib FALSE)
add_plugin(support/libpng FALSE)
add_plugin(support/win32 FALSE)
add_plugin(support/wordexp FALSE)
add_plugin(support/zlib FALSE)
add_module(vehicle/android FALSE)
add_module(vehicle/demo TRUE)
add_module(vehicle/file TRUE)
add_module(vehicle/gpsd TRUE)
add_module(vehicle/gpsd_dbus FALSE)
add_module(vehicle/gypsy TRUE)
add_module(vehicle/iphone FALSE)
add_module(vehicle/maemo FALSE)
add_module(vehicle/null FALSE)
add_module(vehicle/wince FALSE)

# other features
set(USE_PLUGINS TRUE)
set(USE_ROUTING TRUE)
set(USE_SVG TRUE)
set(SVG2PNG TRUE)
set(svg2png_scaling 32 48 96)
set(svg2png_scaling_flag 32 48 96)
set(svg2png_scaling_nav 32 48 96)
   
set(DBUS_USE_SYSTEM_BUS FALSE)
set(BUILD_MAPTOOL TRUE)

# install path
set(BIN_DIR bin)
set(SHARE_DIR share/navit)
set(LOCALE_DIR share)
set(IMAGE_DIR share/navit/xpm)
set(LIB_DIR lib)

# set platform defaults
if(WIN32)
   set_with_reason(graphics/win32 "win32" TRUE)
   set_with_reason(support/glib "win32" TRUE)
   set_with_reason(support/libpng "win32" TRUE)
   set_with_reason(support/win32 "win32" TRUE)
   set_with_reason(speech/espeak "win32" TRUE)
   set_with_reason(support/espeak "win32" TRUE)

   # workaround for crosscompiling. need to get rid of pkg_config or
   # change its search path
   set_with_reason(binding/dbus "win32" FALSE)

   # vehicle_file is broken for windows. use vehicle_wince instead
   set_with_reason(vehicle/wince "win32: replace broken vehicle_file" TRUE)
   set_with_reason(vehicle/file "win32: currently broken" FALSE)

   # plugins currently not supported on windows
   set_with_reason(USE_PLUGINS "win32: currently not supported" FALSE)

   set(HAVE_API_WIN32_BASE 1)
   set(HAVE_API_WIN32 1)
   
   # Image stuff
   set_with_reason(USE_SVG "win32: SVGs currently not supported" FALSE)

   set(BIN_DIR ./)
   set(SHARE_DIR ./)
   set(LOCALE_DIR ./)
   set(IMAGE_DIR xpm)

   list(APPEND NAVIT_LIBS winmm)

endif()

foreach(PLUMO ${ALL_PLUGINS} ${ALL_MODULES} USE_PLUGINS USE_ROUTING BUILD_MAPTOOL USE_SVG SVG2PNG DBUS_USE_SYSTEM_BUS)
   set(OLDVALUE ${${PLUMO}})
   OPTION(${PLUMO} "Enable/Disable module/plugin/support" ${${PLUMO}})
   if ( NOT (${OLDVALUE} STREQUAL ${${PLUMO}} ))
      set(${PLUMO}_REASON "User defined")
   endif()
endforeach()

## check for all nessessary progs and libs
INCLUDE (CheckIncludeFiles)

# SVG2PNG conversion
set (IMAGE_CONVERTER_PROGS rsvg-convert ksvgtopng ksvgtopng4)
find_program(IMAGE_CONVERTER NAMES ${IMAGE_CONVERTER_PROGS})
if (SVG2PNG)
   message(STATUS ${IMAGE_CONVERTER})
   if (NOT IMAGE_CONVERTER)
      message(WARNING "No SVG2PNG converter found. Please install one of the following tools: ${IMAGE_CONVERTER_PROGS}")
      set_with_reason(SVG2PNG "no converter found" FALSE)
   endif(NOT IMAGE_CONVERTER)
endif(SVG2PNG)

if (NOT ${support/glib})
   # Find glib
   find_package(GTK2 2.6 COMPONENTS gtk)

   if(GTK2_GLIB_INCLUDE_DIR)
      include_directories(${GTK2_GLIB_INCLUDE_DIR} ${GTK2_GLIBCONFIG_INCLUDE_DIR})
      #  link_directories(${GTK2_GLIB_LIBRARY})
      set(HAVE_GLIB 1)
      if (FOUND_GTK2)
         list(APPEND NAVIT_LIBS ${GTK2_GLIB_LIBRARY})
         set(HAVE_GTK2 1)
         set_with_reason(gui/gtk "GTK libs found" TRUE)
         set_with_reason(graphics/gtk_drawing_area "GTK libs found" TRUE)
      endif(FOUND_GTK2)
   else()
      message (FATAL_ERROR "glib not found  ${GTK_glib_INCLUDE_PATH}") 
   endif()
endif(NOT ${support/glib})

# QT stuff
find_package(Qt4 COMPONENTS QtCore QtGui QtXml QtDeclarative QtSvg OPTIONAL)
if (QT_FOUND)
   if (QT_QTCORE_FOUND AND QT_QTGUI_FOUND)
      set(graphics/qt_qpainter TRUE)
 
	
      if (QT_QTDECLARATIVE_FOUND AND QT_QTXML_FOUND)
         set_with_reason(gui/qml "Qt libs found" TRUE)
      endif()
      
      if (QT_QTSVG_FOUND)
         set(HAVE_QT_SVG 1)
      elseif( USE_SVG AND graphics/qt_qpainter)
         message( WARNING "QT compiled without SVG support")
      endif()
      
      include(${QT_USE_FILE})
      list(APPEND NAVIT_LIBS ${QT_LIBRARIES})
   endif()
endif(QT_FOUND)

# zlib
find_package(ZLIB)
if(ZLIB_FOUND)
   set(HAVE_ZLIB 1)
   list(APPEND NAVIT_LIBS ${ZLIB_LIBRARIES})
else()
   message(STATUS "using internal zlib")
   set_with_reason(support/zlib "native zlib missing" TRUE)
endif(ZLIB_FOUND)

# OpenGL
if (${graphics/opengl})
   CHECK_INCLUDE_FILES(glc.h HAVE_GLC)
   if (NOT HAVE_GLC)
      set(graphics/opengl FALSE)
      message (STATUS "no glc implementation found. Install Quesoglc for OpenGL support")
   else()
      find_package(OpenGL)
      if (OPENGL_FOUND AND OPENGL_GLU_FOUND AND OPENGL_INCLUDE_DIR)
         message (STATUS "opengl found: ${OPENGL_gl_LIBRARY}")
         include_directories(${OPENGL_INCLUDE_DIR})
      else()
         set(graphics/opengl FALSE)
         message (STATUS "opengl not found")
      endif()   
   endif(NOT HAVE_GLC)
endif()

# SDL
if (${graphics/sdl})
   find_package(SDL)
   if (SDL_FOUND)
      list(APPEND NAVIT_LIBS ${SDL_LIBRARY})
      find_package(SDL_ttf)
      if(SDLTTF_FOUND)
         set(SDL_TTF 1)
         list(APPEND NAVIT_LIBS ${SDLTTF_LIBRARY})
      endif(SDLTTF_FOUND)
   else()
      set_with_reason(graphics/sdl "SDL libs missing" FALSE)
   endif(SDL_FOUND)   

   find_package(SDL_image)
   if (SDLIMAGE_FOUND)
      list(APPEND NAVIT_LIBS ${SDLIMAGE_LIBRARY})
      set(SDL_IMAGE 1)
   endif(SDLIMAGE_FOUND)   
endif()

# freetype
find_package(Freetype)
if (FREETYPE_FOUND)
   include_directories(${FREETYPE_INCLUDE_DIRS})
endif(FREETYPE_FOUND)

CHECK_INCLUDE_FILES(wordexp.h HAVE_WORDEXP)
if (NOT HAVE_WORDEXP)
  set_with_reason(support/wordexp "native wordexp missing" TRUE)
  message(STATUS "wordexp.h not found. use internal wordexp")
endif()

# gpsd
CHECK_INCLUDE_FILES(gps.h HAVE_LIBGPS)
if ( NOT HAVE_LIBGPS)
   set_with_reason(vehicle/gpsd "gpsd lib missing" FALSE)
   set_with_reason(vehicle/gpsd_dbus "gpsd lib missing" FALSE)
endif()

# gypsy
CHECK_LIBRARY_EXISTS(gypsy gypsy_control_get_default "" GYPSY_FOUND)
if (NOT GYPSY_FOUND)
   set_with_reason(vehicle/gypsy "gypsy lib missing" FALSE)
endif(NOT GYPSY_FOUND)   

# gettext
find_package(Gettext)

# DBUS
if(${binding/dbus})
   find_package(PkgConfig)
   if(PKG_CONFIG_FOUND)
      pkg_check_modules(DBUS_GLIB dbus-glib-1)
      if(DBUS_GLIB_FOUND)
         include_directories(${DBUS_GLIB_INCLUDE_DIRS})
         list(APPEND NAVIT_LIBS ${DBUS_GLIB_LIBRARIES})
         set_with_reason(speech/dbus "dbus-glib-1 found" TRUE)
         set_with_reason(vehicle/gpsd_dbus "dbus-glib-1 found" TRUE)
      else()
         set_with_reason(binding/dbus "dbus-glib-1 missing" FALSE)
      endif()
   endif()
endif()


# speech
if(${speech/speech_dispatcher})
   CHECK_INCLUDE_FILES(libspeechd.h HAVE_LIBSPEECHD)
   if (NOT HAVE_LIBSPEECHD)
      set_with_reason(speech/speech_dispatcher "speech_dispatcher lib missing" FALSE)
   endif()
endif()


# Plugins
if(USE_PLUGINS)
   set(MODULE_BUILD_TYPE "MODULE")
   list(APPEND NAVIT_COMPILE_FLAGS -fPIC)
   message(STATUS "Use plugins: ${MODULE_BUILD_TYPE}")
else()
   set(MODULE_BUILD_TYPE "STATIC")
endif(USE_PLUGINS)

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
#CHECK_INCLUDE_FILES (windows.h HAVE_API_WIN32_BASE)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory (navit)
add_subdirectory (man)

if (GETTEXT_FOUND)
   add_subdirectory (po)
endif(GETTEXT_FOUND)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/locale DESTINATION ${LOCALE_DIR})
 
