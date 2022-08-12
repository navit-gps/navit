Basic Configuration
===================

Basics
''''''

This page aims to point out the most common options which a first-time user may want to change - power users may want to consult the :doc:`advanced`.

.. todo::
    It is also possible to edit the navit.xml file for your Android device under Windows and Linux (Debian/Ubuntu derivates) with a third party application called [[NavitConfigurator]].

Setting up Navit is done by editing a configuration file called "**navit.xml**".

Editing XML configurations files in a text editor is simple, they are just plain text XML files, that can be edited with any editor.
Just remember to ''turn off 'save UTF8 byte mark' in Preferences'' or navit may complain very much on the first byte of the file.

.. hint::
    If you are unfamiliar with XML files you can take a look at `Learn XML in Y minutes <https://learnxinyminutes.com/docs/xml/>`__

The Navit Configuration XML is splitted into sections with this hierarchy:

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

Navit comes **shipped** with a default ``navit.xml`` together with ``navit_layout_*.xml`` files that are stored at various locations (depending on your system).

For Linux-like OSes:
 * in ``~/.navit/``: e.g: ``/home/myusername/.navit/navit.xml`` (This is probably to best place to customize your settings!)
 * in ``/usr/share/navit`` or ``/etc/navit``

Navit will apply settings in the following order:
 * in the current directory (used on Windows)
 * location supplied as first argument on the command line, e.g.: ``navit /home/myusername/navittestconfig.xml``
 * in the current directory as ``navit.xml.local`` (Used mainly for development)
 * in the system configuration-directory

.. note::
    In any case, you have to **adapt settings** to your system!
    This includes especially GPS, map provider and vehicle.



***********
Preparation
***********
Get a supported map package
---------------------------
Navit can use different :doc:`maps/index` formats, including free `OpenStreetMap` data. In order to use one of these maps, download a map of your desired area and store it into a local folder (such as navit/maps or /usr/share/navit/maps).

Install TTS
-----------
To get speech support, you need to install a text-to-speech tool such as **espeak**, **mbrola** or **festival**. These tools can be invoked from the command line. Test your setup by invoking the tools manually e.g.

.. code-block:: bash

  espeak "This is a text!"

On some systems Navit comes with integrated espeak support, so you don't need to download it separately.

Connect GPS
-----------
Now connect your GPS. The exact procedure for this varies depending on the type of GPS device you are using and how you connect it to your computer. On Linux / Unix systems, your GPS should typically show up as a character device, i.e., you will find an entry in the /dev folder corresponding to your GPS device. Again, the file name depends on the type and connection method of your GPS receiver. See [[Connecting a GPS receiver]] for details.

Most GPS receiver will output the position in [[NMEA]] format, which can be used directly in Navit or using a GPS daemon program such as **gpsd**. A simple way to test whether your GPS receiver works and outputs NMEA data is to dump its output to a console. For example, if your GPS receiver can be found at **/dev/rfcomm0**, you can dump its output using

.. code-block:: bash

   cat /dev/rfcomm0

The output should give you steady NMEA position updates. You can feed this information into **gpsd** / **xgps** or **QLandkarte** to see a graphical representation of your position and to check whether the output corresponds to your current position.

*******************
Basic Configuration
*******************

Currently Navit doesn't provide a graphical tool to change settings, so you have to do the changes manually using a texteditor.
Please open your current **navit.xml** file and do the following steps:


Setup proper startup-position
-----------------------------
On Navit's very first startup, it needs a **center** to look at on the map. By default this is set to Munich in Germany (at latitude 48.08 and longitude, which is conveniently covered by the sample map created on installation).

.. code-block:: xml

   <navit center="4808 N 1134 E" />

See furthermore: :doc:`general`


Setup GPS
---------
Add the GPS connection

.. code-block:: xml

   <vehicle name="My" enabled="yes" source="file://dev/ttyS0" active="1"/>

See furthermore: :doc:`vehicle`


Enable Map
----------
Just change the map entry corresponding to your local folders

.. code-block:: xml

  <map type="binfile" enabled="yes" data="/var/navit/maps/uk.bin" />

**********************
Advanced Configuration
**********************

After those first setup ther is probably a lot more you night wanne tinker with.
There are many more options, including debugging, specific plugins, speech announcements, trip logging, ...

See :doc:`advanced`

.. toctree::
   :glob:
   :maxdepth: 2
   :hidden:

   general
   display
   vehicle
   maps
   layout
   basic/*

