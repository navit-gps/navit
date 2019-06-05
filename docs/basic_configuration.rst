There are much more options, see [[configuration]].

Preparation
===========
Get a supported map package
---------------------------
Navit can use different :doc:`maps` formats, including free :ref:`OpenStreetMap` data. In order to use one of these maps, download a map of your desired area and store it into a local folder (such as navit/maps or /usr/share/navit/maps).

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

Configuring
===========

Currently Navit doesn't provide a graphical tool to change settings, so you have to do the changes manually using a texteditor.
Please open your current **navit.xml** file and do the following steps:


Setup proper startup-position
-----------------------------
On Navit's very first startup, it needs a **center** to look at on the map. By default this is set to Munich in Germany (at latitude 48.08 and longitude, which is conveniently covered by the sample map created on installation).

.. code-block:: xml

   <navit center="4808 N 1134 E" />

See furthermore: [[Configuration]]


Setup GPS
---------
Add the GPS connection

.. code-block:: xml

   <vehicle name="My" enabled="yes" source="file://dev/ttyS0"/ active="1"/>

See furthermore: [[Configuration]]


Enable Map
----------
Just change the map entry corresponding to your local folders

.. code-block:: xml

  <map type="binfile" enabled="yes" data="/var/navit/maps/uk.bin" />
