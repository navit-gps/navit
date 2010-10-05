# - Try to find GTK2
# Once done this will define
#
#  GTK2_FOUND - System has Boost
#  GTK2_INCLUDE_DIRS - GTK2 include directory
#  GTK2_LIBRARIES - Link these to use GTK2
#  GTK2_LIBRARY_DIRS - The path to where the GTK2 library files are.
#  GTK2_DEFINITIONS - Compiler switches required for using GTK2
#
#  Copyright (c) 2007 Andreas Schneider <mail@cynapses.org>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

set(GTK2_DEBUG ON)

macro(GTK2_DEBUG_MESSAGE _message)
  if (GTK2_DEBUG)
    message(STATUS "(DEBUG) ${_message}")
  endif (GTK2_DEBUG)
endmacro(GTK2_DEBUG_MESSAGE _message)

if (GTK2_LIBRARIES AND GTK2_INCLUDE_DIRS)
  # in cache already
  set(GTK2_FOUND TRUE)
else (GTK2_LIBRARIES AND GTK2_INCLUDE_DIRS)
  if (UNIX)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    include(UsePkgConfig)

    pkgconfig(gtk+-2.0 _GTK2IncDir _GTK2LinkDir _GTK2LinkFlags _GTK2Cflags)

    find_path(GTK2_GTK_INCLUDE_DIR
      NAMES
        gtk/gtk.h
      PATHS
        $ENV{GTK2_HOME}
        ${_GTK2IncDir}
        /usr/include/gtk-2.0
        /usr/local/include/gtk-2.0
        /opt/include/gtk-2.0
        /opt/gnome/include/gtk-2.0
        /sw/include/gtk-2.0
    )
    gtk2_debug_message("GTK2_GTK_INCLUDE_DIR is ${GTK2_GTK_INCLUDE_DIR}")

    # Some Linux distributions (e.g. Red Hat) have glibconfig.h
    # and glib.h in different directories, so we need to look
    # for both.
    #  - Atanas Georgiev <atanas@cs.columbia.edu>
    pkgconfig(glib-2.0 _GLIB2IncDir _GLIB2LinkDir _GLIB2LinkFlags _GLIB2Cflags)
    pkgconfig(gmodule-2.0 _GMODULE2IncDir _GMODULE2LinkDir _GMODULE2LinkFlags _GMODULE2Cflags)

    find_path(GTK2_GLIBCONFIG_INCLUDE_DIR
      NAMES
        glibconfig.h
      PATHS
        ${_GLIB2IncDir}
        ${_GMODULE2IncDir}
        /opt/gnome/lib64/glib-2.0/include
        /opt/gnome/lib/glib-2.0/include
        /opt/lib/glib-2.0/include
        /usr/lib64/glib-2.0/include
        /usr/lib/glib-2.0/include
        /sw/lib/glib-2.0/include
    )
    gtk2_debug_message("GTK2_GLIBCONFIG_INCLUDE_DIR is ${GTK2_GLIBCONFIG_INCLUDE_DIR}")

    find_path(GTK2_GLIB_INCLUDE_DIR
      NAMES
        glib.h
      PATHS
        ${_GLIB2IncDir}
        ${_GMODULE2IncDir}
        /opt/include/glib-2.0
        /opt/gnome/include/glib-2.0
        /usr/include/glib-2.0
        /sw/include/glib-2.0
    )
    gtk2_debug_message("GTK2_GLIB_INCLUDE_DIR is ${GTK2_GLIB_INCLUDE_DIR}")

    pkgconfig(gdk-2.0 _GDK2IncDir _GDK2LinkDir _GDK2LinkFlags _GDK2Cflags)

    find_path(GTK2_GDK_INCLUDE_DIR
      NAMES
        gdkconfig.h
      PATHS
        ${_GDK2IncDir}
        /opt/gnome/lib/gtk-2.0/include
        /opt/gnome/lib64/gtk-2.0/include
        /opt/lib/gtk-2.0/include
        /usr/lib/gtk-2.0/include
        /usr/lib64/gtk-2.0/include
        /sw/lib/gtk-2.0/include
    )
    gtk2_debug_message("GTK2_GDK_INCLUDE_DIR is ${GTK2_GDK_INCLUDE_DIR}")

    find_path(GTK2_GTKGL_INCLUDE_DIR
      NAMES
        gtkgl/gtkglarea.h
      PATHS
        ${_GLIB2IncDir}
        /usr/include
        /usr/local/include
        /usr/openwin/share/include
        /opt/gnome/include
        /opt/include
        /sw/include
    )
    gtk2_debug_message("GTK2_GTKGL_INCLUDE_DIR is ${GTK2_GTKGL_INCLUDE_DIR}")

    pkgconfig(libglade-2.0 _GLADEIncDir _GLADELinkDir _GLADELinkFlags _GLADECflags)

    find_path(GTK2_GLADE_INCLUDE_DIR
      NAMES
        glade/glade.h
      PATHS
        ${_GLADEIncDir}
        /opt/gnome/include/libglade-2.0
        /usr/include/libglade-2.0
        /opt/include/libglade-2.0
        /sw/include/libglade-2.0
    )
    gtk2_debug_message("GTK2_GLADE_INCLUDE_DIR is ${GTK2_GLADE_INCLUDE_DIR}")

    pkgconfig(pango _PANGOIncDir _PANGOLinkDir _PANGOLinkFlags _PANGOCflags)

    find_path(GTK2_PANGO_INCLUDE_DIR
      NAMES
        pango/pango.h
      PATHS
        ${_PANGOIncDir}
        /usr/include/pango-1.0
        /opt/gnome/include/pango-1.0
        /opt/include/pango-1.0
        /sw/include/pango-1.0
    )
    gtk2_debug_message("GTK2_PANGO_INCLUDE_DIR is ${GTK2_PANGO_INCLUDE_DIR}")

    pkgconfig(cairo _CAIROIncDir _CAIROLinkDir _CAIROLinkFlags _CAIROCflags)

    find_path(GTK2_CAIRO_INCLUDE_DIR
      NAMES
        cairo.h
      PATHS
        ${_CAIROIncDir}
        /opt/gnome/include/cairo
        /usr/include
        /usr/include/cairo
        /opt/include
        /opt/include/cairo
        /sw/include
        /sw/include/cairo
    )
    gtk2_debug_message("GTK2_CAIRO_INCLUDE_DIR is ${GTK2_CAIRO_INCLUDE_DIR}")

    pkgconfig(atk _ATKIncDir _ATKLinkDir _ATKLinkFlags _ATKCflags)

    find_path(GTK2_ATK_INCLUDE_DIR
      NAMES
        atk/atk.h
      PATHS
        ${_ATKIncDir}
        /opt/gnome/include/atk-1.0
        /usr/include/atk-1.0
        /opt/include/atk-1.0
        /sw/include/atk-1.0
    )
    gtk2_debug_message("GTK2_ATK_INCLUDE_DIR is ${GTK2_ATK_INCLUDE_DIR}")

    find_library(GTK2_GTK_LIBRARY
      NAMES
        gtk-x11-2.0
      PATHS
        ${_GTK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GTK_LIBRARY is ${GTK2_GTK_LIBRARY}")

    find_library(GTK2_GDK_LIBRARY
      NAMES
        gdk-x11-2.0
      PATHS
        ${_GDK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GDK_LIBRARY is ${GTK2_GDK_LIBRARY}")

    find_library(GTK2_GDK_PIXBUF_LIBRARY
      NAMES
        gdk_pixbuf-2.0
      PATHS
        ${_GDK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GDK_PIXBUF_LIBRARY is ${GTK2_GDK_PIXBUF_LIBRARY}")

    find_library(GTK2_GMODULE_LIBRARY
      NAMES
        gmodule-2.0
      PATHS
        ${_GMODULE2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GMODULE_LIBRARY is ${GTK2_GMODULE_LIBRARY}")

    find_library(GTK2_GTHREAD_LIBRARY
      NAMES
        gthread-2.0
      PATHS
        ${_GTK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GTHREAD_LIBRARY is ${GTK2_GTHREAD_LIBRARY}")

    find_library(GTK2_GOBJECT_LIBRARY
      NAMES
        gobject-2.0
      PATHS
        ${_GTK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GOBJECT_LIBRARY is ${GTK2_GOBJECT_LIBRARY}")

    find_library(GTK2_GLIB_LIBRARY
      NAMES
        glib-2.0
      PATHS
        ${_GLIB2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GLIB_LIBRARY is ${GTK2_GLIB_LIBRARY}")

    find_library(GTK2_GTKGL_LIBRARY
      NAMES
        gtkgl
      PATHS
        ${_GTK2LinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GTKGL_LIBRARY is ${GTK2_GTKGL_LIBRARY}")

    find_library(GTK2_GLADE_LIBRARY
      NAMES
        glade-2.0
      PATHS
        ${_GLADELinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_GLADE_LIBRARY is ${GTK2_GLADE_LIBRARY}")

    find_library(GTK2_PANGO_LIBRARY
      NAMES
        pango-1.0
      PATHS
        ${_PANGOLinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_PANGO_LIBRARY is ${GTK2_PANGO_LIBRARY}")

    find_library(GTK2_CAIRO_LIBRARY
      NAMES
        pangocairo-1.0
      PATHS
        ${_CAIROLinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_PANGO_LIBRARY is ${GTK2_CAIRO_LIBRARY}")

    find_library(GTK2_ATK_LIBRARY
      NAMES
        atk-1.0
      PATHS
        ${_ATKinkDir}
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
        /opt/gnome/lib
        /opt/lib
        /sw/lib
    )
    gtk2_debug_message("GTK2_ATK_LIBRARY is ${GTK2_ATK_LIBRARY}")

    set(GTK2_INCLUDE_DIRS
      ${GTK2_GTK_INCLUDE_DIR}
      ${GTK2_GLIBCONFIG_INCLUDE_DIR}
      ${GTK2_GLIB_INCLUDE_DIR}
      ${GTK2_GDK_INCLUDE_DIR}
      ${GTK2_GLADE_INCLUDE_DIR}
      ${GTK2_PANGO_INCLUDE_DIR}
      ${GTK2_CAIRO_INCLUDE_DIR}
      ${GTK2_ATK_INCLUDE_DIR}
    )

    if (GTK2_GTK_LIBRARY AND GTK2_GTK_INCLUDE_DIR)
      if (GTK2_GDK_LIBRARY AND GTK2_GDK_PIXBUF_LIBRARY AND GTK2_GDK_INCLUDE_DIR)
        if (GTK2_GMODULE_LIBRARY)
          if (GTK2_GTHREAD_LIBRARY)
            if (GTK2_GOBJECT_LIBRARY)
              if (GTK2_GLADE_LIBRARY AND GTK2_GLADE_INCLUDE_DIR)
                if (GTK2_PANGO_LIBRARY AND GTK2_PANGO_INCLUDE_DIR)
                  if (GTK2_CAIRO_LIBRARY AND GTK2_CAIRO_INCLUDE_DIR)
                    if (GTK2_ATK_LIBRARY AND GTK2_ATK_INCLUDE_DIR)

                      # set GTK2 libraries
                      set (GTK2_LIBRARIES
                        ${GTK2_GTK_LIBRARY}
                        ${GTK2_GDK_LIBRARY}
                        ${GTK2_GDK_PIXBUF_LIBRARY}
                        ${GTK2_GMODULE_LIBRARY}
                        ${GTK2_GTHREAD_LIBRARY}
                        ${GTK2_GOBJECT_LIBRARY}
                        ${GTK2_GLADE_LIBRARY}
                        ${GTK2_PANGO_LIBRARY}
                        ${GTK2_CAIRO_LIBRARY}
                        ${GTK2_ATK_LIBRARY}
                      )

                      # check for gtkgl support
                      if (GTK2_GTKGL_LIBRARY AND GTK2_GTKGL_INCLUDE_DIR)
                        set(GTK2_GTKGL_FOUND TRUE)

                        set(GTK2_INCLUDE_DIRS
                          ${GTK2_INCLUDE_DIR}
                          ${GTK2_GTKGL_INCLUDE_DIR}
                        )

                        set(GTK2_LIBRARIES
                          ${GTK2_LIBRARIES}
                          ${GTK2_GTKGL_LIBRARY}
                        )
                      endif (GTK2_GTKGL_LIBRARY AND GTK2_GTKGL_INCLUDE_DIR)

                    else (GTK2_ATK_LIBRARY AND GTK2_ATK_INCLUDE_DIR)
                      message(SEND_ERROR "Could not find ATK")
                    endif (GTK2_ATK_LIBRARY AND GTK2_ATK_INCLUDE_DIR)
                  else (GTK2_CAIRO_LIBRARY AND GTK2_CAIRO_INCLUDE_DIR)
                    message(SEND_ERROR "Could not find CAIRO")
                  endif (GTK2_CAIRO_LIBRARY AND GTK2_CAIRO_INCLUDE_DIR)
                else (GTK2_PANGO_LIBRARY AND GTK2_PANGO_INCLUDE_DIR)
                  message(SEND_ERROR "Could not find PANGO")
                endif (GTK2_PANGO_LIBRARY AND GTK2_PANGO_INCLUDE_DIR)
              else (GTK2_GLADE_LIBRARY AND GTK2_GLADE_INCLUDE_DIR)
                message(SEND_ERROR "Could not find GLADE")
              endif (GTK2_GLADE_LIBRARY AND GTK2_GLADE_INCLUDE_DIR)
            else (GTK2_GOBJECT_LIBRARY)
              message(SEND_ERROR "Could not find GOBJECT")
            endif (GTK2_GOBJECT_LIBRARY)
          else (GTK2_GTHREAD_LIBRARY)
            message(SEND_ERROR "Could not find GTHREAD")
          endif (GTK2_GTHREAD_LIBRARY)
        else (GTK2_GMODULE_LIBRARY)
          message(SEND_ERROR "Could not find GMODULE")
        endif (GTK2_GMODULE_LIBRARY)
      else (GTK2_GDK_LIBRARY AND GTK2_GDK_PIXBUF_LIBRARY AND GTK2_GDK_INCLUDE_DIR)
        message(SEND_ERROR "Could not find GDK (GDK_PIXBUF)")
      endif (GTK2_GDK_LIBRARY AND GTK2_GDK_PIXBUF_LIBRARY AND GTK2_GDK_INCLUDE_DIR)
    else (GTK2_GTK_LIBRARY AND GTK2_GTK_INCLUDE_DIR)
      message(SEND_ERROR "Could not find GTK2-X11")
    endif (GTK2_GTK_LIBRARY AND GTK2_GTK_INCLUDE_DIR)


    if (GTK2_INCLUDE_DIRS AND GTK2_LIBRARIES)
       set(GTK2_FOUND TRUE)
    endif (GTK2_INCLUDE_DIRS AND GTK2_LIBRARIES)

    if (GTK2_FOUND)
      if (NOT GTK2_FIND_QUIETLY)
        message(STATUS "Found GTK2: ${GTK2_LIBRARIES}")
      endif (NOT GTK2_FIND_QUIETLY)
    else (GTK2_FOUND)
      if (GTK2_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find GTK2")
      endif (GTK2_FIND_REQUIRED)
    endif (GTK2_FOUND)

    # show the GTK2_INCLUDE_DIRS and GTK2_LIBRARIES variables only in the advanced view
    mark_as_advanced(GTK2_INCLUDE_DIRS GTK2_LIBRARIES)

  endif (UNIX)
endif (GTK2_LIBRARIES AND GTK2_INCLUDE_DIRS)

