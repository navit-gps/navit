.. _openembedded_development:

OpenEmbedded development
========================

Intro
=====

| |Navit on Nokia N810| This is a work in progress: I tried to build
  Navit svn for my Nokia n810 using the `OpenEmbedded cross-compile
  environment <http://www.openembedded.org>`__, just like I did for my
  `Ångström <Ångström>`__.
| These notes were made during the process. It's sort of a howto.
| Navit now fully works but there are a few things left to be
  'optimised'. It is by no means perfect or complete but shows my
  progress at this time (february/march 2009) with the issues needing
  attention. Maybe, by the time you reed this, the issues I found are
  already fixed, or new ones popped up. Feedback is welcome.
| The n810 is running
  `OS2008 <http://wiki.maemo.org/Updating_the_tablet_firmware>`__.
| My PC is running `Fedora <http://fedoraproject.org/>`__ 10 on AMD
  x86_64.
| I got a considerable amount of tips from gerritv from #navit on
  freenode while getting navit to work well on n810. Thanks!

Notes
=====

| First read the info from
  http://wiki.openembedded.net/index.php/Getting_Started
| Then set up the build environment along the info from
  http://www.angstrom-distribution.org/building-angstrom with these
  differences:
| Do get bitbake `bitbake
  1.8.12 <http://prdownload.berlios.de/bitbake/bitbake-1.8.12.tar.gz>`__.
| Do get the OE development branch:

| `` cd /OE ``
| `` git clone ``\ ```git://git.openembedded.org/openembedded.git`` <git://git.openembedded.org/openembedded.git>`__\ `` org.openembedded.dev ``
| `` cd org.openembedded.dev``
| `` git checkout -b org.openembedded.dev origin/org.openembedded.dev``

| See my build/conf/local.conf near the `bottom of this
  page <Navit_on_OpenEmbedded_for_n810#local.conf>`__.
| Edit org.openembedded.dev/conf/distro/chinook-compat.conf and change:

`` PREFERRED_VERSION_expat            = "1.95.7"``

into

`` PREFERRED_VERSION_expat            = "2.0.0"``

and

`` PREFERRED_VERSION_dbus             = "1.0.3"``

into

`` PREFERRED_VERSION_dbus             = "1.0.2"``

Then edit the navit line in
org.openembedded.dev/conf/distro/include/sane-srcrevs.inc to say:

`` SRCREV_pn-navit ?= "2580"``

(or the svn number you want to build, see
`svn <http://navit.svn.sourceforge.net/viewvc/navit/trunk/?view=log>`__)

In org.openembedded.dev/packages/navit/navit.inc you can remove the
EXTRA_OECONF line.

| My org.openembedded.dev/packages/navit/navit_svn.bb is near the
  `bottom of this page <Navit_on_OpenEmbedded_for_n810#navit_svn.bb>`__.
  As you can see there, the build uses libosso and libhildon so the +
  and - on top of device should zoom in or out; also gps should start
  automatically when we link libgpsbt and libgpsmgr.
| We build navit with gpsd-2.30 to get libgps15 which is deliverd by
  osso-gpsd-1.0 on the n810.
| Next, go to the main dir of the build environment (/OE) and type:

`` bitbake navit``

| The system will be busy for some time.
| *Drink some coffee...*
| When the bitbake build is ready, go to the
  chinook-compat-tmp/deploy/deb/armv6 directory to find your packages.

| We
  `edit <http://synthesize.us/HOWTO_make_a_deb_archive_without_dpkg>`__
  control.tar.gz in the navit_*.deb to remove dependency info about
  libgobject-2.0-0, libgmodule-2.0-0 and libgthread-2.0-0 which are all
  on the system but dpkg doesn't know about.
| Also we change the dependency for libgps17 into osso-gpsd.
| So we get something like this control file:

| `` Package: navit``
| `` Version: 0.1.0+svn2087-r1``
| `` Description: Navit is a car navigation system with routing engine.``
| `` Section: x11/applications``
| `` Priority: optional``
| `` Maintainer: OpenEmbedded Team <openembedded-devel@lists.openembedded.org>``
| `` Architecture: armel``
| `` OE: navit``
| `` Homepage: unknown``
| `` Depends: libc6 (>= 2.5), libdbus-glib-1-2 (>= 0.74), libdbus-1-3 (>= 1.0.2), libglib2.0-0 (>= 2.12.12), osso-gpsd (>= 1.0), libfreetype6 (>= 2.2.1), libfontconfig1 (>= 2.4.1), libgtk2.0-0 (>= 2.10.14), libatk1.0-0 (>= 1.18.0), libpango1.0-0 (>= 1.16.4), libcairo2 (>= 1.4.10), zlib1g (>= 1.2.3), libgcc1 (>= 3.4.4cs2005q3.2)``
| `` Recommends: gpsd, speechd, flite``

Reassemble a fresh deb using the
`info <http://synthesize.us/HOWTO_make_a_deb_archive_without_dpkg>`__:

| `` vi control``
| `` tar cvzpf conrol.tar.gz control``
| `` ar -r navit_2087.deb debian-binary control.tar.gz data.tar.gz ``

| Install and configure this reassembled deb as described (see
  `Configuration <Configuration>`__).
| E.g. try the `Internal GUI <Internal_GUI>`__ for a graphical menu,
  fullscreen map, `OSD <On_Screen_Display>`__, `POI <Other_maps>`__,
  etc.
| Specific tips for configuring navit for n8x0/n770 can be found
  `here <Maemo#Configuration_options>`__.

Issues
======

.. _kernel_patch:

kernel patch
------------

You might need this patch if you get an error during kernel compilation
of 2.6.21 about PATH_MAX:

| `` diff -r 557a4a0a5eac scripts/mod/sumversion.c``
| `` --- a/scripts/mod/sumversion.c    Fri May 30 19:08:50 2008 +0100``
| `` +++ b/scripts/mod/sumversion.c    Mon Jun 02 19:47:43 2008 +0900``
| `` @@ -8,6 +8,7 @@``
| ``  #include <errno.h>``
| ``  #include <string.h>``
| ``  #include "modpost.h"``
| `` +#include <linux/limits.h>``
| `` ``
| ``  /*``
| ``   * Stolen form Cryptographic API.``

.. _gpsd_patch:

gpsd patch
----------

To build gpsd 2.30 you need `this
patch <http://cvs.mandriva.com/cgi-bin/viewvc.cgi/contrib-SPECS/gpsd/gpsd-2.30-dbus050.patch?revision=1.1&view=markup&pathrev=r2_30-6mdv2007_0>`__.

.. _libgpsmgr_patch:

libgpsmgr patch
---------------

Currently compilation of libgpsmgr works on my system, with some
patching:

| `` --- libgpsmgr-0.1/configure.ac    2009-03-06 15:56:14.000000000 +0100``
| `` +++ libgpsmgr-0.1/configure.ac    2009-03-06 15:57:57.000000000 +0100``
| `` @@ -1,6 +1,5 @@``
| `` -AC_INIT(Makefile.am)``
| `` -AM_INIT_AUTOMAKE(libgpsmgr, patsubst(esyscmd([dpkg-parsechangelog | sed -n '/^Version: ``\ :math:`.*`\ ``$/ {s//\1/;p}']), [``
| `` -]))``
| `` +AC_INIT([Makefile.am], [0.1])``
| `` +AM_INIT_AUTOMAKE([libgpsmgr], [0.1])``
| ``  AM_CONFIG_HEADER(config.h)``
| ``  AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])])``

.. _libgpsbt_patch:

libgpsbt patch
--------------

Compilation of libgpsbt works on my system with:

| `` --- libgpsbt-0.1/configure.ac 2009-03-06 15:59:11.000000000 +0100``
| `` +++ libgpsbt-0.1/configure.ac 2009-03-06 15:59:42.000000000 +0100``
| `` @@ -1,6 +1,5 @@``
| `` -AC_INIT(Makefile.am)``
| `` -AM_INIT_AUTOMAKE(libgpsbt, patsubst(esyscmd([dpkg-parsechangelog | sed -n '/^Version: ``\ :math:`.*`\ ``$/ {s//\1/;p}']), [``
| `` -]))``
| `` +AC_INIT([Makefile.am], [0.1])``
| `` +AM_INIT_AUTOMAKE([libgpsbt], [0.1])``
| ``  AM_CONFIG_HEADER(config.h)``
| ``  AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])])``

.. _libgpsbt_bug_report:

libgpsbt bug report
-------------------

See the bug
`report <http://bugs.openembedded.net/show_bug.cgi?id=5056>`__ about
libgpsmgr and libgpsbt.

.. _to_do:

To do
-----

| The libgpsbt issue is the biggest problem to fix. Currently navit
  works but the workaround to get there is ugly. configure should detect
  the libgpsbt package automagically.
| I need a little help here; perhaps the .pc file is the solution?

.. _misc_stuff:

Misc stuff
==========

bitbake
-------

| Usefull reading: `bitbake
  manual <http://bitbake.berlios.de/manual/>`__.

.. _n810_gps:

n810 gps
--------

`FAQ <http://andrew.daviel.org/N810-FAQ.html#gpsd>`__

libhildon
---------

| You might want to go to this
  `site <http://git.pokylinux.org/cgit.cgi/experimental/meta-maemo/>`__
  and fetch the meta-maemo git tree. In there you'll find a recipe for a
  more recent libhildon. Copy the bb file into the maemo4 package
  directory of org.embedded.dev and adjust the version number in
  chinook-compat.conf.
| E.g. change:

`` PREFERRED_VERSION_libhildon = "1.99.0"``

into

`` PREFERRED_VERSION_libhildon = "2.0.4"``

armv6
-----

In org.openembedded.dev/conf/distro/chinook-compat.conf you could change

`` FEED_ARCH_nokia800            = "armv5te"``

into

`` FEED_ARCH_nokia800             = "armv6"``

if you're on Nokia n8x0.

ksvgtopng
---------

For converting the svg's to png's you could install librsvg2 and use
this fake ksvgtopng in e.g. /usr/local/bin which wraps rsvg:

| `` #!/bin/sh``
| `` if [ x$4 != x ] ; then``
| ``   rsvg -w $1 -h $2 $3 $4 $5``
| `` fi``

This wrapper has been obsoleted by svn 2088. Just add
"--with-svg2png-use-rsvg-convert" to the configure options to use rsvg
directly.

Files
=====

local.conf
----------

build/conf/local.conf:

| `` # Where to store sources ``
| `` DL_DIR = "/home/udo/downloads" ``
| `` ``
| `` # Which files do we want to parse: ``
| `` BBFILES := "/usr/src/OE/org.openembedded.dev/packages/*/*.bb" ``
| `` BBMASK = "" ``
| `` ``
| `` # ccache always overfill $HOME.... ``
| `` CCACHE="" ``
| `` ``
| `` # What kind of images do we want? ``
| `` IMAGE_FSTYPES = "jffs2 tar.gz " ``
| `` ``
| `` # Set TMPDIR instead of defaulting it to $pwd/tmp ``
| `` TMPDIR = "/usr/src/OE/${DISTRO}-tmp/" ``
| `` ``
| `` # Make use of my SMP box ``
| `` PARALLEL_MAKE="-j2" ``
| `` BB_NUMBER_THREADS = "2" ``
| `` ``
| `` # Set the Distro ``
| `` DISTRO = "chinook-compat" ``
| `` MACHINE = "nokia800" ``
| `` ENABLE_BINARY_LOCALE_GENERATION=0``
| `` ``
| `` PREFERRED_VERSION_navit="svn"``
| `` PREFERRED_VERSION_avahi="0.6.23"``
| `` PREFERRED_VERSION_udev="124"``
| `` PREFERRED_VERSION_gpsd="2.30"``
| `` INHERIT += "insane"``
| `` QA_LOG=1``

navit_svn.bb
------------

org.openembedded.dev/packages/navit/navit_svn.bb:

| `` require navit.inc``
| `` ``
| `` PV = "0.1.0+svn${SRCREV}"``
| `` PR = "r1"``
| `` ``
| `` DEFAULT_PREFERENCE = "10"``
| `` ``
| `` S = "${WORKDIR}/navit"``
| `` ``
| `` SRC_URI = "``\ ```svn://anonymous@navit.svn.sourceforge.net/svnroot/navit/trunk;module=navit;proto=https`` <svn://anonymous@navit.svn.sourceforge.net/svnroot/navit/trunk;module=navit;proto=https>`__\ `` \``
| ``     ``\ ```file://configurein3.patch;patch=1`` <file://configurein3.patch;patch=1>`__\ `` \``
| ``     ``\ ```file://gpsbtlib.patch;patch=1`` <file://gpsbtlib.patch;patch=1>`__\ ``"``
| `` ``
| `` EXTRA_AUTORECONF = " -I m4"``
| `` ``
| `` EXTRA_OECONF = "--disable-binding-python --disable-gui-sdl --disable-samplemap --enable-avoid-float --enable-avoid-unaligned --enable-gui-gtk --disable-postgresql \``
| `` --disable-graphics-sdl --enable-svg2png-scaling-flag=32,48 --enable-svg2png-scaling-nav=8,16,32,48 --with-svg2png-use-rsvg-convert"``
| `` ``
| `` DEPENDS = "gtk+ libglade libosso libhildon libgpsbt libgpsmgr"``

configurein3.patch
------------------

Forcing the GPSBT stuff in configure.in with
org.openembedded.dev/packages/navit/files/configurein3.patch:

| `` --- navit/configure.in    2009-03-04 16:20:40.000000000 +0100``
| `` +++ navit/configure.in    2009-03-04 16:21:07.000000000 +0100``
| `` @@ -105,7 +105,9 @@``
| ``       AC_SUBST(GPSBT_CFLAGS)``
| ``       AC_SUBST(GPSBT_LIBS)``
| ``       ], [``
| `` -     AC_MSG_RESULT(no)``
| `` +     AC_DEFINE(HAVE_GPSBT, 1, [Have the gpsbt library])``
| `` +     AC_SUBST(GPSBT_CFLAGS)``
| `` +     AC_SUBST(GPSBT_LIBS)``
| ``   ])``
| ``   if test x"${enable_hildon}" = xyes ; then``
| ``       AC_DEFINE(USE_HILDON, 1, [Build with maemo/hildon support])``

This patch is necessary because during the configure stage libgpsbt
isn't detected for some reason.

gpsbtlib.patch
--------------

We patch using org.openembedded.dev/packages/navit/files/gpsbtlib.patch:

| `` --- navit/navit/vehicle/gpsd/Makefile.am        2009-02-26 15:00:22.000000000 +0100``
| `` +++ navit/navit/vehicle/gpsd/Makefile.am        2009-03-06 17:00:22.000000000 +0100``
| `` @@ -2,5 +2,5 @@``
| ``  AM_CPPFLAGS = @NAVIT_CFLAGS@ @GPSBT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_gpsd``
| ``  modulevehicle_LTLIBRARIES = libvehicle_gpsd.la``
| ``  libvehicle_gpsd_la_SOURCES = vehicle_gpsd.c``
| `` -libvehicle_gpsd_la_LIBADD = @GPSD_LIBS@ @GPSBT_LIBS@``
| `` +libvehicle_gpsd_la_LIBADD = @GPSD_LIBS@ -lgpsbt -lgpsmgr``
| ``  libvehicle_gpsd_la_LDFLAGS = -module``

This patch is necessary because during the configure stage libgpsbt
isn't detected for some reason.

.. |Navit on Nokia N810| image:: 1.1small.jpg
