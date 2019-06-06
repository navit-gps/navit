Display Options
---------------
The Navit display is highly customisable and consists of the following components

* Graphics driver (appropriate engine for your system, to draw everything)
* GUI (enables user interaction and map display)
* OSD (shows widgets on map screen)

Graphics Driver
---------------

Different technologies can be used, to let Navit draw it's visual components. Not all might be available at your specific system

The current list of available graphics drivers:

* **android**, for the Android port
* **cocoa**, for the iPhone port
* **gtk_drawing_area**,  usually most appropriate on Linux desktop systems
* **sdl**, render inside an X window, or direct to the Linux framebuffer, with min dependencies on external libraries.
* **qt5**, render using Qt5 library, either using QWidgets or QML. On top of any Qt5 supported display technology.
* **win32** - useable with <tt>gtk</tt> or <tt>internal</tt> GUIs for Windows systems only.

Experimental/less maintained drivers:

* **qt_qpainter**, render inside X window or on top of Qt Palmtop Environment.
* **opengl**, rendering via OpenGL
* **gtk_gl_ext**, rendering via OpenGL using GTK+ OpenGL extension
* **gd**, rendering using the GD Graphics Library

They can be activated and configured as following:

.. code-block:: xml

 <graphics type="gtk_drawing_area"/>

As mentioned, it's usually best to leave this as whatever the default is within your :xml:`navit.xml`, and only mess around with it if you know what you are doing, or have been told to by one of the developers.

Graphical User Interface
------------------------
You can now choose which type of GUI you would like to use with Navit. Not all GUIs work with all Graphics drivers


======== ======= ===== ==== ================ ========== ====== =========== === ===== === =======
GUI      android cocoa gd   gtk_drawing_area gtk_gl_ext opengl qt_qpainter sdl win32 qt5 preview
======== ======= ===== ==== ================ ========== ====== =========== === ===== === =======
GTK
Internal Yes     Yes   Yes  Yes              Yes        Yes    Yes         Yes Yes   Yes
QML2
======== ======= ===== ==== ================ ========== ====== =========== === ===== === =======

Generic GUI Options
~~~~~~~~~~~~~~~~~~~
There are some options available for the ``gui`` tag which are used by all the GUI types. These include:

* **fullscreen** - Enables Navit to start in fullscreen mode.
* **pitch** - The pitch value to pitch the map to when selecting 3D mode from the menus.
* **dimensions** - w="1024" h="600"

The following example uses the :xml:`internal` GUI, and starts Navit up in fullscreen mode, and will pitch the map to 35 degrees when 3D mode is selected from the menu. Note that to start Navit in 3D mode by default, [[#Initial 3D pitch|change the :xml:`pitch` value in the :xml:`navit` tag]]:

.. code-block:: xml

 <gui type="internal" enabled="yes" fullscreen="1" pitch="35">

Internal GUI
~~~~~~~~~~~~
The first GUI is embedded in Navit core and is primarily aimed at [[touchscreen]] devices, or those devices with small screens (such as netbooks). However, this GUI also works very well on desktops and laptops.

.. code-block:: xml

 <gui type="internal" enabled="yes">

Options
^^^^^^^
A number of options specific to the ``internal`` GUI are available. These include:

* **font_size** - Base text size to use within the ``internal`` menu.
* **icon_xs** - The size that extra-small style icons should be scaled to (e.g. country flag on town search).
* **icon_s** - The size that small style icons should be scaled to (e.g. icons of ``internal`` GUI toolbar).
* **icon_l** - The size that large style icons should be scaled to (e.g. icons of internal GUI menu).
* **menu_on_map_click** - Toggles the ability to bring up the menu screen when clicking on the map. See the [[Internal_Gui#Menu_Configuration|``internal`` GUI page]] for more information.

An example ``gui`` tag using the previous options is shown below:

.. code-block:: xml

 <gui type="internal" enabled="yes" font_size="250" icon_xs="48" icon_s="48" icon_l="64">

More options are discussed on the [[Internal Gui]] and the [[Configuration/Full_list_of_options|full list of options]].

GTK GUI
~~~~~~~
The second GUI is called **gtk**, and is most useful for those users who wish to use a traditional windowed GUI. This is one useful to desktop use.

.. code-block:: xml

 <gui type="gtk" enabled="yes" ... />


Options
^^^^^^^
A number of options specific to the ``gtk`` GUI are available. These include:

* menubar - enable/disable the menubar
* toolbar - enable/disable the toolbar
* statusbar - enable/disable the statusbar


.. code-block:: xml

 <gui type="gtk" enabled="yes" menubar="1" toolbar="1" statusbar="1"/>
