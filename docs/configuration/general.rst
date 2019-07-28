==General Options==
Settings which affect Navit's general behaviour are located within the ``<navit .. >`` tag. In a default installation, this is found on line 31 of ``navit.xml``. By default, the ``navit`` tag is set as follows:

.. code-block:: xml

<source lang="xml">
 <navit center="4808 N 1134 E" zoom="256" tracking="1" orientation="-1" recent_dest="10">
</source>

Some common attributes are discussed below. For more advanced features, see [[Configuration/Full_list_of_options | the full list of options]].

===Initial map position===
On Navit's very first startup, it needs a **center** to look at on the map. By default this is set to Munich in Germany, which is conveniently covered by the sample map created on installation.


.. code-block:: xml

<source lang="xml">
 center="11.5666 48.1333"
</source>

Coordinates can be written in different formats; see [[Coordinate_format]] for the full list.
To determine a specific latitude and longitude for your location you can use http://itouchmap.com/latlong.html.
Usually, changing the "center" setting is not necessary, since it is only used during the first start.
On subsequent starts, Navit will remember the last map position (stored in "center.txt") and ignore the "center" setting.

When Navit starts, it will display the map at a pre-defined **zoom**. The default zoom level is 256. The lower the value, the closer you will be zoomed in. <!---what does the zoom number express? How can I estimate it?-->

 zoom="256"

For those using the [[SDL GUI]], a level of 128 is recommended.

 zoom="128"

Note that once Navit has started, the zoom level can be altered using [[OSD]] or menu items.


Use the **orientation** attribute to orient the map in either the direction of travel, or oriented North. .To orient the map in the direction of travel:

 orientation="-1"

or to orient North:

 orientation="0"

Orienting the map North whilst in [[3D]] mode will provide visually confusing results, and is not recommended. When in 3D mode, it's best to have the map oriented in the direction of travel.

===Autozoom===
Navit has the ability to **autozoom** the map in or out dependent upon your speed.

 autozoom_active="1"

To de-activate autozoom:

 autozoom_active="0"

===3D pitch===
Navit has the capability to display either a 2D map (bird's eye perspective) or a [[3D]] map (some amount of tilt looking to the horizon). Navit's default configuration is to startup in the 2D perspective but it is possible to specify that Navit start with a 3D perspective. The amount of tilt is specified by setting the value of <tt>pitch</tt>.

The **pitch** value defines default camera tilting, with a value from 0 to 359. Note that usable values lie between 0 and 90 where 0 is bird's eye perspective looking down and 90 is human perspective looking forward. Also note that values closer to 90 will slow down map drawing, because the line of sight gets longer and longer and more objects are seen.

For example, the following added to the <tt>navit</tt> tag will force Navit to start with a pitch of 30 degrees:

 pitch="30"

===Imperial units===
By default, Navit use the metric system of measurements when displaying or announcing distances, speeds etc. However, you can configure Navit to display and announce these values in imperial units. Simply add an <tt>imperial</tt> attribute to the Navit tag, and set its value to 1, as shown below:

 imperial="1"

Speeds should now be displayed in units of miles-per-hour, whilst distances are converted to miles (large distances) and feet (small distances).

===Default layout===
When no specific layout has been specified by the user, navit uses a default layout to draw maps.
The  <tt>default_layout</tt> attribute of the navit tag allows to specify which layout to use as default;

 default_layout="Car"

This string should match the <tt>name</tt> attribute of the required <tt><layout></tt> tag.

See [[Configuration/Layout_Options| layout options]] for more details.
[[Category:Customizing]]
[[Category:Configuration]]
