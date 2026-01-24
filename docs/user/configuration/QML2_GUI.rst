.. _qml2_gui:

QML2 GUI
========

The QML2 UI is a new UI being currently developed to provide a more
modern look and feel to Navit.

==================== ====================================
Tablet / PC          Mobile UI
==================== ====================================
.. figure:: Qml2.gif .. figure:: Gui_qml_drawer_popup.gif
   :alt: Qml2.gif       :alt: Gui_qml_drawer_popup.gif
                     
   Qml2.gif             Gui_qml_drawer_popup.gif
==================== ====================================

.. _prebuilt_image:

Prebuilt image
==============

We have a prebuilt image for raspberry pi 2/3. This is a preview and
will require more work, but feedback is welcome.

-  download here : https://cloud.kazer.org/index.php/s/QkSgrHoARq2BZC8
-  flash to your card : dd if=rpi3-sdcard.img of=/dev/
-  log in as root, no password required
-  start Navit!

Tweaks (default config file is in /usr/share/navit/navit/xml) :

-  you might want to tweak the default zoom setting (in this image it's
   32)
-  you can disable the qt5_qml GUI and switch back to internal, it'll
   still use EGL

Prerequisites
=============

The QML2 UI is currently developed against QT 5.7

The easiest way to install QT 5.7 (or greater) is probably to use the QT
online installer.

For linux,
https://download.qt.io/archive/online_installers/2.0/qt-unified-linux-x64-2.0.5-2-online.run.mirrorlist

For other platforms,
https://download.qt.io/archive/online_installers/2.0/

Building
========

If you have Qt5 installation in standard paths, simply *cmake* will
create the Makefile and you can proceed with it. When the Qt5
installation is in the non standard paths, you have to use
**CMAKE_INSTALL_PREFIX** for navit to find Qt5 files.

`` cmake -DCMAKE_INSTALL_PREFIX=$QT_INSTALL_PREFIX_PATH  $NAVIT_SOURCE_PATH``

.. _enabling_the_qml2_ui:

Enabling the QML2 ui
====================

Once you compiled Navit, you can enable the QML2 ui from navit.xml.

-  Change your graphics driver to qt5 :

::

   <graphics type="qt5"/>

-  Enable the QML2 UI :

::

   <gui type="qt5_qml" enabled="yes" />

-  Disable the internal UI :

::

   <gui type="internal" enabled="no">
