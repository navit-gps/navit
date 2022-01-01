.. _vehicleprofilehike_bike_on_ground_gravel:

Vehicleprofile/Hike Bike on Ground Gravel
=========================================

.. _hike_bike_on_ground_grave:

**Hike & Bike on Ground & Grave**
=================================

You are a Biker or a Hiker? Navigate your tor with this profile throu
tracks, hikes, paths, mountainpaths and so on

XML
---

Copy and paste the following XML into navit.xml. Last update 01.01.13

.. code:: xml


      <vehicleprofile name="Hike & Bike on Ground & Gravel" flags="0x40000000" flags_forward_mask="0x40000000" flags_reverse_mask="0x40000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
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
               <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_0,street_1_city,living_street" speed="20" route_weight="8">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22" route_weight="8">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_4_city,ramp,street_3_land,street_4_land,street_3_city" speed="22" route_weight="7">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
         </roadprofile>
               </vehicleprofile>
               
