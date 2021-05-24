=================
Linux Development
=================

Build dependencies
==================

Note that most dependencies are optional, meaning Navit will build without them, but you may find that you have some crucial features missing such as the GUI.

In general you will need one of `ksvgtopng`, `rsvg-convert` or `Inkscape` to build pre-scaled icons in the xpm directory - the build process will detect if you have one of those installed, and warn you otherwise.

Please see platform specific sections such as Nokia N8x0 for their additional development environment dependencies.

To build with CMake you will need `CMake 2.6` or newer.

OpenSuse dependencies
---------------------

Compilation tools:
 * libtool
 * automake
 * autoconf
 * gettext-devel
 * glib2-devel
 * gcc

Optionals:
 - To build maptool:
   * protobuf-c
   * libprotobuf-c-devel
 - GTK Gui:
   * gtk2-devel
 - OpenGL Gui:
   * SDL-devel
   * cegui-devel (AT LEAST 0.5). 0.5 is in packman repository. One click install for [10.3](http://api.opensuse-community.org/searchservice//YMPs/openSUSE_103/21b23afee0c62d4b5350bff51ac7aa41e2c28522) [10.2](http://packages.opensuse-community.org/aluminium.png)
   * freeglut-devel
   * QuesoGLC (at least 0.6) http://www.kazer.org/navit/quesoglc-0.7.0-1.i586.rpm
   * gcc-c++

Gentoo dependencies
-------------------

There is a Gentoo ebuild for navit in Gentoo's Bugzilla : http://bugs.gentoo.org/show_bug.cgi?id=176552

If you want, you can vote for it so it gets included in portage :
http://bugs.gentoo.org/votes.cgi?action=show_user&bug_id=176552#vote_176552

You can also try the ebuild in the overlay : [sunrise overlay](http://www.gentoo.org/proj/en/sunrise/). The ebuild is
based on the svn to have the latest version of navit.

Debian / Ubuntu dependencies
----------------------------

It compiles flawlessly on a Lenny (5.0) or later and on `Ubuntu 12.04 LTS` (Precise Pangolin) or later, once all dependencies installed.

Note that this section is for build Navit with CMake. These are not ''all'' packages that you need, only the packages that must be installed, i.e. that are not part of the default (desktop) install. If you removed packages after installation, you may have to re-install them.

Absolute minimum requirements:

.. code-block:: bash

	gcc cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin

Note that not all these packages are strictly required (for example, maptool can be built without installing GTK+),
but this is the smallest practical set of packages if you want to run Navit.

  * Translations for the user interface: `gettext`
  * Maptool: `protobuf-c-compiler  libprotobuf-c-dev`
  * GTK+ is included in minimum requirements. `libimlib2-dev` is needed to enable draw_image_warp function which, in turn
    allows to use raster maps as discussed in [track #1285](http://trac.navit-project.org/ticket/1285)
  * SDL: `libsdl-image1.2-dev libdevil-dev libglc-dev freeglut3-dev libxmu-dev libfribidi-dev`
  * OpenGL graphics: `libglc-dev freeglut3-dev libgl1-mesa-dev libxft-dev libglib2.0-dev libfreeimage-dev`
  * QT: `libqt4-dev` (This package will pull in all the required packages as dependencies.)
  * gpsd: `gpsd gpsd-clients libgps-dev` (optional, but certainly nice to have)
  * espeak: `espeak` (optional)
  * speechd: `libspeechd-dev` (optional, you are better off with using espeak)
  * dbus: `libdbus-glib-1-dev` (optional, you most likely don't need this.)
  * python: `python-dev` (optional, you most likely don't need this.)
  * saxon: `libsaxonb-java` (only required for android)

Everything in one command:

.. code-block:: bash

    sudo apt-get install cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin \
                         g++ gpsd gpsd-clients libgps-dev libdbus-glib-1-dev freeglut3-dev libxft-dev \
                         libglib2.0-dev libfreeimage-dev gettext protobuf-c-compiler  libprotobuf-c-dev

For Raspberry Pi OS:

.. code-block:: bash

    sudo apt-get install cmake zlib1g-dev libpng-dev libgtk2.0-dev librsvg2-bin \
                         g++ gpsd gpsd-clients libgps-dev libdbus-glib-1-dev freeglut3-dev libxft-dev \
                         libglib2.0-dev libfreeimage-dev gettext protobuf-c-compiler  libprotobuf-c-dev


Fedora dependencies
-------------------

Compilation tools:
 * gettext-devel (provides autopoint)
 * libtool (will install a bunch of other needed packages)
 * glib2-devel
 * cvs
 * python-devel

OpenGL GUI:
 * cegui-devel
 * freeglut-devel
 * quesoglc-devel
 * SDL-devel
 * libXmu-devel

GPSD Support:
 * gpsd-devel

GTK Gui:
 * gtk2-devel

Speech support:
 * speech-dispatcher-devel

Installing  all dependencies:

.. code-block:: bash

    sudo yum install gettext-devel libtool glib2-devel cegui-devel freeglut-devel quesoglc-devel SDL-devel libXmu-devel gpsd-devel gtk2-devel speech-dispatcher-devel cvs python-devel saxon-scripts


Taking care of dependencies
===========================

Getting Navit from the GIT repository
-------------------------------------

First, let's make sure we are in our home directory: this is only for the sake of making this tutorial simple to follow. You can save that directory anywhere you want, but you will have to adapt the rest of the instructions of this guide to your particular case.

.. code-block:: bash

 cd ~

Now, let's grab the code from Git. This assumes that you have git binaries installed.

.. code-block:: bash

  git clone https://github.com/navit-gps/navit.git

Compiling
---------

GNU autotools was the old method but is removed in favour of CMake.

CMake builds Navit in a separate directory of your choice - this means that the directory in which the Git source was checked out remains untouched.

.. code-block:: bash

 mkdir navit-build
 cd navit-build

Once inside the build directory just call the following commands:

.. code-block:: bash

 cmake ~/navit
 make

Note that CMake will autodetect your system configuration on the first run, and cache this information. Therefore installing or removing libraries after the first CMake run may confuse it and cause weird compilation errors (though installing new libraries should be ok). If you install or remove libraries/packages and subsequently run into errors, do a clean CMake run:

.. code-block:: bash

  rm -r ~/navit-build/*
  cmake ~/navit

Running the compiled binary
---------------------------

It is advised to just run this binary locally at the moment (i.e. not to install system-wide).
Note that for this to work, Navit must be run from the directory where it resides (that is, you must first change your working directory, as described above). If Navit is run from another directory, it will not find its plugins and image files, and will not start.

Here, I am skipping the usual `make install` because we don't need to install navit systemwide for this example.

To execute navit, you can simply click on the binary file (if you are sure it is compiled properly) and it should launch. If you prefer to launch it from a terminal, you need to go into the directory containing the binary, first, like so:

.. code-block:: bash

 cd ~/navit/navit/
 ./navit

Updating the GIT code
---------------------

You don't need to recompile everything to update navit to the latest code; with `git pull` only the edited files will be downloaded. Just go to the navit directory (e.g. `/home/CHANGEME/navit`) and run:

.. code-block:: bash

 git pull

You then only need to run `make` again from your binary folder ( navit-build in the cmake example, or the current folder when using autotools).

Prebuild binairies
------------------

[[Download Navit|Prebuilt binaries]] exist for many distributions.

Configuring the beast
---------------------

This is [Configuration](https://wiki.navit-project.org/index.php/Configuration), young padawan. Good luck :)

You can also check a [post describing a Navit configuration on Ubuntu Jaunty](http://www.len.ro/2009/07/navit-gps-on-a-acer-aspire-one/).
