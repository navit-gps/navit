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

For infos about map icons, see [[Icons]]

Overriding default (shipped) layouts
------------------------------------
When the XML config file is parsed, layouts are taken in the order they come, and a layout with an already existing name overrides a previous definition.
The default (shipped) navit.xml includes first system-wide navit_layout_*.xml files then the user-specific navit_layout_*.xml files, so the system-wide navit_layout_*.xml files can be overiden by adding a user-specific navit-layout_*.xml containing the same **name** attribute.

Copying the default **navit.xml** file to the user-specific location will still use the default shipped layout files, but copying one or several layout files as well to the user-specific location (and modifying them) allow to override these specific layouts.

Note on the default layout used by navit
----------------------------------------
When no layout has been specifically selected by the user (for example at first startup), navit will use the default layout specified (see [[Configuration/General_Options#Default_layout|the related section to know how to configure this]]).

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

[[Category:Customizing]]
[[Category:Configuration]]
