.. _vehicleprofilebike_on_asphalt:

Vehicleprofile/Bike on Asphalt
==============================

A Vehicleprofile designed for biker, who like to ride on Asphalt (racing
bicycle)

Features
--------

prefers streets with Asphalt surface, less graveled ways and no highways
where bikers aren't allowed

XML
---

Copy and paste the following XML into navit.xml. Last update 01.01.13

.. code:: xml


           <vehicleprofile name="Bike on Asphalt" flags="0x40000000" flags_forward_mask="0x40000000" flags_reverse_mask="0x40000000" maxspeed_handling="1" route_mode="0" static_speed="5" static_distance="25">
               <roadprofile item_types="steps" speed="2" route_weight="4">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_pedestrian,footway" speed="5" route_weight="11">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="path,track_ground,track_gravelled" speed="12" route_weight="5">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="track_paved,cycleway,street_service,street_parking_lane" speed="22" route_weight="20">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_0,street_1_city,living_street" speed="20" route_weight="20">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_2_city,street_1_land,street_2_land" speed="22" route_weight="15">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_3_city,street_4_city,ramp" speed="22" route_weight="20">
                       <announcement level="0" distance_metric="25"/>
                       <announcement level="1" distance_metric="100"/>
                       <announcement level="2" distance_metric="200"/>
               </roadprofile>
               <roadprofile item_types="street_3_land,street_4_land" speed="20" route_weight="15">
                       <announcement level="0" distance_metric="100"/>
                       <announcement level="1" distance_metric="400"/>
                       <announcement level="2" distance_metric="1000"/>
               </roadprofile>
               <roadprofile item_types="roundabout" speed="20" route_weight="18"/>
               <roadprofile item_types="ferry" speed="40" route_weight="15"/>
           </vehicleprofile>


