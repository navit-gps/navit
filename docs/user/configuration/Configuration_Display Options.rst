.. _configurationdisplay_options:

Configuration/Display Options
=============================

.. _display_options:

Display Options
---------------

The Navit display is highly customisable and consists of the following
components

-  Graphics driver (appropriate engine for your system, to draw
   everything)
-  GUI (enables user interaction and map display)
-  OSD (shows widgets on map screen)

.. _graphics_driver:

Graphics Driver
~~~~~~~~~~~~~~~

Different technologies can be used, to let Navit draw it's visual
components. Not all might be available at your specific system

The current list of available graphics drivers:

-  **android**, for the Android port
-  **cocoa**, for the iPhone port
-  **gtk_drawing_area**, usually most appropriate on Linux desktop
   systems
-  **sdl**, render inside an X window, or direct to the Linux
   framebuffer, with min dependencies on external libraries.
-  **qt5**, render using Qt5 library, either using QWidgets or QML. On
   top of any Qt5 supported display technology.
-  **win32** - useable with ``gtk`` or ``internal`` GUIs for Windows
   systems only.

Experimental/less maintained drivers:

-  **qt_qpainter**, render inside X window or on top of Qt Palmtop
   Environment.
-  **opengl**, rendering via OpenGL
-  **gtk_gl_ext**, rendering via OpenGL using GTK+ OpenGL extension
-  **gd**, rendering using the GD Graphics Library

They can be activated and configured as following:

.. code:: xml

    <graphics type="gtk_drawing_area"/>

As mentioned, it's usually best to leave this as whatever the default is
within your ``navit.xml``, and only mess around with it if you know what
you are doing, or have been told to by one of the developers.

.. _graphical_user_interface:

Graphical User Interface
~~~~~~~~~~~~~~~~~~~~~~~~

You can now choose which type of GUI you would like to use with Navit.
Not all GUIs work with all Graphics drivers

+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| GUI   | an    | cocoa | gd    | gtk_d | gtk_g | o     | q     | sdl   | win32 | qt5   | pr    |
|       | droid |       |       | rawin | l_ext | pengl | t_qpa |       |       |       | eview |
|       |       |       |       | g_are |       |       | inter |       |       |       |       |
+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+=======+
| | \   |       |       |       |       |       |       |       |       |       |       |       |
|  `ceG |       |       |       |       |       |       |       |       |       |       |       |
| UI <c |       |       |       |       |       |       |       |       |       |       |       |
| eGUI> |       |       |       |       |       |       |       |       |       |       |       |
| `__\  |       |       |       |       |       |       |       |       |       |       |       |
| |     |       |       |       |       |       |       |       |       |       |       |       |
|  (unm |       |       |       |       |       |       |       |       |       |       |       |
| ainta |       |       |       |       |       |       |       |       |       |       |       |
| ined) |       |       |       |       |       |       |       |       |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| `GTK  |       |       |       | ..    |       |       |       |       | ..    |       | ..    |
| G     |       |       |       | raw:: |       |       |       |       | raw:: |       |  figu |
| UI <G |       |       |       |  medi |       |       |       |       |  medi |       | re::  |
| TK_GU |       |       |       | awiki |       |       |       |       | awiki |       | Navit |
| I>`__ |       |       |       |       |       |       |       |       |       |       | -libe |
|       |       |       |       |    {{ |       |       |       |       |    {{ |       | ratio |
|       |       |       |       | yes}} |       |       |       |       | yes}} |       | n.png |
|       |       |       |       |       |       |       |       |       |       |       |    :  |
|       |       |       |       |       |       |       |       |       |       |       | alt:  |
|       |       |       |       |       |       |       |       |       |       |       | Navit |
|       |       |       |       |       |       |       |       |       |       |       | -libe |
|       |       |       |       |       |       |       |       |       |       |       | ratio |
|       |       |       |       |       |       |       |       |       |       |       | n.png |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |  :hei |
|       |       |       |       |       |       |       |       |       |       |       | ght:  |
|       |       |       |       |       |       |       |       |       |       |       | 100px |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       | Navit |
|       |       |       |       |       |       |       |       |       |       |       | -libe |
|       |       |       |       |       |       |       |       |       |       |       | ratio |
|       |       |       |       |       |       |       |       |       |       |       | n.png |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| `int  | ..    | ..    | ..    | ..    | ..    | ..    | ..    | ..    | ..    | ..    | .. fi |
| ernal | raw:: | raw:: | raw:: | raw:: | raw:: | raw:: | raw:: | raw:: | raw:: | raw:: | gure: |
| G     |  medi |  medi |  medi |  medi |  medi |  medi |  medi |  medi |  medi |  medi | : Int |
| UI <i | awiki | awiki | awiki | awiki | awiki | awiki | awiki | awiki | awiki | awiki | ernal |
| ntern |       |       |       |       |       |       |       |       |       |       | GUI-A |
| al_GU |    {{ |    {{ |    {{ |    {{ |    {{ |    {{ |    {{ |    {{ |    {{ |    {{ | ction |
| I>`__ | yes}} | yes}} | yes}} | yes}} | yes}} | yes}} | yes}} | yes}} | yes}} | yes}} | s.png |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |  :alt |
|       |       |       |       |       |       |       |       |       |       |       | : Int |
|       |       |       |       |       |       |       |       |       |       |       | ernal |
|       |       |       |       |       |       |       |       |       |       |       | GUI-A |
|       |       |       |       |       |       |       |       |       |       |       | ction |
|       |       |       |       |       |       |       |       |       |       |       | s.png |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |  :hei |
|       |       |       |       |       |       |       |       |       |       |       | ght:  |
|       |       |       |       |       |       |       |       |       |       |       | 100px |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |   Int |
|       |       |       |       |       |       |       |       |       |       |       | ernal |
|       |       |       |       |       |       |       |       |       |       |       | GUI-A |
|       |       |       |       |       |       |       |       |       |       |       | ction |
|       |       |       |       |       |       |       |       |       |       |       | s.png |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| | \   |       |       |       |       |       |       | ..    |       |       | ..    | .. f  |
|  `QML |       |       |       |       |       |       | raw:: |       |       | raw:: | igure |
|   GUI |       |       |       |       |       |       |  medi |       |       |  medi | :: Qm |
|  <QML |       |       |       |       |       |       | awiki |       |       | awiki | l_poi |
| _GUI> |       |       |       |       |       |       |       |       |       |       | nt_20 |
| `__\  |       |       |       |       |       |       |    {{ |       |       |    {  | 10040 |
| |     |       |       |       |       |       |       | yes}} |       |       | {no}} | 4.png |
|  (unm |       |       |       |       |       |       |       |       |       |       |       |
| ainta |       |       |       |       |       |       |       |       |       |       |   :al |
| ined) |       |       |       |       |       |       |       |       |       |       | t: Qm |
|       |       |       |       |       |       |       |       |       |       |       | l_poi |
|       |       |       |       |       |       |       |       |       |       |       | nt_20 |
|       |       |       |       |       |       |       |       |       |       |       | 10040 |
|       |       |       |       |       |       |       |       |       |       |       | 4.png |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |  :hei |
|       |       |       |       |       |       |       |       |       |       |       | ght:  |
|       |       |       |       |       |       |       |       |       |       |       | 100px |
|       |       |       |       |       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |       |       |       |    Qm |
|       |       |       |       |       |       |       |       |       |       |       | l_poi |
|       |       |       |       |       |       |       |       |       |       |       | nt_20 |
|       |       |       |       |       |       |       |       |       |       |       | 10040 |
|       |       |       |       |       |       |       |       |       |       |       | 4.png |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+

.. _generic_gui_options:

Generic GUI Options
^^^^^^^^^^^^^^^^^^^

There are some options available for the ``gui`` tag which are used by
all the GUI types. These include:

-  **fullscreen** - Enables Navit to start in fullscreen mode.
-  **pitch** - The pitch value to pitch the map to when selecting 3D
   mode from the menus.
-  **dimensions** - w="1024" h="600"

The following example uses the ``internal`` GUI, and starts Navit up in
fullscreen mode, and will pitch the map to 35 degrees when 3D mode is
selected from the menu. Note that to start Navit in 3D mode by default,
`change the ``pitch`` value in the ``navit`` tag <#Initial_3D_pitch>`__:

.. code:: xml

    <gui type="internal" enabled="yes" fullscreen="1" pitch="35">

.. _internal_gui:

Internal GUI
^^^^^^^^^^^^

The first GUI is embedded in Navit core and is primarily aimed at
`touchscreen <touchscreen>`__ devices, or those devices with small
screens (such as netbooks). However, this GUI also works very well on
desktops and laptops.

.. code:: xml

    <gui type="internal" enabled="yes">

Options
'''''''

A number of options specific to the ``internal`` GUI are available.
These include:

-  **font_size** - Base text size to use within the ``internal`` menu.
-  **icon_xs** - The size that extra-small style icons should be scaled
   to (e.g. country flag on town search).
-  **icon_s** - The size that small style icons should be scaled to
   (e.g. icons of ``internal`` GUI toolbar).
-  **icon_l** - The size that large style icons should be scaled to
   (e.g. icons of internal GUI menu).
-  **menu_on_map_click** - Toggles the ability to bring up the menu
   screen when clicking on the map. See the ```internal`` GUI
   page <Internal_Gui#Menu_Configuration>`__ for more information.

An example ``gui`` tag using the previous options is shown below:

.. code:: xml

    <gui type="internal" enabled="yes" font_size="250" icon_xs="48" icon_s="48" icon_l="64">

More options are discussed on the `Internal Gui <Internal_Gui>`__ and
the `full list of options <Configuration/Full_list_of_options>`__.

.. _gtk_gui:

GTK GUI
^^^^^^^

The second GUI is called **gtk**, and is most useful for those users who
wish to use a traditional windowed GUI. This is one useful to desktop
use.

.. code:: xml

    <gui type="gtk" enabled="yes" ... />

.. _options_1:

Options
'''''''

A number of options specific to the ``gtk`` GUI are available. These
include:

-  menubar - enable/disable the menubar
-  toolbar - enable/disable the toolbar
-  statusbar - enable/disable the statusbar

.. code:: xml

    <gui type="gtk" enabled="yes" menubar="1" toolbar="1" statusbar="1"/>

.. _on_screen_display:

On Screen Display
~~~~~~~~~~~~~~~~~

`OSD <OSD>`__
