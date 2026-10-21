Vehicleprofile
==============

For general information about vehicles and vehicleprofiles, see :doc:`vehicle options </user/configuration/basic/vehicle>`.

A vehicleprofile defines the behaviour of routing. Multiple vehicleprofiles can be defined in navit.xml and linked to vehicles via the ``profilename`` attribute. This allows switching routing behaviour (e.g. car vs bike vs pedestrian) from within Navit.

The following example vehicleprofiles are provided as starting points. Copy and adapt them to your needs.

.. _vehicleprofile_bike_cycleway:

Bike preferred Cycleways
------------------------

A vehicleprofile designed for cyclists who prefer to ride on cycleways.

Features
~~~~~~~~

Prefers cycleways, avoids graveled ways and highways where cyclists aren't allowed.

XML
~~~

.. code-block:: xml

   <vehicleprofile name="Bike preferred Cycleways" flags="0x80000000" flags_forward_mask="0x80000000" flags_reverse_mask="0x80000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
       <roadprofile item_types="steps" speed="2">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_pedestrian,footway" speed="18">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="path,track_ground,hiking,track_grass,hiking_mountain" speed="12">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_gravelled,track_unpaved" speed="17">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_paved,street_service,street_parking_lane" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="cycleway" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_0,street_1_city,living_street" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_2_city,street_1_land,street_2_land,street_3_city,street_4_city,ramp,street_3_land,street_4_land" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="roundabout" speed="20"/>
       <roadprofile item_types="ferry" speed="40"/>
   </vehicleprofile>

.. _vehicleprofile_bike_on_asphalt:

Bike on Asphalt
---------------

A vehicleprofile designed for racing bicycles that prefer asphalt surfaces.

Features
~~~~~~~~

Prefers streets with asphalt surface, avoids graveled ways and highways where cyclists aren't allowed.

XML
~~~

.. code-block:: xml

   <vehicleprofile name="Bike on Asphalt" flags="0x40000000" flags_forward_mask="0x40000000" flags_reverse_mask="0x40000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
       <roadprofile item_types="steps" speed="2">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_pedestrian,footway" speed="5">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="path,track_ground,track_gravelled" speed="12">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_0,street_1_city,living_street" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_3_city,street_4_city,ramp" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_3_land,street_4_land" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="roundabout" speed="20"/>
       <roadprofile item_types="ferry" speed="40"/>
   </vehicleprofile>

.. _vehicleprofile_hike_bike_on_ground_gravel:

Hike & Bike on Ground & Gravel
------------------------------

A vehicleprofile for bikers and hikers navigating tracks, paths, mountainpaths and similar ways.

Features
~~~~~~~~

Covers tracks, hikes, paths, mountainpaths and similar off-road ways.

XML
~~~

.. code-block:: xml

   <vehicleprofile name="Hike & Bike on Ground & Gravel" flags="0x40000000" flags_forward_mask="0x40000000" flags_reverse_mask="0x40000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
       <roadprofile item_types="steps" speed="2">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_pedestrian,footway" speed="5">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="path,track_ground,hiking,track_grass,hiking_mountain" speed="12">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_gravelled,track_unpaved" speed="17">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_0,street_1_city,living_street" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_4_city,ramp,street_3_land,street_4_land,street_3_city" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
   </vehicleprofile>

.. _vehicleprofile_hike_bike_hard:

Hike & Bike hard
----------------

A vehicleprofile for bikers and hikers who want to use more paths and smaller ways than the "Hike & Bike on Ground & Gravel" profile.

Features
~~~~~~~~

Includes more path types for adventurous routing.

XML
~~~

.. code-block:: xml

   <vehicleprofile name="Hike & Bike hard" flags="0x80000000" flags_forward_mask="0x80000000" flags_reverse_mask="0x80000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
       <roadprofile item_types="steps" speed="2">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_pedestrian,footway" speed="5">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="path,track_ground,hiking,track_grass,hiking_mountain" speed="12">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_gravelled,track_unpaved" speed="17">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_0,street_1_city,living_street" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_3_city" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_4_city,ramp" speed="22">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_3_land,street_4_land" speed="20">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="roundabout" speed="20"/>
       <roadprofile item_types="ferry" speed="40"/>
   </vehicleprofile>

.. _vehicleprofile_car_no_highway:

Car (no Highway)
----------------

A vehicleprofile for car routing that avoids highways (e.g. to avoid toll roads).

Features
~~~~~~~~

Routes like the standard "car" profile but avoids highways.

XML
~~~

.. code-block:: xml

   <vehicleprofile name="Car (no Highway)" flags="0x4000000" flags_forward_mask="0x4000002" flags_reverse_mask="0x4000001" maxspeed_handling="0" route_mode="0" static_speed="5" static_distance="25">
       <roadprofile item_types="street_0,street_1_city,living_street,street_service,track_gravelled,track_unpaved,street_parking_lane" speed="10">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_2_city,track_paved" speed="30">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_3_city" speed="40">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="street_4_city" speed="50">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="highway_city" speed="80">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="street_1_land" speed="60">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="street_2_land" speed="65">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="street_3_land" speed="70">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="street_4_land" speed="80">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2" distance_metric="1000"/>
       </roadprofile>
       <roadprofile item_types="street_n_lanes" speed="120">
           <announcement level="0"/>
           <announcement level="1" distance_metric="1000"/>
           <announcement level="2" distance_metric="2000"/>
       </roadprofile>
       <roadprofile item_types="ramp" speed="40">
           <announcement level="0"/>
           <announcement level="1"/>
           <announcement level="2"/>
       </roadprofile>
       <roadprofile item_types="roundabout" speed="10"/>
       <roadprofile item_types="ferry" speed="40"/>
   </vehicleprofile>

Creating custom vehicleprofiles
-------------------------------

See :doc:`basic/vehicle </user/configuration/basic/vehicle>` for a full reference of vehicleprofile attributes (flags, route_depth, route_mode, static_speed, static_distance) and roadprofile attributes (item_types, speed, maxspeed), including the complete :ref:`vehicle_profile_flags` table.
