.. _internal_gui:

Internal GUI
============

The Internal GUI is designed to be used on touch screen devices, but
also work very well on other devices such as netbooks and laptops. It is
under continual development and as such features are constantly being
added and improved upon. If you think that a particular feature is
missing or poorly implemented, come talk to us in the `irc
channel <Contacts#IRC>`__ or open up a feature request on the `trac
system <Contacts#trac>`__.

.. _configuring_internal_gui:

Configuring Internal GUI
------------------------

.. _enabling_internal_gui:

Enabling Internal GUI
~~~~~~~~~~~~~~~~~~~~~

The Internal GUI is configured as the default GUI for Navit, so if
you're reading this after a first install no further configuration is
required.

If the configuration has changed since first install, the Internal GUI
can be chosen by setting the ``type`` attribute in the tag.

Ensure that any other ``gui`` tags are disabled by setting their
``enabled`` attribute to "no".

.. _keyboard_preferences:

Keyboard Preferences
~~~~~~~~~~~~~~~~~~~~

Some options inside the Internal GUI menu require keyboard input - for
example, Town search. By default, Navit provides a custom on-screen
keyboard to enter text. If your device has it's own keyboard which you'd
prefer to use, and you'd like to conserve some screen space then set the
``keyboard`` attribute to "false" inside the tag.

.. _map_click_preferences:

Map-click Preferences
~~~~~~~~~~~~~~~~~~~~~

By default, the menu appears when the map is clicked. This can be
disabled by adding the following to the tag:

``menu_on_map_click="0"``

This of course means that you now can't access the menu by clicking on
the map. Instead you will have to add an `OSD <OSD>`__ item with the
`command <OSD#GUI_commands>`__ ``gui.menu()``, as shown in the example
below:

You can use the following example to enable/disable the
``menu_on_map_click`` behaviour from an `OSD#button <OSD#button>`__:

.. _icon_and_font_sizes:

Icon and font sizes
~~~~~~~~~~~~~~~~~~~

You can also configure the icon sizes used in the menu. The syntax is as
follows:

``font_size`` is off course the font size, ``icon_xs`` is the size of
the green "ticks" you see `here <Internal_GUI#Map_Point>`__. ``icon_s``
is the size of the world and home icon and of the icons in the `POI
selector <Internal_GUI#POIs>`__. Finally, ``icon_l`` is the size of the
icons `defined in the menu html <Internal_GUI/Menu_configurations>`__.
The icon sizes need to be available as files in Navit (they should be by
default).

.. _menu_configuration:

Menu Configuration
~~~~~~~~~~~~~~~~~~

Using Internal GUI, the menu can be brought up by clicking (almost)
anywhere on the map, or pressing the Enter (Return) key on the device's
keyboard.

The menu is configured using a html-like syntax inside the ... tags. Of
course, this configuration can be customised - alternative
configurations can be found in `Internal GUI/Menu
configurations <Internal_GUI/Menu_configurations>`__.

.. _initial_start_up:

Initial Start-up
~~~~~~~~~~~~~~~~

|N810-OSD-Home.png| When Navit is first started using the Internal GUI
one should see (depending on the skin you have selected to use)
something similar to the image to the right. The layout of the internal
GUI is controlled by the OSD tags located in the navit.xml file. These
tags should be located within the first 100 lines of the file. For
information on how to modify the appearance of the OSD layout please
reference this link. `OSD Layout <OSD>`__

.. _using_the_internal_gui:

Using the Internal GUI
----------------------

.. _basics_and_breadcrumbs:

Basics and breadcrumbs
~~~~~~~~~~~~~~~~~~~~~~

The Internal GUI should be mostly self-explanatory (that's the idea, at
least - if it is not, please file a bug). It basically consists of
different screens which show icons that can be clicked / touched, lists
(such as search results) and input fields. For text input, a **virtual
keyboard** is available. Of course, a regular hardware keyboard can be
used if available.

On all screens of the Internal GUI, there is a **breadcrumb trail** at
the top of the screen, which shows the current position inside the
screen hierarchy of the Internal GUI. The breadcrumbs are clickable, to
return to an earlier screen.

.. _operation_with_keyboard_or_rotary_encoder:

Operation with keyboard or rotary encoder
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While the Internal GUI is mainly designed to be used with a mouse or
touch screen, it can also be operated using a (hardware) keyboard or
even a `rotary encoder <https://en.wikipedia.org/wiki/Rotary_encoder>`__
(which only offers "forward", "backward" and "enter").

The GUI elements can be navigated using arrow keys, and activated using
Enter, as usual. Additionally, all GUI elements can also be reached
using only PgUp/PgDown - this allows the use of a rotary encoder, if its
actions are mapped to these two keys.

When using a rotary encoder (or cursor keys), it may be useful to set
option hide_impossible_next_keys to hide irrelevant keys when searching.
See the `full list of
options <Configuration/Full_list_of_options#gui>`__ for details.

*Support for rotary encoders was added in December 2015, and
hide_impossible_next_keys in February 2017.*

.. _main_menu:

Main Menu
---------

|InternalGui-MainMenu.png| The main menu is accessed by a single click
(or tap for touch screen) anywhere on the map. From here all other
sub-menus and actions are accessible. The sub menu items are:

#. `Actions <Internal_Gui#Actions>`__
#. `Settings <Internal_Gui#Settings>`__
#. `Tools <Internal_Gui#Tools>`__
#. `Route <Internal_Gui#Route>`__
#. `About <Internal_Gui#About>`__

| 

Actions
~~~~~~~

|InternalGUI-Actions.png| The Actions menu brings up several sub menus
that are focused primarily on routing and location finding. The sub menu
items are:

#. `Bookmarks <Internal_Gui#Bookmarks>`__
#. `Former destinations <Internal_Gui#Former_destinations>`__
#. `Map Point <Internal_Gui#Map_Point>`__
#. `Current Location <Internal_Gui#Current_Location>`__
#. `Town <Internal_Gui#Town>`__
#. Quit - Closes Navit

| 

Bookmarks
^^^^^^^^^

|InternalGUI-Bookmarks.png| Bookmarks provide a convenient way to store
often used destinations. Since Navit does not fully support entering a
complete address using OpenStreetMap maps, a user can locate some
oft-used destinations on the map and then add that point as a bookmark.
That way the next time the user would like a route for that particular
destination the user only has to select it from the Bookmarks menu and
does not have to go through the tedium of panning the map and zooming
into the destination location.

Bookmarks can be arranged hierarchical using / as a separator - anything
before the separator becomes the folder name; anything after the
separator becomes the bookmark name. For example, if you name your
bookmarks Friends/Joe and Friends/Bill, you will have a folder named
Friends and the bookmarks Bill and Joe in there.

| A fully functioning bookmark editor is currently not available, though
  some common edits can be performed from within the Bookmarks menu.
  Bookmarks are stored in a plain-text bookmarks file in your Navit
  directory (~/.navit on unix systems).

.. _former_destinations:

Former destinations
^^^^^^^^^^^^^^^^^^^

A list of the last destinations that were set in Navit. Every time a
destination is set (via a bookmark, via a map point or by searching for
an address and choosing it as a destination), the destination will be
added to this list.

The list of former destinations is a convenience feature, to quickly
reuse a destination. The functionality offered is similar to the
Bookmarks menu (however, the list cannot be edited, as it is meant as a
record of the destinations selected).

To prevent the list from getting too long to be useful, only the last 10
destinations are kept. This limit can be changed in navit.xml (attribute
"recent_dest" in element ""). So normally, each time a new destination
is selected, it will be added to the list, and the oldest entry in the
list will be discarded. As an exception, if a destination is set that is
already in the list, it will not be repeated in the list; instead the
entry will just be moved to the top.

.. _map_point:

Map Point
^^^^^^^^^

|InternalGUI-MapPoint.png| The world icon brings up the Map Point sub
menu for actions that can be performed for the point that was selected
on the map. The items contained in this sub menu are:

-  Set as Destination: Will generate a route to that location from
   either current GPS data or where vehicle position is manually set
   (see Vehicle Position).
-  Set as Position: If no GPS data is available then you can specify
   your "current" location in order to have a route generated from that
   position to your desired destination.
-  Add as Bookmark: Brings up a keyboard so a name can be entered for
   the bookmark. The point can then be easily recalled via the
   `Bookmark <Internal_GUI#Bookmarks>`__ menu.
-  POIs: Brings up a list of all known POIs around the map point.

| 

POIs
''''

| |InternalGUI-POIs.png| The POIs sub menu shows all of the POIs that
  are close to the location that was clicked on the map, with the
  distance to the POI shown in kilometres. At the top of the menu there
  are various filter options that allow for specifying the types of POIs
  to be displayed. The user can click on the POI and select to be routed
  to that location. Navit will create a route from the current position
  to the location of the POI selected.

.. _vehicle_position:

Vehicle Position
^^^^^^^^^^^^^^^^

|InternalGUI-VehiclePosition.png| The vehicle icon brings up the Current
Location sub menu. This sub menu allows for various actions to be taken
for the GPS position of the device.

-  Set as Destination: Will generate a route to that location from
   either current GPS data or where vehicle position is manually set
   (see Vehicle Position).
-  Set as Position: If no GPS data is available then you can specify
   your "current" location in order to have a route generated from that
   position to your desired destination.
-  Add as Bookmark: Brings up a keyboard so a name can be entered for
   the bookmark. The point can then be easily recalled via the
   `Bookmark <Internal_GUI#Bookmarks>`__ menu.
-  POIs: Brings up a list of all known POIs around the map point.
-  View on Map: Re-pans the map to display the current "known" position
   based upon GPS data.

| 

Town
^^^^

|InternalGui-Town.png| The town icon allows for searching for different
cities within your map set. Note that Navit attempts to auto complete
the town name based upon names available in the mapset being used. On
slow devices this can result in a slight pause as each character is
typed in. Once a town is located and selected another sub menu will come
up allowing for a street to be found within that town.

The icon in the upper left corner (just below the world icon) shows the
current country which is being searched. To change the country just
click on the icon and another menu will appear allowing you to select
the country you would like to search in. Note that this menu also
attempts to auto complete as the user types in the name of a country.

Note that if you compiled Navit yourself there can be issues with the
icons not being properly generated. This will result in no icon image at
all. If you have this problem check your logs to see what is happening
during compiling.

| If you are having problems with search, please check
  `OpenStreetMap#Problems_with_OSM_and_navit_or_navigation_in_general <OpenStreetMap#Problems_with_OSM_and_navit_or_navigation_in_general>`__.

Settings
~~~~~~~~

|InternalGUI-Settings.png| The settings menu provides several sub menus
to enable certain aspects of how Navit operates to be modified. Note
that at this time there is only a limited set of options that can be
changed through these sub menus. In order to change settings not
currently available in this sub-menu it is necessary to modify the
navit.xml file. At some point in the future a more robust settings menu
will be implemented that will allow for configuring Navit through a GUI
instead of the navit.xml file.

The sub menu items are:

#. `Display <Internal_Gui#Display>`__
#. `Maps <Internal_Gui#Maps>`__
#. `Vehicle <Internal_Gui#Vehicle>`__
#. `Rules <Internal_Gui#Rules>`__

| 

Display
^^^^^^^

|InternalGUI-Display.png| The display sub menu provides items to control
various display features within Navit.

#. `Layout <Internal_Gui#Layout>`__
#. `Fullscreen/Window Mode <Internal_Gui#Window_Mode>`__
#. `3D <Internal_Gui#3D>`__

| 

Layout
''''''

| |InternalGUI-Layout.png| Layout allows for different layouts specified
  in the navit.xml file to be shown on the map. Different layouts can be
  used for different reasons including allowing one to see other friends
  position (if their GPS data is specified in the layout tag). Note that
  layout options MUST be enabled in the navit.xml file before they can
  be turned on or off in this menu.

.. _window_mode_toggle:

Window Mode (Toggle)
''''''''''''''''''''

Changes Navit from windowed mode to fullscreen mode and vice versa.

.. _d_toggle:

3D (Toggle)
'''''''''''

This is a toggle button that enables / disables drawing the map in
either a 2D mode or a 3D mode. Currently the only way to modify the
"tilt" for the 3D mode is to modify the navit.xml file.

Maps
^^^^

| |InternalGUI-Maps.png| Displays the maps that are specified in the
  navit.xml file and allows for activating/de-activating those maps.
  Note that a map must be enabled in navit.xml before it will appear in
  this menu.

Vehicle
^^^^^^^

|InternalGUI-Vehicle.png| Brings up a menu showing what GPS device is
currently being used for the current vehicle. Tapping the GPS device
name opens a menu with available routing profiles.

| 

Rules
^^^^^

| |InternalGUI-Rules.png| The rules menu provides for options that
  change how Navit behaves when there is a satellite lock. Note that
  some of these items are currently not function and must be changed in
  the navit.xml file.

Tools
~~~~~

The tools menu allows the user to check what Locale Navit is set to.

Route
~~~~~

|InternalGUI-Route.png| The route icon brings up the route menu that
will display the active route.

#. `Route Description <Internal_Gui#Route_Description>`__
#. Height Profile, requires a dedicated binfile to providing
   heightlines.

| 

.. _route_description:

Route Description
^^^^^^^^^^^^^^^^^

|InternalGUI-RouteDescription.png| The route description sub menu
displays all of the directions for the currently calculated route.

.. |N810-OSD-Home.png| image:: N810-OSD-Home.png
   :width: 300px
.. |InternalGui-MainMenu.png| image:: InternalGui-MainMenu.png
   :width: 300px
.. |InternalGUI-Actions.png| image:: InternalGUI-Actions.png
   :width: 300px
.. |InternalGUI-Bookmarks.png| image:: InternalGUI-Bookmarks.png
   :width: 300px
.. |InternalGUI-MapPoint.png| image:: InternalGUI-MapPoint.png
   :width: 300px
.. |InternalGUI-POIs.png| image:: InternalGUI-POIs.png
   :width: 300px
.. |InternalGUI-VehiclePosition.png| image:: InternalGUI-VehiclePosition.png
   :width: 300px
.. |InternalGui-Town.png| image:: InternalGui-Town.png
   :width: 300px
.. |InternalGUI-Settings.png| image:: InternalGUI-Settings.png
   :width: 300px
.. |InternalGUI-Display.png| image:: InternalGUI-Display.png
   :width: 300px
.. |InternalGUI-Layout.png| image:: InternalGUI-Layout.png
   :width: 300px
.. |InternalGUI-Maps.png| image:: InternalGUI-Maps.png
   :width: 300px
.. |InternalGUI-Vehicle.png| image:: InternalGUI-Vehicle.png
   :width: 300px
.. |InternalGUI-Rules.png| image:: InternalGUI-Rules.png
   :width: 300px
.. |InternalGUI-Route.png| image:: InternalGUI-Route.png
   :width: 300px
.. |InternalGUI-RouteDescription.png| image:: InternalGUI-RouteDescription.png
   :width: 300px
