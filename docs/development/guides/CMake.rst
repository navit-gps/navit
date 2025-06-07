CMake
=====

Building navit with CMake on different platforms:

.. _building_on_nix_systems:

Building on \*nix systems
-------------------------

Building
~~~~~~~~

First, install the required `dependencies <dependencies>`__.

Get the Navit source code, create somewhere a directory for the build
and then type

::

      cd /path/to/build/directory
      cmake /path/to/navit/source
      make

with cmake -i you'll be able to configure various build aspects

.. _building_on_windows:

Building on windows
-------------------

.. raw:: mediawiki

   {{merge|Windows development}}

.. _building_with_mingw:

Building with MinGW
~~~~~~~~~~~~~~~~~~~

You'll need MinGW with development headers and libs and CMake 2.8 or
newer. FreeType is a used library (but not really required, Stand 4.
August 2011), so get it from the http://www.gtk.org/download/win32.php
(32 bit) or http://www.gtk.org/download/win64.php (64 bit). It is
recommended to download a pkc-config and gettext from the site,
mentioned above. You may get full GTK2 download too, as you will be able
to use a gtk graphics and gui. Qt graphics modules works fine too, but
Qml gui have issues.

When build environment is ready, get the navit source code, create
directory for build and issue following commands:

::

    cd drive:/path/to/build/directory
    cmake drive:/path/to/navit/source -G "MinGW Makefiles"
    mingw32-make

.. _detailled_instructions_proved_to_be_working_with_r4675:

Detailled instructions proved to be working with r4675
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| `` Autors: AlexWien, Arpy``
| `` (alphabethically ordered)``

Since all attempts suggested on the `Windows <Windows>`__ failed for
weeks, we were given the hint to use CMake, and a link to this page,
which we try now to enhance with our detailled instructions which worked
for us finally. We hope you can find them useful too.

Unfortunately, since we are not Navit developers, this is a one-shot
explanation (because the ones provided did not work for us!). We will
not keep it up to date, please feel free to further enhance this page!

.. _setup_the_build_environment:

Setup the build Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  Setup MinGW

   -  Download MinGW from http://www.mingw.org/ (We used
      ``mingw-get-inst-20110530.exe``)
   -  Install to C:\MinGW (default) and use prepacked libs (default) and
      select all packages offered.

-  Setup GTK+

   -  Download the GTK+ dev installer. (We used
      ``gtk-dev-2.12.9-win32.exe``)
   -  Install to C:\GTK (default) and select all components
   -  Add an environment variable to have CMake find glib-2.0 later
      ``PKG_CONFIG_PATH=C:\GTK\lib\pkgconfig``

-  Setup CMake

   -  Download CMake 2.8.5 from http://www.cmake.org (Version 2.6 did
      not work!) We used ``cmake-2.8.5-win32-x86.exe``
   -  Install to C:\CMake (not default!) and add configure adding it to
      the Path (not default!)

-  Install an image converter tool
   This is needed to convert the SVG vector graphics from the sources to
   actual pixel images for the runtime.

   -  To use ImageMagick, download it from http://www.imagemagick.org,
      we took the 16 bit dll package:
      ``ImageMagick-6.7.1-2-Q16-windows-dll.exe``
   -  Install to C:\ImageMagic (not default: It is important, that there
      is no space in the path!) and have it added to your Path variable
      (not default!)
   -  Make sure ImageMagic is in your Path variable before system32, it
      contains a convert.exe as well, and ImageMagic won't work.

.. _run_a_build:

Run a build
^^^^^^^^^^^

-  Open a MinGW shell
-  Go to the navit sources
-  Create a build directory next to the navit source directory and cd
   into it
-  Configure the build (assuming that ../navit is the navit source
   directory)
   ``cmake ../navit -G "MinGW Makefiles"``
-  Hint: If CMake gives an error like *sh.exe was found in your PATH*,
   just ignore it and run the command again. CMake always seems to do
   this in an empty build directory.
-  Run the build
   ``mingw32-make``
-  Hint: If it complanins during image conversions, and multiple runs
   won't overcome the problem, simply take a pre-converted set of images
   and copy them to the navit/xpm folder. You may take the ones provided
   with a pre-installed Navit release. You may have to touch them
   (``touch navit/xpm/*.png``) in order for make to find them new
   enough.
-  Go into the navit folder and run navit.exe

.. _trouble_shooting_build:

Trouble Shooting Build
^^^^^^^^^^^^^^^^^^^^^^

-  Check your environment variables:

   -  GTK install should add many entries to INCLUDE and LIB; These
      values must be visible inside the mingw shell. (One of us had to
      add them manualy to the users environment variables, although they
      were visible at the systems environment variables)

-  After starting cmake, you get a configuration summary:

   -  Check in section --->>> graphics that gtk_drawing_area (GTK libs
      found) was detected (Enabled)

-  Compile error: undefined reference to \`libintl_printf'

   -  Check that PKG_CONFIG_PATH is set (see above)

.. _cross_compiling_for_windows_ce:

Cross-compiling for Windows CE
------------------------------

-  see `WinCE
   development#Building_Navit_using_Cmake <WinCE_development#Building_Navit_using_Cmake>`__
