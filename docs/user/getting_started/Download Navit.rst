.. _download_navit:

Download Navit
==============

Most operating systems contain package managers, which usually already
contain Navit packages. Unfortunatly, they are mostly outdated, so we
recommend to use github sources:

-  https://github.com/navit-gps/navit/

.. raw:: mediawiki

   {{note
   |NAVIT is under heavy development, so recommend to use the '''up to date Github version''' to get the newest features and bug fixes!
   }}

.. _portable_devices:

Portable devices
================

If you can't find you platform, check `Impossible
ports <Impossible_ports>`__ and ask at `contacts <contacts>`__

-  `Android <Android>`__

   -  `Augmented Reality <Augmented_Reality>`__

-  `Ångström development <Ångström_development>`__
-  `iOS <iOS>`__
-  `Maemo <Maemo>`__
-  `TomTom <TomTom>`__
-  `WebOS <WebOS>`__
-  `WinCE <WinCE>`__ (see also `Navit on
   Windows <Windows#Windows_Mobile.2FWindows_CE>`__)
-  `Sailfish OS <Sailfish_OS>`__

Desktop
=======

ArchLinux
---------

If you want to install the SVN version of Navit on ArchLinux you can
build it by using the
`PKGBUILD <https://aur.archlinux.org/packages/navit/>`__ from AUR

Debian
------

unofficial packages available on the repositories of all currently
supported versions

Fedora
------

*currently no unofficial packages*

Gentoo
------

Navit is built from SVN, so it is marked as live-overlay. Therefore it
is masked and cannot be installed without further means. You have to
insert a new line to /etc/portage/package.keywords:

``=sci-geosciences/navit-9999-r1 **``

Ebuild available in the `booboo
overlay <https://github.com/l29ah/booboo>`__

Information on using Gentoo overlays can be found
`here <http://www.gentoo.org/proj/en/overlays/userguide.xml>`__

openSuSE
--------

unofficial packages available from the open build service for openSUSE
Tumbleweed, Leap 42.2 and Leap 42.3

Ubuntu
------

unofficial packages available on the universe repository of all
currently supported versions

Building
========

See `Development <Development>`__
