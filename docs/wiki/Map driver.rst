.. _map_driver:

Map driver
==========

.. _from_the_map_file_to_the_rendered_map_on_the_screen:

From the map file to the rendered map on the screen
===================================================

-  Screen size (rectangle) and zoom level as a user input determine what
   needs to get drawn on the screen.
-  These two values will be converted, according to the desired
   coordinates, into a map rectangle and an order value.
-  Coordinates, map rectangle and order value are the parameters for the
   map driver, which returns a handle.
-  With this handle you can get all the elements, which are found in the
   map rectangle.
-  These elements (coordinates and labels) will be saved in a hash.
-  Then the layout will be processed (see
   `navit.xml <http://navit.cvs.sourceforge.net/navit/navit/src/navit.xml?view=markup>`__),
   from top to bottom.
-  For each element in the layout the hash will be browsed and then
   passed, according to the layout, to the graphics driver, which draws
   the map onto the screen.

Terms
-----

-  **Elements** are defined in `Item_def.h <Item_def.h>`__. It is
   divided in 3 main categories:

   -  Point (e.g. a city center)
   -  Line (e.g. a street)
   -  Area/Polygon (e.g. a lake, a building, ..)

-  **Order value** tells the map driver roughly in which resolution the
   map needs to be displayed

   -  0 = entire globe .. 18 = maximum zoom level
   -  Example: street_nopass has the order=10 (>=10), that means for a
      lower resolution (<10) street_nopass wouldn't get drawn to the
      display, since it is not really relevant.

-  **Layout**

   -  The layout in navit.xml will be processed from top to bottom. This
      is necessary to make sure a street_nopass is always drawn on top
      of a poly_wood, regardless in which order the map driver delivers
      the data.

.. _the_api_point_of_view:

The API point of view
=====================

Usually a map driver is a `Plugin <Plugin>`__

.. code:: c

    plugin_register_map_type("your maptype", map_new_your_maptype);

    struct map_priv *map_new_your_maptype(struct map_methods *meth, struct attr **attrs)

meth are the methods to access the map (explained later) attrs are the
attributes which have been given to the map. For example, for

in navit.xml attr[0] would point to an attribute with ->type set to
attr_type and ->u.str with content "binfile", attr[1] would point to an
attribute with ->type set to attr_data and ->u.str with content
"osm_bbox_11.3,47.9,11.7,48.2.bin", attr[2] would be NULL indicating the
end of the list.

map_methods must be filled with a struct described in
`http://doxygen.navit-project.org/structmap__methods.html <http://doxygen.navit-project.org/structmap_methods.html>`__

The calling sequence is as following:

.. _plugin_initialisation:

Plugin Initialisation
---------------------

-  plugin_init is called when the plugin gets loaded

.. _map_configuration:

Map Configuration
-----------------

-  map_new_your_maptype is called when a map gets configured via an xml
   tag

.. _map_query:

Map Query
---------

-  map_rect_new is called when a map rectangle gets queried (actually it
   can be several rectangles...)
-  map_rect_get_item is called to get the first item (point, polyline,
   polygon) of the map rectangle

This function has to give back a pointer to an item with type set to the
type of the item, id_lo and id_hi set to something which allows the
driver to find that item again by that ids, meth to the methods to work
with the item, and priv_data to a driver private pointer to (quickly)
find the item. This pointer needs only be valid until map_rect_get_item
is called the next time or map_rect_destroy is called.

-  item_coord_get is called to get the coordinates of the item
-  item_attr_get is called to get the attributes of the item. The data
   needs only be valid until item_attr_get is called again,
   map_rect_get_item or map_rect destroy is called

Both calls are optional, if the requestor isn't interested in
coordinates or attributes, they are not called.

-  map_rect_get_item will be called again until it returns NULL (no more
   data available) or the requestor isn't interested in any more data
-  map_rect_destroy will be called to free any data allocated by
   map_rect_new,

.. _item_query:

Item Query
----------

The requestor can open an item by its id_hi and id_lo value to query
coordinates and or attributes

-  map_rect_new is called when a map rectangle which can be ignored
   (usually NULL)
-  map_rect_get_item_byid is called which should return an item struct
   just like map_rect_get_item
-  item_coord_get and/or item_attr_get just like Map Query
-  There might be more map_rect_get_item_byid calls
-  map_rect_destroy is called

.. _map_search:

Map Search
----------

-  map_search_new Creates a new map search
-  map_search_get_item Similar to map_rect_get_item
-  map_search_destroy to free any data allocated by the search

.. _raw_notes_about_iguidance:

Raw notes about iGuidance
=========================

-  It uses a format quite similar as ViaMichelin
-  Take a look at the hawaiian map, since it is the simplest, but the
   other maps have a similar format (but not exactly the same, the tile
   configuration seems to differ)

-  It has a 36 byte header (which is unknown for now)
-  Then there are 307 pointers to data.
-  The 307 probably consists of 256 + 32 + 16 + 2 + 1 tiles
-  Each pointer is either 0 (no data for this tile) or points to a
   position in the file where there are two integers: the compressed
   data size and the uncompressed data size

Following that are (uncompressed data size) bytes which are compressed
with a slightly modified zlib. Therefore the little program below will
work on viamichelin, but not on iguidance, since the crc verification
fails and it returns "Invalid Data" instead of the uncompressed length

Only difference is that it doesn't use crcs. So a good starting point
would be to write a little tool which extracts all tiles from a file and
take a look at the tile data.

Redirect stdin from your extracted data and stdout to a new file which
will contain the uncompressed data Then look at the uncompressed data
with an hex editor Usually the coordinates in this data are clearly
visible. Write out the coordinates into a textfile and load this
textfile into navit.

Then you can see the dots and guess what they mean. And then you
probably will find out what the other data in the tiles besides the
coordinates mean.

So for a first step you need to compile zlib and disable its crc check.
Then if you write a little program which decompresses all tiles we are
already a big step further.

Skip the data you don't understand for now. Maybe you will know later.

.. _raw_notes_about_viamichelin:

Raw notes about ViaMichelin
===========================

It has a quite similar format, except that the tile configuration is
different, they are using 16-Bit-Lenghts instead of 32-Bit-Pointers to
the data, and their compressed stream includes the crc. So basically,
it's as previously, excepted the following:

Look for the hex bytes "78 da". They usually indicate the start of a
zlib compressed stream.

Cut out an area of 64 kBytes after 78 da (including 78 da) Then run this
data through the following program:

.. code:: c

    #include <stdio.h>
    #include <zlib.h>
    char inb[65536];
    char outb[65536*10];
    int main()
    {
       long ins,outs=0;
       int ret;
       ins=read(0, inb, 65536);
       outs=65536*10;
       ret=uncompress(outb,&outs,inb,ins);
       fprintf(stderr,"outs=%d ret=%d\n", outs, ret);
       write(1, outb, outs);
       return 0;
    }

As previously, look at the uncompressed data with an hex editor. The
coordinates are usually quite easy to identify For a first test put each
coordinate into a textfile and load this textfile as map with the
textfile map driver Then take a look what the coordinates might mean
Skip the data you don't understand for now. Maybe you will know later
