.. _mapserver_rewrite:

Mapserver rewrite
=================

.. raw:: mediawiki

   {{Warning| WIP }}

.. raw:: mediawiki

   {{Warning|This feature does not exist in the current Mapserver }}

APIs
----

.. _old_api_v1:

Old API (v1)
~~~~~~~~~~~~

Example:

::

   /api/map/?bbox=-180,-90,180,90&split=1048576000&part=0

.. _new_api_v2:

New API (v2)
~~~~~~~~~~~~

The new API is a HTTP-GET API (you can probably call it RESTfull)

All URLs which are supported:

+----------------------+----------------------+----------------------+
| URL                  | Description          | Example              |
+----------------------+----------------------+----------------------+
| /api/v2/areas        | Defaults to json     |                      |
|                      | output of all areas  |                      |
+----------------------+----------------------+----------------------+
| /api/v2/a            | Defaults to json     | /api/v2/areas/json   |
| reas/:responseformat | output of all areas  |                      |
+----------------------+----------------------+----------------------+
|                      | \|                   |                      |
+----------------------+----------------------+----------------------+
| /api/                |                      |                      |
| v2/download/bbox/:xm |                      |                      |
| in/:xmax/:ymin/:ymax |                      |                      |
+----------------------+----------------------+----------------------+
| /api/v2/downl        |                      |                      |
| oad/bbox/:xmin/:xmax |                      |                      |
| /:ymin/:ymax/:format |                      |                      |
+----------------------+----------------------+----------------------+
| /api/v2/d            |                      |                      |
| ownload/area/id/:id/ |                      |                      |
+----------------------+----------------------+----------------------+
| /api/v2/download     |                      |                      |
| /area/id/:id/:format |                      |                      |
+----------------------+----------------------+----------------------+
| /api/v2/download     |                      |                      |
| /area/name/:areaname |                      |                      |
+----------------------+----------------------+----------------------+
| /api                 |                      |                      |
| /v2/download/area/na |                      |                      |
| me/:areaname/:format |                      |                      |
+----------------------+----------------------+----------------------+
|                      | \|                   |                      |
+----------------------+----------------------+----------------------+
| /api/v               |                      | /api/v2/filesize/b   |
| 2/filesize/bbox/:xmi |                      | box/-180/180/-90/90/ |
| n/:xmax/:ymin/:ymax/ |                      |                      |
+----------------------+----------------------+----------------------+
| /api/v2/files        |                      | /api/v2/             |
| ize/bbox/:xmin/:xmax |                      | filesize/bbox/-180/1 |
| /:ymin/:ymax/:format |                      | 80/-90/90/planet.bin |
+----------------------+----------------------+----------------------+
| /api/v2/filesize     | Outputs the filesize |                      |
| /area/name/:areaname | of the downloadable  |                      |
|                      | area given by its    |                      |
|                      | area name. Defaults  |                      |
|                      | to json              |                      |
+----------------------+----------------------+----------------------+
| /api/v2/file         | Outputs the filesize |                      |
| size/area/name/:area | of the downloadable  |                      |
| name/:responseformat | area given by its    |                      |
|                      | area name in the     |                      |
|                      | supplied response    |                      |
|                      | format               |                      |
+----------------------+----------------------+----------------------+
| /api/v2/             | Outputs the filesize |                      |
| filesize/area/id/:id | of the downloadable  |                      |
|                      | area given by its    |                      |
|                      | id. Defaults to json |                      |
+----------------------+----------------------+----------------------+
| /api                 | Outputs the filesize |                      |
| /v2/filesize/area/id | of the downloadable  |                      |
| /:id/:responseformat | area given by its id |                      |
|                      | in the supplied      |                      |
|                      | response format      |                      |
+----------------------+----------------------+----------------------+
|                      |                      |                      |
+----------------------+----------------------+----------------------+

Parameter:

+-----------------+---------------------------------------------------+
| Parameter       | Value                                             |
+-----------------+---------------------------------------------------+
| :responseformat | json \| xml                                       |
+-----------------+---------------------------------------------------+
| :format         | bin \| bin2 \| a UTF-8 filename ending with       |
|                 | ".bin|.bin2"                                      |
+-----------------+---------------------------------------------------+
| :areaname       | The name of an area from the area list (names     |
|                 | should be static but can change so if a name does |
|                 | not work anymore look into the area list ->       |
|                 | /api/v2/areas)                                    |
+-----------------+---------------------------------------------------+
| :id             | The ID of an area from the area list. IDs will    |
|                 | change on every map update so make sure to look   |
|                 | them up before you download a map in order to get |
|                 | the newest available map.                         |
+-----------------+---------------------------------------------------+
| :xmin \| :xmax  | Any value between -180 and 180                    |
+-----------------+---------------------------------------------------+
| :ymin \| :ymax  | Any value between -90 and 90                      |
+-----------------+---------------------------------------------------+
