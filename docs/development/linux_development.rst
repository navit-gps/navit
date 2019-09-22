=================
Linux Development
=================

Build dependencies
==================

To build with [[#CMake|CMake]] you will need '''CMake 2.6''' or newer.

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
* cegui-devel (AT LEAST 0.5). 0.5 is in packman repository. One click install for [http://api.opensuse-community.org/searchservice//YMPs/openSUSE_103/21b23afee0c62d4b5350bff51ac7aa41e2c28522  10.3] [http://packages.opensuse-community.org/aluminium.png 10.2]
* freeglut-devel
* QuesoGLC (at least 0.6) http://www.kazer.org/navit/quesoglc-0.7.0-1.i586.rpm
* gcc-c++


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
