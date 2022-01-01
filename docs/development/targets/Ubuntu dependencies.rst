.. _ubuntu_dependencies:

Ubuntu dependencies
===================

.. raw:: mediawiki

   {{warning|1='''This is a page has been migrated to readthedocs:'''https://github.com/navit-gps/navit/pull/880 . It is only kept here for archiving purposes.}}

.. raw:: mediawiki

   {{merge|Debian dependencies}}

.. _packages_required_for_building_and_running_navit_on_ubuntu:

Packages required for building and running Navit on Ubuntu
==========================================================

These are the packages that must be installed to build and run Navit on
Ubuntu.

Notes:

-  These are not *all* packages that you need, only the packages that
   must be installed, i.e. that are not part of the default (desktop)
   install. If you removed packages after installation, you may have to
   re-install them.
-  The list applies to **Ubuntu 12.04 LTS** (Precise Pangolin). It
   should be similar for other versions, but if not, please edit this
   page to correct.
-  This list is for the CMake build. The build via autotools requires
   some more packages, but is no longer supported.

.. _minimum_requirements:

Minimum requirements
--------------------

-  cmake
-  protobuf-c-compiler
-  libprotobuf-c-dev
-  zlib1g-dev
-  libpng12-dev
-  libgtk2.0-dev
-  librsvg2-bin
-  g++ (not really required, but installing it avoids `Navit bug
   1041 <http://trac.navit-project.org/ticket/1041>`__).

*Note:* Not all these packages are strictly required (for example,
maptool can be built without installing GTK+), but this is the smallest
practical set of packages if you want to run Navit.

Optionals
---------

.. _translated_text_in_the_user_interface:

Translated text in the user interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  gettext

.. _gps_support:

GPS support
~~~~~~~~~~~

-  gpsd
-  gpsd-clients
-  libgps-dev

DBus
~~~~

libdbus-glib-1-dev

.. _gtk_drawing_area_graphics:

GTK drawing area graphics
~~~~~~~~~~~~~~~~~~~~~~~~~

libimlib2-dev (needed to enable draw_image_warp function which, in turn
allows to use raster maps as discussed in
`#1285 <http://trac.navit-project.org/ticket/1285>`__)

.. _opengl_graphics:

OpenGL graphics
~~~~~~~~~~~~~~~

-  freeglut3-dev
-  libxft-dev
-  libglib2.0-dev
-  libfreeimage-dev

.. _ubuntu_source_package_dependencies:

Ubuntu source package dependencies
==================================

These are the dependencies for compiling Navit with *all* features. You
may not need all of the packages mentioned there, but they can be useful
if you experience problems just following the above instructions.

-  `12.04
   (Precise) <https://launchpad.net/ubuntu/precise/+source/navit>`__

Older Ubuntu versions (will probably not work with current Navit
versions):

-  `11.04 (Natty) <https://launchpad.net/ubuntu/natty/+source/navit>`__
-  `10.10
   (Maverick) <https://launchpad.net/ubuntu/maverick/+source/navit>`__
-  `10.04 (Lucid) <https://launchpad.net/ubuntu/lucid/+source/navit>`__

.. _everything_in_one_command:

Everything in one command
=========================

| ``sudo apt-get install cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin \``
| ``g++ gpsd gpsd-clients libgps-dev libdbus-glib-1-dev freeglut3-dev libxft-dev \``
| ``libglib2.0-dev libfreeimage-dev gettext protobuf-c-compiler  libprotobuf-c-dev``
