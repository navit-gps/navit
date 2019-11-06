.. _configuration:

*************
Configuration
*************

Navit is highly modular and customizable. This page aims to point out the most common options which a first-time user may want to change - power users may want to consult [[Configuration/Full_list_of_options | the full list of options]].
It is also possible to edit the navit.xml file for your Android device under Windows and Linux (Debian/Ubuntu derivates) with a third party application called [[NavitConfigurator]].

Setting up Navit is done by editing a configuration file called "**navit.xml**".
Editing XML configurations files in a text editor is simple, they are just plain text XML files, that can be edited with any editor. Just remember to ''turn off 'save UTF8 byte mark' in Preferences'' or navit may complain very much on the first byte of the file.<br>
The XML configuration file is splitted into sections within a hierarchy:

.. code-block:: xml

	<config>
		<plugins></plugins>
		<navit>
			<osd></osd>
			<vehicle></vehicle>
			<vehicleprofile></vehicleprofile>
			<mapset></mapset>
			<layout></layout>
		</navit>
	</config>

Navit comes **preshipped** with a default ``navit.xml`` together with ``navit_layout_*.xml`` files that are stored at various locations (depending on your system).

For Linux-like OSes:
 * in ``~/.navit/``: e.g: ``/home/myusername/.navit/navit.xml`` (This is probably to best place to customize your settings!)
 * in ``/usr/share/navit`` or ``/etc/navit``

Navit will apply settings in the following order:
 * in the current directory (used on Windows)
 * location supplied as first argument on the command line, e.g.: ``navit /home/myusername/navittestconfig.xml`` (Used mainly for development)
 * in the current directory as ``navit.xml.local`` (Used mainly for development)

{{note|In any case, you have to **adapt settings** to your system!<br> This includes especially GPS, map provider and vehicle: [[Basic configuration]]}}

*********************
Configurable Sections
*********************

General
'''''''
Common options such as units, position, zoom and map orientation, ... be configured in this section.

.. include:: general.rst

Display
'''''''
A large number of display properties can be configured, including desktop or touchscreen-optimised GUIs, on-screen display items and complete control over menu items.

.. include:: display.rst

Vehicle
'''''''
A number of vehicles can be defined within Navit, depending upon the device and/or operating system in use. Vehicle profiles for routing (eg: car, pedestrian, bicycle...) are also completely configurable.

.. include:: vehicle.rst

Maps
''''
You can use maps from a variety of sources, any number of maps can be configured and enabled at any one time.


Layout
''''''
Maps are displayed according to the rules defined in the layout. All aspects of the layout are configurable, from POI icons to colours for a particular type of highway.

For all versions shipped after nov 2018, layout XML configuration is stored in dedicated XML files called with the prefix ´´navit_layout_´´ (one file per layout definition).

.. include:: layout.rst

Advanced
''''''''
There are many more options, including debugging, specific plugins, speech announcements, trip logging, ...

.. include:: advanced.rst












