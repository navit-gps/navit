.. _scratchbox_development:

Scratchbox development
======================

Developers may need to compile the source within the scratchbox
environment. You need to have Scratchbox and the Nokia SDK installed.
These are freely available from `Maemo <http://maemo.org>`__ Place the
source inside the scratchbox environment using

   svn co https://navit.svn.sourceforge.net/svnroot/navit/trunk/navit

, log in to scratchbox, cd to the source directory and execute the
following commands:

| ``export SBOX_DEFAULT_AUTOMAKE=1.9``
| ``./autogen.sh``
| ``./configure --prefix=/usr --disable-binding-python --disable-gui-sdl --enable-avoid-float \``
| ``--enable-avoid-unaligned --enable-svg2png-scaling="32,48" --enable-svg2png-scaling-flag="32,48" \``
| ``--enable-svg2png-scaling-nav="8,16,32,48"``
| ``make``
| ``make install``

You may need to apply the following
`patch <http://git.savannah.gnu.org/cgit/automake.git/commit/?id=b9df3285f2c32960ebeb979bbc6d76ea3e438ba4>`__
to automake, if autogen.sh fails with the following error:

   am__fastdepOBJC does not appear in AM_CONDITIONAL

If you don't want/need the sample map then add --disable-samplemap to
the above command.

As of SVN 1843 you need to get libgpsbt-dev before compiling. Execute
``fakeroot apt-get install libgpsbt-dev`` in scratchbox.

The above will build Navit along with pre-scaled icons. Prescaling the
icons is known to greatly improve rendering of the gui/internal
displays.

navit/xpm/navit_svg2png uses ksvg2png to pre-build the icons. If you
don't have kde installed (which you wouldn't have inside scratchbox)
then you will need to install librsvg2. Execute
``fakeroot apt-get librsvg2`` in scratchbox. The Makefile in navit/xpm
will automatically use one of rsvg-convert, ksvg2png or Inkscape,
depending on which is installed in your development environment.

A `debian directory <http://tech.visser-scully.ca>`__ is available so
that you can build your own packages. Download and unpack in your top
Navit source directory. Build a .deb file by using
``fakeroot ./debian/rules binary`` after running the initial autogen and
configure at least once. The .deb file will appear in the directory
above where you are. This works best after you have done at least one
build using the full set of commands above.

To install librsvg2 inside Scratchbox require you to get the source code
and install from there. I obtained mine from the `Ubuntu
sources <https://launchpad.net/ubuntu/+source/librsvg/2.22.3-0ubuntu1>`__.
Unpack the directory and then do the usual
``./configure && fakeroot make install`` inside Scratchbox. You need to
do this ``./configure && fakeroot make install`` for each Scratchbox
target that you want to build with. This same process applies to other
tools that you want to add the Scratchbox, e.g. ddd (symbolic debugger),
splint (code checker), etc.

.. _n900_notes:

N900 notes
----------

There is a `bug <https://bugzilla.redhat.com/show_bug.cgi?id=513582>`__
in a maemo5 freetype library, that causes navit to segv. Fast workaround
is to disable face caching in freetype plugin:

::

   Index: navit/navit/font/freetype/font_freetype.c
   ===================================================================
   --- navit/navit/font/freetype/font_freetype.c<->(revision 2912)
   +++ navit/navit/font/freetype/font_freetype.c<->(working copy)
   @@ -5,8 +5,8 @@
    #include <ft2build.h> 
    #include <glib.h> 
    #include FT_FREETYPE_H 
   -#ifndef USE_CACHING 
   -#define USE_CACHING 1 
   +#ifdef USE_CACHING 
   +#undef USE_CACHING 
    #endif 
    #if USE_CACHING 
    #include FT_CACHE_H 

Long way is to get the fresh freetype2 library(at least 2.3.10), build
it **statically** under scratchbox and link with Navit, so Navit will
use your version of freetype, instead of system one.

Maemo 5 introduced a new API for accessing GPS devices and Navit
supports it as vehicle_maemo, which is disabled by default, don't forget
to enable it. The tag for maemo could be like this:

`` ``\ 

-  Source could be:

   -  any - user selected source
   -  cwp - Complementary Wireless Positioning
   -  acwp - Assisted Complementary Wireless Positioning
   -  gnss - Global Navigation Satellite System (GPS)
   -  agnss - Assisted Global Navigation Satellite System (A-GPS)

Recommended (and default) value - "any"

retry_int defines, how often location updates should be sent to navit,
valid values are **1,2,5,10,20,30,60,120**, measured in seconds. "1"
second is a recommended and default value.

My configure line for n900 is:

   configure --prefix=/opt/navit --disable-binding-python
   --disable-gui-sdl --enable-avoid-float --enable-avoid-unaligned
   --enable-svg2png-scaling="32,48,64"
   --enable-svg2png-scaling-flag="32,48,64"
   --enable-svg2png-scaling-nav="8,16,32,48,64" --disable-samplemap
   --disable-graphics-sdl --enable-debug --enable-vehicle-maemo
