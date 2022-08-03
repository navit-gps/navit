.. _vehicleprofilecar_no_highway:

Vehicleprofile/Car no Highway
=============================


**Car (no Highway)**
====================

Drive your car like the standard "car" profile but not on highways, so
you haven't to pay for toll

XML
---

Copy and paste the following XML into navit.xml. Last update 01.01.13

.. code:: xml


               <vehicleprofile name="Car (no Highway)" flags="0x4000000" flags_forward_mask="0x4000002" flags_reverse_mask="0x4000001" maxspeed_handling="0" route_mode="0" static_speed="5" static_distance="25">
               <roadprofile item_types="street_0,street_1_city,living_street,street_service,track_gravelled,track_unpaved,street_parking_lane" speed="10" route_weight="10">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_2_city,track_paved" speed="30" route_weight="30">
                       <announcement level="0" distance_metric="50"/>
                       <announcement level="1" distance_metric="200"/>
                       <announcement level="2" distance_metric="500"/>
               </roadprofile>
               <roadprofile item_types="street_3_city" speed="40" route_weight="40">
                       <announcement level="0" distance_metric="50"/>
                       <announcement level="1" distance_metric="200"/>
                       <announcement level="2" distance_metric="500"/>
               </roadprofile>
               <roadprofile item_types="street_4_city" speed="50" route_weight="50">
                       <announcement level="0" distance_metric="50"/>
                       <announcement level="1" distance_metric="200"/>
                       <announcement level="2" distance_metric="500"/>
               </roadprofile>
               <roadprofile item_types="highway_city" speed="80" route_weight="80">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="street_1_land" speed="60" route_weight="60">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="street_2_land" speed="65" route_weight="65">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="street_3_land" speed="70" route_weight="70">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="street_4_land" speed="80" route_weight="80">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="street_n_lanes" speed="120" route_weight="120">
                       <announcement level="0" distance_metric="300"/>
                       <announcement level="1" distance_metric="1000"/>
                       <announcement level="2" distance_metric="2000"/>
               </roadprofile>
               <roadprofile item_types="ramp" speed="40" route_weight="40">
                       <announcement level="0" distance_metric="50"/>
                       <announcement level="1" distance_metric="200"/>
                       <announcement level="2" distance_metric="500"/>
               </roadprofile>
               <roadprofile item_types="roundabout" speed="10" route_weight="10"/>
               <roadprofile item_types="ferry" speed="40" route_weight="40"/>
           </vehicleprofile>

           
