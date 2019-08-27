==Vehicle Options==
It's important to understand the separate but linked Navit concepts of a **vehicle** and [[Vehicleprofile| **vehicleprofile**]] element. A vehicle defines the source of positional data (suchas a USB GPS device), and how to present that data to the user on the map, where the vehicleprofile defines all aspects of routing.

A simple vehicle definition looks like this:


.. code-block:: xml

<source lang="xml">
  <vehicle name="My" enabled="yes" source="file://dev/ttyS0"/ active="1"/>
</source>


Here some of the available options:
* **active**: If set to 1, makes the vehicle the default active one. Routing, view centering and map redraw would be applied to this one by default.
* **enabled**: If set to yes, Navit connects to the vehicle data source and shows the vehicle on the map.
* **follow**: map follows after "n" gps updates (where n=0 means only when the vehicle leaves the map->saving CPU time)
* **source** : source of GPS (required)
* **profilename**: link the vehicleprofile for this vehicle.

<!-- do we need to keep this keys or note them elsewhere?
Obsolete options:
* **update**: This will force the map to be recentered at your main cursor's position.
* **color**/**color2**: The color of the cursor is now specified within the cursor tag itself.
* **animate**: If set to 1, the cursor will be animated as a moving dotted line, to make it easier to see.
-->

===GPS===
Most essential detail is the gps source, that need to be adapted to your local setup.


**Local:**

Windows:
* source="serial:COM2 baud=4800 parity=N data=8 stop=1"
** For Locosys GT31 (or BGT31 plugged by USB), settings are: source="serial:COM3 baud=38400 parity=N data=8 stop=1" where COM3 should be replaced by the correct COM over USB number.

Windows Mobile:
* source="wince:GPD1:" - using the internal GPS driver, configured from Windows Remote GPS Settings.
** To use a BT GPS it must be configured as outgoing com port and paired, then selected as hardware port in GPS Settings.
** To start bluetooth on navit startup add bluetooth="yes". When exiting navit, the previous bluetooth state is recovered.
* source="wince:COM1:" baudrate="57600"

There is useful [http://w5.nuinternet.com/s660100031/SirfTech.htm. SirfTech utility] that can  automatically scan existing serial ports trying different baudrates to detect GPS source.

Linux:
* source="file:/dev/rfcomm0"	- BlueTooth GPS
* source="file:/dev/ttyS0"	- serial GPS connected to the first serial port (you may need to add the correct baudrate eg.: freerunner source="file:/dev/ttySAC1" baudrate="9600")
* source="gpsd_dbus:"           - via dbus, position reported by gpsd

**Network based:**<br>
If you want to connect multiple tools to your GPS, you need an multiplexer tool, as gpsd or gypsy.
* source="gpsd://host[:port]"	 - gpsd://localhost, the default one, will try to connect to gpsd on localhost
* source="socket:ipaddr:post"    - socket connection (expects nmea stream)
* source="socket:ipaddr:2947:r=1" - connect to gpsd in nmea mode (gpsd versions 2.39 or older)
* source='socket:ipaddr:2947:?WATCH={"enable":true,"nmea":true};' - connect to gpsd in nmea mode (gpsd versions newer than 2.39)
* source="gypsy://connectstring" - gypsy

**Tricks:**
* source="file:/home/myhome/mynmea.log" : here, navit will replay the nmea logfile (under Windows it is currently not possible in Navit)
* source="pipe:/usr/bin/gpspipe -r" - any executable that produces NMEA output - gpsbabel, gpspipe, ...
* source="demo://" : to use the demo vehicle. Set your Position and Destination, and vehicle will follow the calculated route. Useful if you have no nmea data source.
* source="null://" : no GPS at all

===Logging tracks===
To record your trip , you can add a sub-instance "log" to the vehicle. It is possible to add multiple logs.


.. code-block:: xml

<source lang="xml">
 <log type="gpx" data="track_%Y%m%d%i.gpx" flush_size="1048576" flush_time="900" />
 <log type="nmea" data="track_%Y%m%d%i.nmea" flush_size="1048576" flush_time="900" />
</source>


This will give a log file named YearMonthDaySequencenumber.gpx/.nmea which will be kept in memory and flushed to disk when it is 1048576 bytes large or the oldest data is older than 900 seconds.
<!--how to define, where the file get's stored?-->

To display your track for more than one hour, you must use [[binfile]] to create a cache file that get's display <!--do I need to add it as a map source?-->

.. code-block:: xml

<source lang="xml">
 <log type="binfile" data="track.bin" flush_size="0"/>
</source>

For customizing what is stored, see [[GPX]]

===Vehicleprofile===
[[Vehicleprofile | Profiles to add in the navit.xml]]
<br/>
<!-- this is tricky, here we need a step by step introduction-->
Defines the behaviour of the routing and are usually linked to a vehicle section, so switching the "vehicle" (type of mobility) from within Navit, routing also will change its behaviour. This way, it is possible to include steps for pedestrian routing, but to exclude it for bike, horse or car routing. Within the vehicleprofile section, roadprofile sections are used to describe the routing behaviour of different roads. Here's a very basic example:


.. code-block:: xml

<source lang="xml">
<vehicleprofile name="bike" flags="0x40000000" flags_forward_mask="0x40000000" flags_reverse_mask="0x40000000" maxspeed_handling="1" route_mode="0">
  <roadprofile item_types="path,track_ground" speed="12" route_weight="5">
  </roadprofile>
  <roadprofile item_types="track_gravelled,track_paved,cycleway,street_service,street_parking_lane,street_0,street_1_city,living_street,street_2_city,street_1_land,street_2_land,street_3_city" speed="25" route_weight="15">
  </roadprofile>
  <roadprofile item_types="roundabout" speed="20" route_weight="10"/>
  <roadprofile item_types="ferry" speed="40" route_weight="40"/>
 </vehicleprofile>
</source>

For details on the flags, see [[Vehicle profile flags]].
The speeds are in km/h.

Only the vehicle profile names "car", "bike" and "pedestrian" are translated in the GUI.


[[Category:Customizing]]
[[Category:Configuration]]
