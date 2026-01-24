.. _fedora_dependencies:

Fedora dependencies
===================

Compilation tools
-----------------

-  gettext-devel (provides autopoint)
-  libtool (will install a bunch of other needed packages)
-  glib2-devel
-  cvs
-  python-devel


OpenGL GUI
----------

cegui-devel freeglut-devel quesoglc-devel SDL-devel libXmu-devel


GPSD Support
------------

gpsd-devel


GTK Gui
-------

-  gtk2-devel


Speech support
--------------

-  speech-dispatcher-devel


Installing all dependencies
---------------------------

.. bash
   su -

   yum install gettext-devel libtool glib2-devel cegui-devel freeglut-devel \
   quesoglc-devel SDL-devel libXmu-devel gpsd-devel gtk2-devel \
   speech-dispatcher-devel cvs python-devel saxon-scripts

   exit


Now continue and follow compilation instructions on:
http://wiki.navit-project.org/index.php/NavIt_on_Linux
