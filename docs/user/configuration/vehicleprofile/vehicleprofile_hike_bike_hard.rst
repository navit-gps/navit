.. _vehicleprofilehike_bike_hard:

Vehicleprofile/Hike Bike hard
=============================

**Hike & Bike hard**
====================

A Vehicleprofile designed for biker and hiker, who like to ride and hike
tracks, paths and other small ways.

XML
---

Copy and paste the following XML into navit.xml. Last update 01.01.13

.. code:: xml


           <vehicleprofile name="Hike & Bike hard" flags="0x80000000" flags_forward_mask="0x80000000" flags_reverse_mask="0x80000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
               <roadprofile item_types="steps" speed="2" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_pedestrian,footway" speed="5" route_weight="5">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="path,track_ground,hiking,track_grass,hiking_mountain" speed="12" route_weight="26">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="track_gravelled,track_unpaved" speed="17" route_weight="20">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22" route_weight="15">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_0,street_1_city,living_street" speed="20" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_3_city" speed="22" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_4_city,ramp" speed="22" route_weight="7">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_3_land,street_4_land" speed="20" route_weight="7">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="roundabout" speed="20" route_weight="10"/>
               <roadprofile item_types="ferry" speed="40" route_weight="10"/>
           </vehicleprofile>
           
