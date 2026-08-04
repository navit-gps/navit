.. configuration_layout:

Layout Options
==============
Layouts
-------
A layout defines how to **render a map**.  Layouts are fully customisable, from the road colours and widths to size and type of icons to display for specific POIs. The layout is also where the **cursor** (i.e. the shape which shows where you are) is defined.<br>
A number of user-generated layouts and cursor definitions are available at [[Layout]].

Defining layers
---------------
A layout consist of one cursor and one or more **layers** which are each a set of rules on how and when to draw certain items. Those rules are called **itemgra**. The layers are rendered one by one in the order they appear in the XML config file, as are the items in each layer. If you can't see an item make sure there is not another one hiding it. If your item is hidden, you can move your item further down in the layout section of the file.


.. code-block:: xml

 <layout name="Demo layout" color="#ffefb7" font="Liberation Sans" active="1">
	<cursor w="26" h="26">
	<layer name="layer_1">
		<itemgra item_types="water_poly" order="0-">
			<polygon color="#82c8ea" />
		 	<polyline color="#5096b8" />
		 </itemgra>
	</layer>
 <layout>

Here the available options:

* item_types: Comma separated list of items (see navit/item.h for definitions)
* order: Range for zoom levels.
* speed_range: Range for vehicle speed, useful for cursors.
* angle_range: Range for pitch angle.
* sequence_range: Useful for animated cursors.

Order ranges
^^^^^^^^^^^^
A range for ``order`` is defined as ``lower_bound-upper_bound``. Possible
values for a bound are between 0 and 32767, but not all of them make sense.
``-5`` is a synonym for ``0-5``, as is ``17`` for ``17-17``. ``0-`` is a
synonym for ``0-32767`` and means to always draw the item, i.e. to not
apply this range as a limiting factor; this is the default for range
options that are not specified. ``10-`` is good for items visible at a zoom
level showing an entire city, ``17-`` for items showing when zoomed onto a
block. Meaningful values for ``order`` are between 0 and 18.

An item type can be defined multiple times, for example to give it a
different width or color depending on the zoom level:

.. code-block:: xml

  <itemgra item_types="town" order="0-17">
      <polygon color="#e0e0e0" />
  </itemgra>
  <itemgra item_types="town" order="18-">
      <polygon color="#c0c0c0" />
  </itemgra>

The poly\ **gon** color defines the color with which the polygon is filled;
it only applies to polygons such as water, towns or woods. The poly\ **line**
color defines the color with which lines are drawn. If the item is a line,
such as a street, it is its color; if the item is a polygon, it is its
border color.

.. _layout_icons:

Icons
~~~~~
POI and map icons are provided with the source tree in the ``navit/xpm/``
directory (installed usually to ``/usr/share/navit/xpm/``). Originally they
were 16x16 .xpm bitmaps; they are now .svg files that are converted to
.png bitmaps at build time.

Icons are drawn in an ``itemgra`` with an ``icon`` element. The ``src``
attribute is required; ``w`` and ``h`` limit the size, ``x`` and ``y``
offset the icon within the item, and ``rotation`` rotates it:

.. code-block:: xml

  <itemgra item_types="poi_fuel" order="12-">
      <icon src="fuel.png"/>
  </itemgra>

Navit does not scale map icons automatically, so it is up to the style to
use different sizes at different zoom levels.

Overriding default (shipped) layouts
------------------------------------
When the XML config file is parsed, layouts are taken in the order they come, and a layout with an already existing name overrides a previous definition.
The default (shipped) navit.xml includes first system-wide navit_layout_*.xml files then the user-specific navit_layout_*.xml files, so the system-wide navit_layout_*.xml files can be overiden by adding a user-specific navit-layout_*.xml containing the same **name** attribute.

Copying the default **navit.xml** file to the user-specific location will still use the default shipped layout files, but copying one or several layout files as well to the user-specific location (and modifying them) allow to override these specific layouts.

Note on the default layout used by navit
----------------------------------------
When no layout has been specifically selected by the user (for example at first startup), navit will use the default layout specified (see [[Configuration/General_Options#Default_layout|the related section to know how to configure this]]).

Alternatively, a layout tag can carry an ``active="1"`` attribute;
irrespective of where the layout appears in the XML file, that layout
becomes the default. ``default_layout`` is the preferred way to select a
default layout.

Day and night layouts
---------------------
A layout can reference a daytime and a nighttime counterpart via the
``daylayout`` and ``nightlayout`` attributes, each holding the name of
another layout. Navit then automatically switches between the two based
on the actual sunrise and sunset at the current position, so the behavior
is identical in both hemispheres.

To control the switch manually, use the ``switch_layout_day_night``
command (see [[OSD]]) with ``"manual"``, ``"auto"``, ``"manual_toggle"``,
``"manual_day"`` or ``"manual_night"``.

Setting ``tunnel_nightlayout="1"`` on the ``<navit>`` tag additionally
switches to the night layout while driving through a tunnel.

Using a layer in multiple layouts
---------------------------------
Sometimes, multiple layouts can use the same layer. For example, a reduced layout for a cleaner map may use the same layers as the regular layout, just not all of them.

To use a layer in multiple layouts, it can be referenced using the **ref** attribute. In place of the regular layer definition, use an empty tag with only the attributes ''name'' and ''ref'':

.. code-block:: xml

  <layer name="Found items" order="0-">
    <itemgra item_types="found_item">
        <circle color="#008080" radius="24" width="2" text_size="12"/>
    </itemgra>
  </layer>
  [...]
  <layout name="Demo layout">
    [...]
    <layer name="Found items for demo layout" ref="Found items" />
    [...]
  </layout>
  <layout name="Demo layout reduced">
    [...]
    <layer name="Found items" ref="Found items" />
    [...]
  </layout>

Note that the layer you want to reuse must be placed ''outside'' the layout. Layers defined inside a layout cannot be reused in this way.

Text size
---------
The size of labels on the map (street names, town names and just about
everything else with a label) is defined per element via a ``text_size``
attribute:

.. code-block:: xml

  <itemgra item_types="street_city">
      <text text_size="14" text_color="#000000" />
  </itemgra>

At the moment there is no relative text size adjustment, so each tag has to
be adjusted manually.

Fonts
-----
The default font used for text on the map is defined by the ``font``
attribute of the ``layout`` tag. It is important to pick a font that your
OS actually supports; on Linux systems ``fc-list`` lists the available
fonts. When specifying a font, ignore the ``style=`` part and use just the
font name.

