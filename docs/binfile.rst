Binfile
-------

Navit has its own map format, called **binfile** format. It's a binary format, optimized for use with Navit (rendering, search, routing on embedded devices).

Dividing the world into tiles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The map coordinates of binfile are in meters, measured from equator and null meridian, with a merkator projection. The limit of this world is about 20000 km in earch direction from this null point. This doesn't cover polar regions, but it's ok for now.

So, the world gives a 40000x40000km rectangle (tile). This rectangle is divided into 4 equally-sized sub-rectangles (tiles) called a,b,c and d counter clockwise ...

* a is top right
* b top left
* c bottom right
* d bottom left

Each of the sub-rectangles (tiles) is then further subdivided, so a rectangle "aa" is top right in "a"

This is continued further up to 14 levels (but the number of levels might be variable). So in the end you get many tiles which are containing objects which are within this tile... And the tiles are also containing references to tiles which are within them.

A Navit binfile is actually a ZIP file; each tile is a member file in a zip file. So to extract an area of this file, you just read the zip file sequentially.

If you extract a small binfile using a zip tool, you will see member files
adbdbbbbadcacd, adbdbbbbadcacd, ... . Each file contains the data for one tile.

Projection
~~~~~~~~~~

The coordinates are projected using a modified "Google" (i.e. EPSG900913) projection.
Navit assumes an earth radius of 6371000m rather than the WGS84 value of 6378137m. Thus, for navit, the circumference of the earth is (2 x 20015087m).

Tile data format
~~~~~~~~~~~~~~~~

A tile itself is a dynamic data structure.

As declared in `navit/map/binfile/binfile.c`__, a tile is represented in memory by a struct :

.. code-block:: c

 struct tile {
   int *start;           /* Memory address of the whole data structure */
   int *end;             /* Memory address of first memory address not belonging to tile
                          * thus tile->end - tile->start represents the size of the tile
                          * in multiples of 4 Bytes */
   int *pos;             /* current position inside the tile */
   int *pos_coord_start; /* pointer to the first element inside the tile that is a
                          * coordinate. That is the first position after the header of a
                          * tile. The header holds 3 entries each 32bit wide integers.
                          * header[0] holds the size of the whole data-structure.
                          * header[1] holds the type of the item
                          * header[2] holds the size of the coordinates in the tile */
   int *pos_coord;       /* current position inside the coordinates region within the tile */
   int *pos_attr_start;  /* pointer to the first attr //TODO explain attr format// data
                          * structure inside the tile's memory region */
   int *pos_attr;        /* current position inside the attr region */
   int *pos_next;        /* link to the next tile */
   int zipfile_num;
 }


Content
~~~~~~~
Inside the binfile, each tile file contains a list of items. Each item is stored like this (everything is 4 bytes wide and always aligned):

.. code-block:: c

   {
     int: Length of the item (not including this length field) in integers
     int: Type of the item (from item_def.h)
     int: Length of the coordinate data which follows in integers
     {
       int: pairs of coordinates with consisting of 2 integers each
     } 0..n
     {
       int: length of the attribute (not including this length field) in integers
       int: Type of attribute (from attr_def.h)
       {
         int: Attribute data, depending on attribute type
       } 0..n
     } 0..n
   }

Extracting a specific area
~~~~~~~~~~~~~~~~~~~~~~~~~~

You can calculate the bounding box of the current tile.

Then there are two possibilities:

* The tile overlaps with the area you are interested in: Then simply copy the whole file data, including its header to the output, and add an entry to the directory which will be written later
* The tile doesn't overlap: Then don't drop that file, but instead write a file with size 0 and the same name to the output (I will explain later why this is needed), and add an entry to the directory

At some point you will have reached the end of the zip files, then you have to write the zip directory and the "end of directory" marker.

This will be very fast (you don't have to look into the zip files, which would mean decompressing and compressing them again) but has some disadvantages:

* You will have many empty files in it which are not really necessary. This is needed because the reference to sub-tiles are by number, and not by name (would be slow), and so the position of a tile within the zip file is not allowed to change
* You get some data you didn't want to have: this is because a tile which overlaps with your area of course doesn't contain only data from your wanted area, but from the area where it is located


How an object is placed in a tile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An object is placed inside of a tile using the following approach

* If the object can fit into one of the 4 top tiles it is placed in that tile

* The 4 sub-tiles are then checked to see if the object will fit inside of any of the four tiles that are contained inside of the sub-tile.  If so, it is moved down a tile.  This step is repeated until the object spans 2 or more tiles (or the lowest tile level is reached)

* If the object can't fit inside of any of the 4 top sub-tiles it is placed inside of the top-most tile

An object 'fits' inside of a tile if the coordinates of the object (min lat, min lon, max lat, max lon) lie inside of the coordinates of the tile (tile_min_lat, tile_min_lon, tile_max_lat, tile_max_lon)

Any object that cross the equator or the poles is placed in the top-most tile because it can not fit inside of any sub-tile.

Some important objects are placed into upper level tiles despite of their length to be easier reachable for routing or display purposes. This is done by specifying maximum tile name length for them in item_order_by_type() function in `navit/maptool/misc.c`__.

BTW, "order" (zoom level) values used to query map and referred in <itemgra> and route_depth are equal to (tile_name_length-4).

.. __: https://github.com/navit-gps/navit/blob/trunk/navit/map/binfile/binfile.c
.. __: https://github.com/navit-gps/navit/blob/trunk/navit/maptool/misc.c
