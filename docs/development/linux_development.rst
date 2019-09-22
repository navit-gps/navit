=================
Linux Development
=================

Build dependencies
==================

To build with [[#CMake|CMake]] you will need `CMake 2.6` or newer.

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
To build maptool:
* protobuf-c
* libprotobuf-c-devel

GTK Gui:
* gtk2-devel

OpenGL Gui:
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

You can also try the ebuild in the overlay : [http://www.gentoo.org/proj/en/sunrise/ sunrise overlay].The ebuild is
based on the svn to have the latest version of navit

Debian dependencies
-------------------

It compiles flawlessly on a Lenny (5.0) or later, once all dependencies installed.

Notes:

* This page is for building navit with CMake.
* The Debian dependencies are the same as the Ubuntu dependencies (as Ubuntu is based on Debian). Maybe we should
  merge this page with [[Ubuntu dependencies]]?


== Absolute minimum requirements ==
'''gcc cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin'''

''Note:'' Not all these packages are strictly required (for example, maptool can be built without installing GTK+),
but this is the smallest practical set of packages if you want to run Navit.
== Translations for the user interface ==
'''gettext'''

== Maptool ==
'''protobuf-c-compiler  libprotobuf-c-dev'''

== GTK+ ==
''Included in minimum requirements''

== SDL ==
'''libsdl-image1.2-dev libdevil-dev libglc-dev freeglut3-dev libxmu-dev libfribidi-dev'''

== OpenGL graphics ==
'''libglc-dev freeglut3-dev libgl1-mesa-dev'''

== QT ==
'''libqt4-dev'''

This package will pull in all the required packages as dependencies.

== gpsd ==
'''libgps-dev'''

(optional, but certainly nice to have)

== espeak ==
'''espeak'''

(optional)

== speechd ==
'''libspeechd-dev'''

(optional, you are better off with using espeak)

== dbus ==
'''libdbus-glib-1-dev'''

(optional, you most likely don't need this.)

== python ==
'''python-dev'''

(optional, you most likely don't need this.)

== saxon ==
'''libsaxonb-java'''

only required for android

Ubuntu dependencies
-------------------

= Packages required for building and running Navit on Ubuntu =
These are the packages that must be installed to build and run Navit on Ubuntu.

Notes:
* These are not ''all'' packages that you need, only the packages that must be installed, i.e. that are not part of the default (desktop) install. If you removed packages after installation, you may have to re-install them.
* The list applies to '''Ubuntu 12.04 LTS''' (Precise Pangolin). It should be similar for other versions, but if not, please edit this page to correct.
* This list is for the CMake build. The build via autotools requires some more packages, but is no longer supported.

== Minimum requirements ==
* cmake
* protobuf-c-compiler
* libprotobuf-c-dev
* zlib1g-dev
* libpng12-dev
* libgtk2.0-dev
* librsvg2-bin
* g++ (not really required, but installing it avoids [http://trac.navit-project.org/ticket/1041 Navit bug 1041]).

''Note:'' Not all these packages are strictly required (for example, maptool can be built without installing GTK+), but this is the smallest practical set of packages if you want to run Navit.

==Optionals==
===Translated text in the user interface===
* gettext

===GPS support===
* gpsd
* gpsd-clients
* libgps-dev

===DBus===
libdbus-glib-1-dev

===GTK drawing area graphics===
libimlib2-dev (needed to enable draw_image_warp function which, in turn allows to use raster maps as discussed in [http://trac.navit-project.org/ticket/1285 #1285])

===OpenGL graphics===
* freeglut3-dev
* libxft-dev
* libglib2.0-dev
* libfreeimage-dev

= Ubuntu source package dependencies =

These are the dependencies for compiling Navit with ''all'' features. You may not need all of the packages mentioned there, but they can be useful if you experience problems just following the above instructions.

* [https://launchpad.net/ubuntu/precise/+source/navit 12.04 (Precise)]

Older Ubuntu versions (will probably not work with current Navit versions):

* [https://launchpad.net/ubuntu/natty/+source/navit 11.04 (Natty)]
* [https://launchpad.net/ubuntu/maverick/+source/navit 10.10 (Maverick)]
* [https://launchpad.net/ubuntu/lucid/+source/navit 10.04 (Lucid)]

=Everything in one command=

 sudo apt-get install cmake zlib1g-dev libpng12-dev libgtk2.0-dev librsvg2-bin \
 g++ gpsd gpsd-clients libgps-dev libdbus-glib-1-dev freeglut3-dev libxft-dev \
 libglib2.0-dev libfreeimage-dev gettext protobuf-c-compiler  libprotobuf-c-dev

Fedora dependencies
-------------------

= Compilation tools =
* gettext-devel (provides autopoint)
* libtool (will install a bunch of other needed packages)
* glib2-devel
* cvs
* python-devel

= OpenGL GUI =

cegui-devel
freeglut-devel
quesoglc-devel
SDL-devel
libXmu-devel

= GPSD Support =

gpsd-devel

= GTK Gui =
* gtk2-devel

= Speech support =
* speech-dispatcher-devel



= Installing  all dependencies =
su -

yum install gettext-devel libtool glib2-devel cegui-devel freeglut-devel quesoglc-devel SDL-devel libXmu-devel gpsd-devel gtk2-devel speech-dispatcher-devel cvs python-devel saxon-scripts

exit


Now continue and follow compilation instructions on: http://wiki.navit-project.org/index.php/NavIt_on_Linux

Taking care of dependencies
===========================

= Getting Navit from the GIT repository =
First, let's make sure we are in our home directory: this is only for the sake of making this tutorial simple to follow. You can save that directory anywhere you want, but you will have to adapt the rest of the instructions of this guide to your particular case.
 cd ~

Now, let's grab the code from Git. This assumes that you have git binaries installed.
  git clone https://github.com/navit-gps/navit.git

= Compiling =
GNU autotools was the old method but is removed in favour of CMake.

==CMake==
CMake builds Navit in a separate directory of your choice - this means that the directory in which the Git source was checked out remains untouched. See also [[CMake]].
 mkdir navit-build
 cd navit-build

Once inside the build directory just call the following commands:
 cmake ~/navit
 make

'''Note:''' CMake will autodetect your system configuration on the first run, and cache this information. Therefore installing or removing libraries after the first CMake run may confuse it and cause weird compilation errors (though installing new libraries should be ok). If you install or remove libraries/packages and subsequently run into errors, do a clean CMake run:
  rm -r ~/navit-build/*
  cmake ~/navit

===Running the compiled binary===
The binary is called '''navit''' and can be run without arguments:
 cd ~/navit-build/navit/
 ./navit

It is advised to just run this binary locally at the moment (i.e. not to install system-wide).
Note that for this to work, Navit must be run from the directory where it resides (that is, you must first change your working directory, as described above). If Navit is run from another directory, it will not find its plugins and image files, and will not start.

===Running the compiled binary===
Here, I am skipping the usual "make install" because we don't need to install navit systemwide for this example.

To execute navit, you can simply click on the binary file (if you are sure it is compiled properly) and it should launch. If you prefer to launch it from a terminal, you need to go into the directory containing the binary, first, like so:
 cd ~/'''navit'''/navit/
 ./navit

= Updating the GIT code =

You don't need to recompile everything to update navit to the latest code; with 'git pull' only the edited files will be downloaded. Just go to the navit directory (e.g. /home/CHANGEME/navit) and run:

 git pull

You then only need to run "make" again from your binary folder ( navit-build in the cmake example, or the current folder when using autotools).

= Prebuild binairies =

[[Download Navit|Prebuilt binaries]] exist for many distributions.

= Configuring the beast =
This is [[Configuration]], young padawan. Good luck :)

You can also check a [http://www.len.ro/2009/07/navit-gps-on-a-acer-aspire-one/ post describing a Navit configuration on Ubuntu Jaunty].

[[Category:Ports]]
[[Category:Linux]]
