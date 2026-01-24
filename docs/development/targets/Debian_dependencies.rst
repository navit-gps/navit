.. _debian_dependencies:

Debian dependencies
===================

It compiles flawlessly on a Lenny (5.0) or later, once all dependencies
installed.

Notes:

-  This page is for building navit with CMake.
-  The Debian dependencies are the same as the Ubuntu dependencies (as
   Ubuntu is based on Debian). Maybe we should merge this page with
   `Ubuntu dependencies <Ubuntu_dependencies>`__?

Absolute minimum requirements
-----------------------------

**gcc cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin**

*Note:* Not all these packages are strictly required (for example,
maptool can be built without installing GTK+), but this is the smallest
practical set of packages if you want to run Navit.


Translations for the user interface
-----------------------------------

**gettext**

Maptool
-------

**protobuf-c-compiler libprotobuf-c-dev**

GTK+
----

*Included in minimum requirements*

SDL
---

**libsdl-image1.2-dev libdevil-dev libglc-dev freeglut3-dev libxmu-dev
libfribidi-dev**

OpenGL graphics
---------------

**libglc-dev freeglut3-dev libgl1-mesa-dev**

QT
--

**libqt4-dev**

This package will pull in all the required packages as dependencies.

gpsd
----

**libgps-dev**

(optional, but certainly nice to have)

espeak
------

**espeak**

(optional)

speechd
-------

**libspeechd-dev**

(optional, you are better off with using espeak)

dbus
----

**libdbus-glib-1-dev**

(optional, you most likely don't need this.)

python
------

**python-dev**

(optional, you most likely don't need this.)

saxon
-----

**libsaxonb-java**

only required for android
