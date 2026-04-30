.. _ustiger_maps:

US/Tiger maps
=============

.. raw:: mediawiki

   {{note
   |[[OpenStreetMap]] has finally finished importing USA TIGER data into their db. So now you can simply use it instead!
   }}

.. _outdated_for_historical_interest:

Outdated, For historical interest:
==================================

You can use the free Tiger maps for US.

Quick howto :

The conversion script converts tiger to osm v4 (where navit uses osm v5)
but hopefully there is a script which can convert osm v4 to osm v5...

So the procedure should be something like :

Get your datas from tiger. You can get the tiger to osm tools here :
http://sr71.net/~dave/osm/tiger/

``tiger-to-osm``

Now convert the resulting datas to osm v5 (you can get the tool here :
http://trac.openstreetmap.org/browser/applications/utils/conv05 )

``./04to05.pl mymap.osm``

Now, convert the datas to navit binfile :

``./osm2navit mymap.bin < mymap.osm``
