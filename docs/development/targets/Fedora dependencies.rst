.. _fedora_dependencies:

Fedora dependencies
===================

.. raw:: mediawiki

   {{warning|1='''This is a page has been migrated to readthedocs:'''https://github.com/navit-gps/navit/pull/880 . It is only kept here for archiving purposes.}}

.. _compilation_tools:

Compilation tools
=================

-  gettext-devel (provides autopoint)
-  libtool (will install a bunch of other needed packages)
-  glib2-devel
-  cvs
-  python-devel

.. _opengl_gui:

OpenGL GUI
==========

cegui-devel freeglut-devel quesoglc-devel SDL-devel libXmu-devel

.. _gpsd_support:

GPSD Support
============

gpsd-devel

.. _gtk_gui:

GTK Gui
=======

-  gtk2-devel

.. _speech_support:

Speech support
==============

-  speech-dispatcher-devel

.. _installing_all_dependencies:

Installing all dependencies
===========================

su -

yum install gettext-devel libtool glib2-devel cegui-devel freeglut-devel
quesoglc-devel SDL-devel libXmu-devel gpsd-devel gtk2-devel
speech-dispatcher-devel cvs python-devel saxon-scripts

exit

Now continue and follow compilation instructions on:
http://wiki.navit-project.org/index.php/NavIt_on_Linux
