.. _layoutmapnik_for_hdpi:

Layout/mapnik-for-hdpi
======================

.. _mapnikopenstreetmap_style_layout:

Mapnik/OpenStreetMap Style Layout
=================================

Based upon the `Mapnik <Layout/mapnik>`__ layout, this layout improves
the map for high definition small screens, such as those for smartphones
and netbooks.

.. _see_also:

See also
--------

A Mapnik-based layout optimised for medium definition `small screens is
also available <Layout/mapnik-for-mdpi>`__.

XML
---

.. code:: xml

   <layout name="android-hdpi" nightlayout="android-dark" color="#F8F8F8" font="Arial">

       <cursor w="90" h="90">
           <itemgra>
                 <circle color="#0000ff" radius="72" width="6">
                           <coord x="0" y="0"/>
                       </circle>
                   </itemgra>
                   <itemgra speed_range="-2">
                       <polyline color="#0000ff" width="6">
                           <coord x="0" y="0"/>
                           <coord x="0" y="0"/>
                       </polyline>
                   </itemgra>
                   <itemgra speed_range="3-">
                       <polyline color="#0000ff" width="6">
                           <coord x="-7" y="-10"/>
                           <coord x="0" y="12"/>
                           <coord x="7" y="-10"/>
                       </polyline>
               </itemgra>
       </cursor>
           
               <layer name="polygons">
                   <itemgra item_types="image" order="0-">
                       <image/>
                   </itemgra>
                   <itemgra item_types="poly_farm" order="12-">
                       <polygon color="#EADBC4"/>
                       <polyline color="#CFC1AB"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_town" order="0-">
                       <polygon color="#DCDCDC"/>
                       <polyline color="#C0BEBE"/>
                   </itemgra>
                   <itemgra item_types="poly_university" order="12-">
                       <polygon color="#F0F0D8"/>
                       <polyline color="#BFA990"/>
                   </itemgra>
                   <itemgra item_types="poly_nature_reserve" order="12-">
                       <polygon color="#ABDE96"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_reservoir" order="12-">
                       <polygon color="#B5D0D0"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_basin" order="12-">
                       <polygon color="#B5D0D0"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_marina" order="10-">
                       <polygon color="#B5D0D0"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_land" order="12-">
                       <polygon color="#F0D9D9"/>
                       <polyline color="#F2EEE8"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
           <itemgra item_types="poly_commercial" order="12-">
                       <polygon color="#EEC8C8"/>
                       <polyline color="#FFFFC1"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_retail" order="0-">
                       <polygon color="#F0D9D9"/>
                       <polyline color="#EC989A"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_recreation_ground" order="12-">
                       <polygon color="#CEEBA8"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_construction" order="10-">
                       <polygon color="#9D9C6B"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_quarry" order="0-">
                       <polygon color="#C5C4C3"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_allotments" order="12-">
                       <polygon color="#C8B084"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_common" order="10-">
                       <polygon color="#CEEBA7"/>
                       <text text_size="5"/>
                   </itemgra>
                   <itemgra item_types="poly_park" order="12-">
                       <polygon color="#B5FCB5"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_village_green" order="12-">
                       <polygon color="#CEEBA8"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                       <itemgra item_types="poly_heath" order="0-">
                       <polygon color="#FEFEC0"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                     <itemgra item_types="poly_golf_course" order="12-">
                       <polygon color="#B4E2B4"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_wood" order="9-">
                       <polygon color="#ADD1A0"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="poly_playground" order="12-">
                       <polygon color="#B5FCB5"/>
               <polyline color="#8AD2AE" width="5"/>
                   </itemgra>
                   <!--<itemgra item_types="poly_pedestrian" order="10">
                       <polyline color="#9A9889" width="3"/>
                       <polyline color="#E5E0C2" width="1"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="11">
                       <polyline color="#9A9889" width="5"/>
                       <polyline color="#E5E0C2" width="3"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>-->
                   <itemgra item_types="poly_pedestrian" order="12">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#E5E0C2" width="6"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="13">
                       <polyline color="#9A9889" width="9"/>
                       <polyline color="#E5E0C2" width="7"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="14">
                       <polyline color="#9A9889" width="13"/>
                       <polyline color="#E5E0C2" width="9"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="15">
                       <polyline color="#9A9889" width="18"/>
                       <polyline color="#E5E0C2" width="14"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="16">
                       <polyline color="#9A9889" width="21"/>
                       <polyline color="#E5E0C2" width="17"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="17">
                       <polyline color="#9A9889" width="25"/>
                       <polyline color="#E5E0C2" width="21"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_pedestrian" order="18">
                       <polyline color="#9A9889" width="40"/>
                       <polyline color="#E5E0C2" width="34"/>
                       <polygon color="#EDEDED"/>
                   </itemgra>
                   <itemgra item_types="poly_airport" order="4-">
                       <polygon color="#a0a0a0"/>
                   </itemgra>
           <itemgra item_types="poly_military" order="4-">
               <polygon color="#F6D1CE"/>
                       <text text_size="5"/>
                   </itemgra>
           <itemgra item_types="poly_danger_area" order="0-">
                       <polygon color="#FCD8DB" />
                       <polyline color="#BD6B71" width="1"/>
               <text text_size="5"/>
                   </itemgra>
           <itemgra item_types="poly_railway" order="11-">
               <polygon color="#DED0D5"/>
                   </itemgra>
           <itemgra item_types="poly_barracks" order="12-">
               <polygon color="#FE8E8E"/>
                       <text text_size="5"/>
                   </itemgra>
                   <itemgra item_types="poly_sport,poly_sports_pitch" order="12-">
                       <polygon color="#B5FCB5"/>
               <polyline color="#8AD2AE" width="5"/>
                   </itemgra>
                   <itemgra item_types="poly_industry,poly_place" order="12-">
                       <polygon color="#DED0D5"/>
                   </itemgra>
                   <itemgra item_types="poly_service" order="8-18">
                       <polygon color="#fefefe"/>
                       <polyline color="#9A9889" width="1"/>
                   </itemgra>
                   
                   <itemgra item_types="poly_street_1" order="8-13">
                       <polygon color="#ffffff"/>
                       <polyline color="#9A9889" width="1"/>
                   </itemgra>
                   <itemgra item_types="poly_street_1" order="14-16">
                       <polygon color="#ffffff"/>
                       <polyline color="#9A9889" width="2"/>
                   </itemgra>
                   <itemgra item_types="poly_street_1" order="17-18">
                       <polygon color="#ffffff"/>
                       <polyline color="#9A9889" width="3"/>
                   </itemgra>
                   <itemgra item_types="poly_street_2" order="7-12">
                       <polygon color="#ffff00"/>
                       <polyline color="#c0c0c0" width="1"/>
                   </itemgra>
                   <itemgra item_types="poly_street_2" order="13-16">
                       <polygon color="#ffff00"/>
                       <polyline color="#c0c0c0" width="2"/>
                   </itemgra>
                   <itemgra item_types="poly_street_2" order="17-18">
                       <polygon color="#ffff00"/>
                       <polyline color="#c0c0c0" width="3"/>
                   </itemgra>
                   <itemgra item_types="poly_street_3" order="7-11">
                       <polygon color="#FDBF70"/>
                       <polyline color="#FDBF70" width="1"/>
                   </itemgra>
                   <itemgra item_types="poly_street_3" order="12-15">
                       <polygon color="#FDBF70"/>
                       <polyline color="#FDBF70" width="2"/>
                   </itemgra>
                   <itemgra item_types="poly_street_3" order="16-18">
                       <polygon color="#FDBF70"/>
                       <polyline color="#FDBF70" width="3"/>
                   </itemgra>
                   <itemgra item_types="poly_apron" order="8-">
                       <polygon color="#F0E0FE"/>
                   </itemgra>
                   <itemgra item_types="poly_terminal" order="7-">
                       <polygon color="#CB99FE"/>
                   </itemgra>
                   <itemgra item_types="poly_cemetery" order="12-">
                       <polygon color="#ADD0A0"/>
                   </itemgra>
                   <itemgra item_types="poly_car_parking" order="12-">
                       <polygon color="#F6EEB7"/>
                       <polyline color="#F6EEB7"/>
                   </itemgra>
                   <itemgra item_types="poly_building" order="15-">
                       <polygon color="#BCA9A9"/>
                       <polyline color="#BCA9A9" width="2"/> 
                   </itemgra>
                   <itemgra item_types="rail" order="9-11">
                       <polyline color="#000000" width="3"/>
                       <polyline color="#777777" width="1" dash="5,5"/>
                   </itemgra>
                   <itemgra item_types="rail" order="12-13">
                       <polyline color="#000000" width="5"/>
                       <polyline color="#777777" width="3" dash="5,5"/>
                   </itemgra>
                   <itemgra item_types="rail" order="14-15">
                       <polyline color="#000000" width="8"/>
                       <polyline color="#777777" width="5" dash="5,5"/>
                   </itemgra>
                   <itemgra item_types="rail" order="16-">
                       <polyline color="#000000" width="12"/>
                       <polyline color="#777777" width="8" dash="5,5"/>
                   </itemgra>
                   <itemgra item_types="ferry" order="10-">
                       <polyline color="#000000" width="1" dash="10"/>
                   </itemgra>
                  
                   <itemgra item_types="border_state" order="0-">
                       <polyline color="#808080" width="1"/>
                   </itemgra>
                   <itemgra item_types="height_line_1" order="0-">
                       <polyline color="#000000" width="4"/>
                   </itemgra>
                   <itemgra item_types="height_line_2" order="0-">
                       <polyline color="#000000" width="2"/>
                   </itemgra>
                   <itemgra item_types="height_line_3" order="0-">
                       <polyline color="#000000" width="1"/>
                   </itemgra>
                   <itemgra item_types="poly_water" order="3-14">
                       <polygon color="#B5D0D0"/>
                   </itemgra>
                   <itemgra item_types="poly_water" order="15-">
                       <polygon color="#B5D0D0"/>
                       <!--<text text_size="16"/>-->
                   </itemgra>
                   <itemgra item_types="water_line" order="0-">
                       <polyline color="#5096b8" width="1"/>
                       <!--<text text_size="5"/>-->
                   </itemgra>
                   <itemgra item_types="water_river" order="4-6">
                       <polyline color="#B5D0D0" width="3"/>
                   </itemgra>
                   <itemgra item_types="water_river" order="7-10">
                       <polyline color="#B5D0D0" width="6"/>
                   </itemgra>
                   <itemgra item_types="water_river" order="10-12">
                       <polyline color="#B5D0D0" width="12"/>
                       <text text_size="12"/>
                   </itemgra>
                   <itemgra item_types="water_river" order="12-">
                       <polyline color="#B5D0D0" width="18"/>
                       <text text_size="18"/>
                   </itemgra>
                   <itemgra item_types="water_river" order="13-">
                       <polyline color="#B5D0D0" width="24"/>
                       <text text_size="24"/>
                   </itemgra>
                   <itemgra item_types="water_canal" order="6">
                       <polyline color="#B5D0D0" width="1"/>
                   </itemgra>
                   <itemgra item_types="water_canal" order="7">
                       <polyline color="#B5D0D0" width="2"/>
                       <text text_size="5"/>
                   </itemgra>
                   <itemgra item_types="water_canal" order="8-9">
                       <polyline color="#B5D0D0" width="3"/>
                       <text text_size="7"/>
                   </itemgra>
                   <itemgra item_types="water_canal" order="10-">
                       <polyline color="#B5D0D0" width="3"/>
                       <text text_size="10"/>
                   </itemgra>
                   <itemgra item_types="water_stream" order="8-9">
                       <polyline color="#B5D0D0" width="2"/>
                   </itemgra>
                   <itemgra item_types="water_stream" order="10-11">
                       <polyline color="#B5D0D0" width="4"/>
                   </itemgra>
                   <itemgra item_types="water_stream" order="12-13">
                       <polyline color="#B5D0D0" width="8"/>
                       <text text_size="12"/>
                   </itemgra>
                   <itemgra item_types="water_stream" order="14-">
                       <polyline color="#B5D0D0" width="16"/>
                       <text text_size="12"/>
                   </itemgra>
                   <itemgra item_types="water_drain" order="12-13">
                       <polyline color="#B5D0D0" width="20"/>
                       <text text_size="18"/>
                   </itemgra>
                   <itemgra item_types="water_drain" order="14-">
                       <polyline color="#B5D0D0" width="30"/>
                       <text text_size="24"/>
                   </itemgra>
               </layer>
   <layer name="streets">
           <itemgra item_types="selected_line" order="2">
                       <polyline color="#ba00b8" width="8"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="3-5">
                       <polyline color="#ba00b8" width="16"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="6">
                       <polyline color="#ba00b8" width="20"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="7-8">
                       <polyline color="#ba00b8" width="32"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="9-10">
                       <polyline color="#ba00b8" width="40"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="11">
                       <polyline color="#ba00b8" width="56"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="12">
                       <polyline color="#ba00b8" width="64"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="13">
                       <polyline color="#ba00b8" width="104"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="14">
                       <polyline color="#ba00b8" width="128"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="15">
                       <polyline color="#ba00b8" width="136"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="16">
                       <polyline color="#ba00b8" width="264"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="17">
                       <polyline color="#ba00b8" width="536"/>
                   </itemgra>
                   <itemgra item_types="selected_line" order="18">
                       <polyline color="#ba00b8" width="1060"/>
                   </itemgra>
                   <itemgra item_types="street_nopass" order="10-">
                       <polyline color="#000000" width="2"/>
                   </itemgra>
                   <itemgra item_types="track_paved" order="15-">
                       <polyline color="#000000" width="2"/>
                   </itemgra>
                   <itemgra item_types="track_gravelled" order="12">
                       <polyline color="#664310" width="2" dash="3,6"/>
                   </itemgra>
                   <itemgra item_types="track_gravelled" order="13-14">
                       <polyline color="#ffffff" width="8" dash="4,8"/>
                       <polyline color="#664310" width="4" dash="4,8"/>
                   </itemgra>-->
                   <itemgra item_types="track_gravelled" order="15-16">
                       <polyline color="#ffffff" width="10" dash="5,10"/>
                       <polyline color="#A68F61" width="6" dash="5,10"/>
                   </itemgra>
                   <itemgra item_types="track_gravelled" order="17-">
                       <polyline color="#ffffff" width="14" dash="7,15"/>
                       <polyline color="#A68F61" width="10" dash="7,15"/>
                   </itemgra>
                   <itemgra item_types="track_unpaved" order="12-">
                       <polyline color="#664310" width="2"/>
                   </itemgra>
                   <itemgra item_types="bridleway" order="10-">
                       <polyline color="#52A750" width="2" dash="5,5"/>
                   </itemgra>
                  <!-- <itemgra item_types="piste_downhill_novice" order="10-12">
                       <polyline color="#00A000" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_novice" order="13-14">
                       <polyline color="#00A000" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_novice" order="15-16">
                       <polyline color="#00A000" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_novice" order="17-">
                       <polyline color="#00A000" width="10"/>
                   </itemgra>
                  <!--<itemgra item_types="piste_downhill_easy" order="10-12">
                       <polyline color="#0000ff" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_easy" order="13-14">
                       <polyline color="#0000ff" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_easy" order="15-16">
                       <polyline color="#0000ff" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_easy" order="17-">
                       <polyline color="#0000ff" width="10"/>
                   </itemgra>
                   <!--<itemgra item_types="piste_downhill_intermediate" order="10-12">
                       <polyline color="#ff0000" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_intermediate" order="13-14">
                       <polyline color="#ff0000" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_intermediate" order="15-16">
                       <polyline color="#ff0000" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_intermediate" order="17-">
                       <polyline color="#ff0000" width="10"/>
                   </itemgra>
                   <!--<itemgra item_types="piste_downhill_advanced" order="10-12">
                       <polyline color="#000000" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_advanced" order="13-14">
                       <polyline color="#000000" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_advanced" order="15-16">
                       <polyline color="#000000" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_advanced" order="17-">
                       <polyline color="#000000" width="10"/>
                   </itemgra>
                   <!--<itemgra item_types="piste_downhill_expert" order="10-12">
                       <polyline color="#ffaa00" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_expert" order="13-14">
                       <polyline color="#ffaa00" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_expert" order="15-16">
                       <polyline color="#ffaa00" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_expert" order="17-">
                       <polyline color="#ffaa00" width="10"/>
                   </itemgra>
                   <!--<itemgra item_types="piste_downhill_freeride" order="10-12">
                       <polyline color="#ffff00" width="2"/>
                   </itemgra>-->
                   <itemgra item_types="piste_downhill_freeride" order="13-14">
                       <polyline color="#ffff00" width="4"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_freeride" order="15-16">
                       <polyline color="#ffff00" width="6"/>
                   </itemgra>
                   <itemgra item_types="piste_downhill_freeride" order="17-">
                       <polyline color="#ffff00" width="10"/>
                   </itemgra>
                   <itemgra item_types="lift_cable_car" order="12-">
                       <polyline color="#696969" width="4" dash="10"/>
                   </itemgra>
                   <itemgra item_types="lift_chair" order="12-">
                       <polyline color="#696969" width="4" dash="10"/>
                   </itemgra>
                   <itemgra item_types="lift_drag" order="12-">
                       <polyline color="#696969" width="4" dash="10"/>
                   </itemgra>
                   <!--<itemgra item_types="piste_nordic" order="10-12">
                       <polyline color="#0000ff" width="2" dash="3,6" offset="4"/>
                   </itemgra>-->
                   <itemgra item_types="piste_nordic" order="13-14">
                       <polyline color="#ffffff" width="8" dash="4,8" offset="6"/>
                       <polyline color="#0000ff" width="4" dash="4,8" offset="6"/>
                   </itemgra>
                   <itemgra item_types="piste_nordic" order="15-16">
                       <polyline color="#ffffff" width="10" dash="5,10" offset="7"/>
                       <polyline color="#0000ff" width="6" dash="5,10" offset="7"/>
                   </itemgra>
                   <itemgra item_types="piste_nordic" order="17-">
                       <polyline color="#ffffff" width="14" dash="7,15" offset="10"/>
                       <polyline color="#0000ff" width="10" dash="7,15" offset="10"/>
                   </itemgra>
                   <!--<itemgra item_types="footway_and_piste_nordic" order="10-12">
                       <polyline color="#ff0000" width="4" dash="3,15"/>
                       <polyline color="#0000ff" width="4" dash="3,15" offset="15"/>
                   </itemgra>-->
                   <itemgra item_types="footway_and_piste_nordic" order="13-14">
                       <polyline color="#ffffff" width="8" dash="4,8"/>
                       <polyline color="#ff0000" width="4" dash="4,20"/>
                       <polyline color="#0000ff" width="4" dash="4,20" offset="12"/>
                   </itemgra>
                   <itemgra item_types="footway_and_piste_nordic" order="15-16">
                       <polyline color="#ffffff" width="10" dash="5,10"/>
                       <polyline color="#ff0000" width="6" dash="5,25"/>
                       <polyline color="#0000ff" width="6" dash="5,25" offset="15"/>
                   </itemgra>
                   <itemgra item_types="footway_and_piste_nordic" order="17-">
                       <polyline color="#ffffff" width="14" dash="7,15"/>
                       <polyline color="#ff0000" width="10" dash="7,37"/>
                       <polyline color="#0000ff" width="10" dash="7,37" offset="22"/>
                   </itemgra>

                   <!--<itemgra item_types="footway" order="10-12">
               <polyline color="#ffffff" width="6" dash="3,3"/>
                       <polyline color="#E8A591" width="4" dash="1,1"/>
                   </itemgra>
                   <itemgra item_types="footway" order="13-14">
                       <polyline color="#ffffff" width="8" dash="3,3"/>
                       <polyline color="#E8A591" width="6" dash="1,1"/>
                   </itemgra>
                   <itemgra item_types="footway" order="15-16">
                       <polyline color="#ffffff" width="10" dash="3,3"/>
                       <polyline color="#E8A591" width="8" dash="1,1"/>
                   </itemgra>
                   <itemgra item_types="footway" order="17-">
                       <polyline color="#ffffff" width="14" dash="3,3"/>
                       <polyline color="#E8A591" width="12" dash="1,1"/>
                   </itemgra>-->
                   <itemgra item_types="steps" order="13-">
                       <polyline color="#bbbbbb" width="4"/>
                   </itemgra>
                  <!-- <itemgra item_types="street_pedestrian" order="11">
                       <polyline color="#9A9889" width="6"/>
                       <polyline color="#E5E0C2" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="11">
                       <polyline color="#9A9889" width="10"/>
                       <polyline color="#E5E0C2" width="6"/>
                   </itemgra>-->
                   <itemgra item_types="street_pedestrian" order="12">
                       <polyline color="#9A9889" width="16"/>
                       <polyline color="#E5E0C2" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="13">
                       <polyline color="#9A9889" width="18"/>
                       <polyline color="#E5E0C2" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="14">
                       <polyline color="#9A9889" width="26"/>
                       <polyline color="#E5E0C2" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="15">
                       <polyline color="#9A9889" width="36"/>
                       <polyline color="#E5E0C2" width="28"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="16">
                       <polyline color="#9A9889" width="42"/>
                       <polyline color="#E5E0C2" width="34"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="17">
                       <polyline color="#9A9889" width="50"/>
                       <polyline color="#E5E0C2" width="42"/>
                   </itemgra>
                   <itemgra item_types="street_pedestrian" order="18">
                       <polyline color="#9A9889" width="80"/>
                       <polyline color="#E5E0C2" width="68"/>
                   </itemgra>
                   <!--<itemgra item_types="street_service" order="10">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#fefefe" width="4"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="11">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#fefefe" width="4"/>
                   </itemgra>-->
                   <itemgra item_types="street_service" order="12">
                       <polyline color="#9A9889" width="10"/>
                       <polyline color="#fefefe" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="13">
                       <polyline color="#9A9889" width="12"/>
                       <polyline color="#fefefe" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="14">
                       <polyline color="#9A9889" width="14"/>
                       <polyline color="#fefefe" width="10"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="15">
                       <polyline color="#9A9889" width="16"/>
                       <polyline color="#fefefe" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="16">
                       <polyline color="#9A9889" width="18"/>
                       <polyline color="#fefefe" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="17">
                       <polyline color="#9A9889" width="20"/>
                       <polyline color="#fefefe" width="16"/>
                   </itemgra>
                   <itemgra item_types="street_service" order="18">
                       <polyline color="#9A9889" width="22"/>
                       <polyline color="#fefefe" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="12">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#fefefe" width="4"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="13">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#fefefe" width="4"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="14">
                       <polyline color="#9A9889" width="10"/>
                       <polyline color="#fefefe" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="15">
                       <polyline color="#9A9889" width="12"/>
                       <polyline color="#fefefe" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="16">
                       <polyline color="#9A9889" width="14"/>
                       <polyline color="#fefefe" width="10"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="17">
                       <polyline color="#9A9889" width="16"/>
                       <polyline color="#fefefe" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_parking_lane" order="18">
                       <polyline color="#9A9889" width="18"/>
                       <polyline color="#fefefe" width="14"/>
                   </itemgra>
                   <!--<itemgra item_types="street_0,street_1_city,street_1_land" order="10">
                       <polyline color="#9A9889" width="8"/>
                       <polyline color="#ffffff" width="4"/>
                   </itemgra>-->
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="11">
                       <polyline color="#bbbbbb" width="3"/>
                   </itemgra>
                   
                           
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="12">
                       <polyline color="#9A9889" width="16"/>
                       <polyline color="#ffffff" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="13">
                       <polyline color="#9A9889" width="22"/>
                       <polyline color="#ffffff" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="14">
                       <polyline color="#9A9889" width="34"/>
                       <polyline color="#ffffff" width="26"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="15">
                       <polyline color="#9A9889" width="36"/>
                       <polyline color="#ffffff" width="28"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="16">
                       <polyline color="#9A9889" width="60"/>
                       <polyline color="#ffffff" width="52"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="17">
                       <polyline color="#9A9889" width="134"/>
                       <polyline color="#ffffff" width="122"/>
                   </itemgra>
                   <itemgra item_types="street_0,street_1_city,street_1_land" order="18">
                       <polyline color="#9A9889" width="264"/>
                       <polyline color="#ffffff" width="252"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="7-8">
                       <polyline color="#E0E08D" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="9">
                       <polyline color="#E0E08D" width="9"/>
                       <polyline color="#FFFF90" width="3"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="10">
                       <polyline color="#E0E08D" width="12"/>
                       <polyline color="#FFFF90" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="11">
                       <polyline color="#E0E08D" width="15"/>
                       <polyline color="#FFFF90" width="9"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="12">
                       <polyline color="#E0E08D" width="21"/>
                       <polyline color="#FFFF90" width="15"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="13">
                       <polyline color="#E0E08D" width="33"/>
                       <polyline color="#FFFF90" width="24"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="14">
                       <polyline color="#E0E08D" width="42"/>
                       <polyline color="#FFFF90" width="33"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="15">
                       <polyline color="#E0E08D" width="57"/>
                       <polyline color="#FFFF90" width="45"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="16">
                       <polyline color="#E0E08D" width="90"/>
                       <polyline color="#FFFF90" width="78"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="17">
                       <polyline color="#E0E08D" width="189"/>
                       <polyline color="#FFFF90" width="171"/>
                   </itemgra>
                   <itemgra item_types="street_2_city,street_2_land" order="18">
                       <polyline color="#E0E08D" width="300"/>
                       <polyline color="#FFFF90" width="270"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="7-8">
                       <polyline color="#E0E08D" width="4"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="9">
                       <polyline color="#E0E08D" width="6"/>
                       <polyline color="#FFFF90" width="2"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="10">
                       <polyline color="#E0E08D" width="8"/>
                       <polyline color="#FFFF90" width="4"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="11">
                       <polyline color="#E0E08D" width="10"/>
                       <polyline color="#FFFF90" width="6"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="12">
                       <polyline color="#E0E08D" width="14"/>
                       <polyline color="#FFFF90" width="10"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="13">
                       <polyline color="#E0E08D" width="22"/>
                       <polyline color="#FFFF90" width="16"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="14">
                       <polyline color="#E0E08D" width="28"/>
                       <polyline color="#FFFF90" width="22"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="15">
                       <polyline color="#E0E08D" width="38"/>
                       <polyline color="#FFFF90" width="30"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="16">
                       <polyline color="#E0E08D" width="60"/>
                       <polyline color="#FFFF90" width="52"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="17">
                       <polyline color="#E0E08D" width="126"/>
                       <polyline color="#FFFF90" width="114"/>
                   </itemgra>
                   <itemgra item_types="ramp" order="18">
                       <polyline color="#E0E08D" width="200"/>
                       <polyline color="#FFFF90" width="180"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="7-8">
                       <polyline color="#D8B384" width="6"/>
                       <polyline color="#FDBF70" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="9">
                       <polyline color="#D8B384" width="10"/>
                       <polyline color="#FDBF70" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="10">
                       <polyline color="#D8B384" width="16"/>
                       <polyline color="#FDBF70" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="11">
                       <polyline color="#D8B384" width="18"/>
                       <polyline color="#FDBF70" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="12">
                       <polyline color="#D8B384" width="26"/>
                       <polyline color="#FDBF70" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="13">
                       <polyline color="#D8B384" width="36"/>
                       <polyline color="#FDBF70" width="28"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="14">
                       <polyline color="#D8B384" width="42"/>
                       <polyline color="#FDBF70" width="34"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="15">
                       <polyline color="#D8B384" width="50"/>
                       <polyline color="#FDBF70" width="42"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="16">
                       <polyline color="#D8B384" width="80"/>
                       <polyline color="#FDBF70" width="68"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="17">
                       <polyline color="#D8B384" width="158"/>
                       <polyline color="#FDBF70" width="146"/>
                   </itemgra>
                   <itemgra item_types="street_3_city,street_3_land,roundabout" order="18">
                       <polyline color="#D8B384" width="312"/>
                       <polyline color="#FDBF70" width="300"/>
                   </itemgra>
                    <itemgra item_types="street_4_city,street_4_land" order="7-8">
                       <polyline color="#D78C8D" width="6"/>
                       <polyline color="#E46D71" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="9">
                       <polyline color="#D78C8D" width="10"/>
                       <polyline color="#E46D71" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="10">
                       <polyline color="#D78C8D" width="12"/>
                       <polyline color="#E46D71" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="11">
                       <polyline color="#D78C8D" width="18"/>
                       <polyline color="#E46D71" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="12">
                       <polyline color="#D78C8D" width="26"/>
                       <polyline color="#E46D71" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="13">
                       <polyline color="#D78C8D" width="36"/>
                       <polyline color="#E46D71" width="28"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="14">
                       <polyline color="#D78C8D" width="42"/>
                       <polyline color="#E46D71" width="34"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="15">
                       <polyline color="#D78C8D" width="48"/>
                       <polyline color="#E46D71" width="40"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="16">
                       <polyline color="#D78C8D" width="78"/>
                       <polyline color="#E46D71" width="66"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="17">
                       <polyline color="#D78C8D" width="156"/>
                       <polyline color="#E46D71" width="144"/>
                   </itemgra>
                   <itemgra item_types="street_4_city,street_4_land" order="18">
                       <polyline color="#D78C8D" width="312"/>
                       <polyline color="#E46D71" width="300"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="2">
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="3-5">
                       <polyline color="#466083" width="6"/>
                       <polyline color="#809BC0" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="6">
                       <polyline color="#466083" width="8"/>
                       <polyline color="#809BC0" width="4"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="7-8">
                       <polyline color="#466083" width="14"/>
                       <polyline color="#809BC0" width="10"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="9-10">
                       <polyline color="#466083" width="18"/>
                       <polyline color="#809BC0" width="10"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="11">
                       <polyline color="#466083" width="26"/>
                       <polyline color="#809BC0" width="18"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="12">
                       <polyline color="#466083" width="30"/>
                       <polyline color="#809BC0" width="20"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="13">
                       <polyline color="#466083" width="50"/>
                       <polyline color="#809BC0" width="34"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="14">
                       <polyline color="#466083" width="62"/>
                       <polyline color="#809BC0" width="48"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="15">
                       <polyline color="#466083" width="66"/>
                       <polyline color="#809BC0" width="54"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="16">
                       <polyline color="#466083" width="130"/>
                       <polyline color="#809BC0" width="118"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="17">
                       <polyline color="#466083" width="266"/>
                       <polyline color="#809BC0" width="254"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>
                   <itemgra item_types="highway_land,highway_city" order="18">
                       <polyline color="#466083" width="528"/>
                       <polyline color="#809BC0" width="516"/>
                       <polyline color="#466083" width="2"/>
                   </itemgra>


                       <itemgra item_types="border_country" order="8-11">
                      <polyline color="#000000" width="12" />
                         <polyline color="#ffffff" width="4" dash="10,1"/>
                   </itemgra>
                   <itemgra item_types="border_country" order="11-">
                      <polyline color="#000000" width="24" />
                         <polyline color="#ffffff" width="8" dash="10,1"/>
                   </itemgra>


                     <itemgra item_types="street_n_lanes" order="6">
                       <polyline color="#658F65" width="4"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="7-8">
                       <polyline color="#315231" width="12"/>
                       <polyline color="#658F65" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="9-10">
                       <polyline color="#315231" width="16"/>
                       <polyline color="#658F65" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="11">
                       <polyline color="#315231" width="22"/>
                       <polyline color="#658F65" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="12">
                       <polyline color="#315231" width="26"/>
                       <polyline color="#658F65" width="16"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="13">
                       <polyline color="#315231" width="40"/>
                       <polyline color="#658F65" width="22"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="14">
                       <polyline color="#315231" width="50"/>
                       <polyline color="#658F65" width="34"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="15">
                       <polyline color="#315231" width="56"/>
                       <polyline color="#658F65" width="46"/>
                       <polyline color="#315231" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="16">
                       <polyline color="#315231" width="90"/>
                       <polyline color="#658F65" width="60"/>
                       <polyline color="#315231" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="17">
                       <polyline color="#315231" width="266"/>
                       <polyline color="#658F65" width="254"/>
                       <polyline color="#315231" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_n_lanes" order="18">
                       <polyline color="#315231" width="528"/>
                       <polyline color="#658F65" width="516"/>
                       <polyline color="#315231" width="2"/>
                   </itemgra>
           <itemgra item_types="height_line_1" order="1-18">
                       <polyline color="#00FFFF" width="4"/>
                   </itemgra>

           

           <!-- ROUTING -->

                   <itemgra item_types="street_route" order="2">
                       <polyline color="#00FF00" width="1"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="3-5">
                       <polyline color="#00FF00" width="2"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="6">
                       <polyline color="#00FF00" width="4"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="7-8">
                       <polyline color="#00FF00" width="6"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="9-10">
                       <polyline color="#00FF00" width="8"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="11">
                       <polyline color="#00FF00" width="10"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="12">
                       <polyline color="#00FF00" width="12"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="13">
                       <polyline color="#00FF00" width="14"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="14">
                       <polyline color="#00FF00" width="16"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="15">
                       <polyline color="#00FF00" width="18"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="16">
                       <polyline color="#00FF00" width="30"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="17">
                       <polyline color="#00FF00" width="40"/>
                   </itemgra>
                   <itemgra item_types="street_route" order="18">
                       <polyline color="#00FF00" width="72"/>
                   </itemgra>
           <!-- ROUTING -->

                   <!-- This entry shows all unknown linear elements as blue lines -->
                   <!--
                   <itemgra item_types="street_unkn" order="0-">
                       <polyline color="#8080ff" width="6"/>
                   </itemgra>
                   -->
                   <itemgra item_types="tracking_0" order="0-">
                       <polyline color="#000000" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_10" order="0-">
                       <polyline color="#191919" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_20" order="0-">
                       <polyline color="#333333" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_30" order="0-">
                       <polyline color="#4c4c4c" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_40" order="0-">
                       <polyline color="#666666" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_50" order="0-">
                       <polyline color="#7f7f7f" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_60" order="0-">
                       <polyline color="#999999" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_70" order="0-">
                       <polyline color="#b2b2b2" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_80" order="0-">
                       <polyline color="#cccccc" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_90" order="0-">
                       <polyline color="#e5e5e5" width="6"/>
                   </itemgra>
                   <itemgra item_types="tracking_100" order="0-">
                       <polyline color="#ffffff" width="6"/>
                   </itemgra>
                   <itemgra item_types="highway_exit_label" order="10-">
                       <circle color="#000000" radius="3" text_size="7"/>
                   </itemgra>
                   
                   <itemgra item_types="highway_city,highway_land,street_4_city,street_4_land,street_n_lanes" order="10-13">
                       <text text_size="14"/>
                   </itemgra>
                   <itemgra item_types="highway_city,highway_land,street_4_city,street_4_land,street_n_lanes" order="14">
                       <text text_size="20"/>
                   </itemgra>
                   <itemgra item_types="highway_city,highway_land,street_4_city,street_4_land,street_n_lanes" order="8-18">
                       <text text_size="26"/>
                   </itemgra>
                   <itemgra item_types="highway_city,highway_land,street_4_city,street_4_land,street_n_lanes" order="17-18">
                       <text text_size="36"/>
                   </itemgra>

                   
                   <itemgra item_types="street_2_city,street_2_land,street_3_city,street_3_land,ramp" order="10-13">
                       <text text_size="12"/>
                   </itemgra>
                    <itemgra item_types="street_2_city,street_2_land,street_3_city,street_3_land,ramp" order="14">
                       <text text_size="18"/>
                   </itemgra>
                    <itemgra item_types="street_2_city,street_2_land,street_3_city,street_3_land,ramp" order="15-16">
                       <text text_size="24"/>
                   </itemgra>
                    <itemgra item_types="street_2_city,street_2_land,street_3_city,street_3_land,ramp" order="17-18">
                       <text text_size="32"/>
                   </itemgra>
                   

                   <itemgra item_types="street_nopass,street_0,street_1_city,street_1_land,street_pedestrian" order="12-13">
                       <text text_size="12"/>
                   </itemgra>
                   <itemgra item_types="street_nopass,street_0,street_1_city,street_1_land,street_pedestrian" order="14">
                       <text text_size="18"/>
                   </itemgra>
                   <itemgra item_types="street_nopass,street_0,street_1_city,street_1_land,street_pedestrian" order="15-16">
                       <text text_size="24"/>
                   </itemgra>
                   <itemgra item_types="street_nopass,street_0,street_1_city,street_1_land,street_pedestrian" order="17-18">
                       <text text_size="32"/>
                   </itemgra>
                   
                   
                   <itemgra item_types="traffic_distortion" order="2">
                       <polyline color="#ff9000" width="4"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="3-5">
                       <polyline color="#ff9000" width="8"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="6">
                       <polyline color="#ff9000" width="10"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="7-8">
                       <polyline color="#ff9000" width="16"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="9-10">
                       <polyline color="#ff9000" width="20"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="11">
                       <polyline color="#ff9000" width="28"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="12">
                       <polyline color="#ff9000" width="32"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="13">
                       <polyline color="#ff9000" width="52"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="14">
                       <polyline color="#ff9000" width="64"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="15">
                       <polyline color="#ff9000" width="68"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="16">
                       <polyline color="#ff9000" width="132"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="17">
                       <polyline color="#ff9000" width="268"/>
                   </itemgra>
                   <itemgra item_types="traffic_distortion" order="18">
                       <polyline color="#ff9000" width="530"/>
                   </itemgra>
               </layer>
               <layer name="polylines">
                   <itemgra item_types="aeroway_taxiway" order="10">
                       <polyline color="#9494AC" width="8"/>
                       <polyline color="#BBBBCC" width="4"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="11">
                       <polyline color="#9494AC" width="12"/>
                       <polyline color="#BBBBCC" width="8"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="12">
                       <polyline color="#9494AC" width="20"/>
                       <polyline color="#BBBBCC" width="16"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="13">
                       <polyline color="#9494AC" width="24"/>
                       <polyline color="#BBBBCC" width="18"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="14">
                       <polyline color="#9494AC" width="30"/>
                       <polyline color="#BBBBCC" width="26"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="15">
                       <polyline color="#9494AC" width="34"/>
                       <polyline color="#BBBBCC" width="28"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="16">
                       <polyline color="#9494AC" width="66"/>
                       <polyline color="#BBBBCC" width="52"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="17">
                       <polyline color="#9494AC" width="138"/>
                       <polyline color="#BBBBCC" width="122"/>
                   </itemgra>
                   <itemgra item_types="aeroway_taxiway" order="18">
                       <polyline color="#9494AC" width="264"/>
                       <polyline color="#BBBBCC" width="252"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="2-6">
                       <polyline color="#BBBBCC" width="2"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="7-8">
                       <polyline color="#565676" width="6"/>
                       <polyline color="#BBBBCC" width="2"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="9">
                       <polyline color="#565676" width="10"/>
                       <polyline color="#BBBBCC" width="6"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="10">
                       <polyline color="#565676" width="12"/>
                       <polyline color="#BBBBCC" width="8"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="11">
                       <polyline color="#565676" width="18"/>
                       <polyline color="#BBBBCC" width="14"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="12">
                       <polyline color="#565676" width="26"/>
                       <polyline color="#BBBBCC" width="18"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="13">
                       <polyline color="#565676" width="36"/>
                       <polyline color="#BBBBCC" width="28"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="14">
                       <polyline color="#565676" width="42"/>
                       <polyline color="#BBBBCC" width="34"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="15">
                       <polyline color="#565676" width="48"/>
                       <polyline color="#BBBBCC" width="40"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="16">
                       <polyline color="#565676" width="78"/>
                       <polyline color="#BBBBCC" width="66"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="17">
                       <polyline color="#565676" width="156"/>
                       <polyline color="#BBBBCC" width="144"/>
                   </itemgra>
                   <itemgra item_types="aeroway_runway" order="18">
                       <polyline color="#565676" width="312"/>
                       <polyline color="#BBBBCC" width="300"/>
                   </itemgra>
                   <itemgra item_types="rail_tram" order="12">
                       <polyline color="#0000A5" width="2"/>
                   </itemgra>
                   <itemgra item_types="rail_tram" order="13-">
                       <polyline color="#0000A5" width="4"/>
                   </itemgra>
           <itemgra item_types="footway" order="13-">
                       <!--<polyline color="#C4BB88" width="10" dash="7,15"/>
                       <polyline color="#E5E0C2" width="4"/>-->
               <polyline color="#999999" width="4" dash="5,5"/>
                   </itemgra>
           <!--<itemgra item_types="footway" order="10-">
                       <polyline color="#C4BB88" width="6" dash="7,15"/>
                       <polyline color="#E5E0C2" width="2"/>
               <polyline color="#FF6767" width="6" dash="5,5"/>
                   </itemgra>-->

           <!--<itemgra item_types="cycleway" order="13-">
                       <polyline color="#C4BB88" width="10" dash="7,15"/>
                       <polyline color="#D2FAD1" width="4"/>
               <polyline color="#871F78" width="4" dash="2,2" />
                   </itemgra>
           <itemgra item_types="cycleway" order="10-">
                       <polyline color="#C4BB88" width="10" dash="7,15"/>
                       <polyline color="#D2FAD1" width="4"/>
               <polyline color="#871F78" width="4" dash="2,2" />
                   </itemgra>-->
                   <itemgra item_types="track" order="7-">
                       <polyline color="#3f3f3f" width="2"/>
                   </itemgra>
                   <itemgra item_types="track_tracked" order="7-">
                       <polyline color="#3f3fff" width="6"/>
                   </itemgra>
                  <!-- <itemgra item_types="powerline" order="8-10">
                       <polyline color="#918A8A" width="2" dash="15,15"/>
                   </itemgra>-->

               </layer>
               
               <layer name="POI Symbols">

   <!-- IMPORTANT sort by layer -->
                   <itemgra item_types="poi_bar" order="15-">
                       <icon src="/sdcard/navit/icons/food_bar.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_airport" order="5-">
                       <icon src="/sdcard/navit/icons/transport_airport2.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_airport_terminal" order="5-">
                       <icon src="/sdcard/navit/icons/transport_airport_terminal.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_aerodrome" order="5-">
                       <icon src="/sdcard/navit/icons/transport_aerodrome2.n.32.png"/>
                   </itemgra>      
                   <itemgra item_types="poi_fuel" order="11-">
                       <icon src="/sdcard/navit/icons/transport_fuel.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_car_parking" order="13-">
                       <icon src="/sdcard/navit/icons/transport_parking.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_camping" order="10-">
                       <icon src="/sdcard/navit/icons/accommodation_camping.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_fastfood" order="15-">
                       <icon src="/sdcard/navit/icons/food_fastfood2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_restaurant" order="15-">
                       <icon src="/sdcard/navit/icons/food_restaurant.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_cafe" order="15-">
                       <icon src="/sdcard/navit/icons/food_cafe.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bar" order="15-">
                       <icon src="/sdcard/navit/icons/food_bar.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_museum_history" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_museum.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_cinema" order="13-" zoom="0">
                       <icon src="/sdcard/navit/icons/tourist_cinema2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_attraction" order="12-">
                       <icon src="/sdcard/navit/icons/tourist_attraction.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_theatre" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_theatre.p.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_zoo" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_zoo.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_casino" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_casino.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_atm" order="12-">
                       <icon src="/sdcard/navit/icons/money_atm2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_handg" order="14-">
                       <icon src="/sdcard/navit/icons/shopping_diy.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shopping" order="14-">
                       <icon src="/sdcard/navit/icons/shopping_supermarket.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_hospital" order="13-">
                       <icon src="/sdcard/navit/icons/health_hospital_emergency.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_pharmacy" order="13-">
                       <icon src="/sdcard/navit/icons/health_pharmacy.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_drinking_water" order="12-">
                       <icon src="/sdcard/navit/icons/food_drinkingtap.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_swimming" order="13-">
                       <icon src="/sdcard/navit/icons/sport_swimming_indoor.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_skiing" order="13-">
                       <icon src="/sdcard/navit/icons/sport_skiing_downhill.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="tec_common" order="12-">
                       <icon src="/sdcard/navit/icons/traffic_camera.png" w="24" h="24"/>
                   </itemgra>
                   <itemgra item_types="tec_mobile" order="12-">
                       <icon src="/sdcard/navit/icons/traffic_camera_mobile.png" w="24" h="24"/>
                   </itemgra>
                   <itemgra item_types="poi_biergarten" order="14-">
                       <icon src="/sdcard/navit/icons/biergarten.xpm"/>
                   </itemgra>
                       <itemgra item_types="poi_post_office" order="14-">
                       <icon src="/sdcard/navit/icons/amenity_post_office.glow.20.png"/>
                   </itemgra>
                       <itemgra item_types="poi_recycling" order="14-">
                       <icon src="/sdcard/navit/icons/amenity_recycling.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_restroom" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_toilets.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bus_stop" order="12">
                       <circle color="#ff0000" radius="4" width="4"/>
                   </itemgra>
                   <itemgra item_types="poi_bus_stop" order="13-">
                       <circle color="#ff0000" radius="6" width="6"/>
                   </itemgra>
                   <itemgra item_types="poi_bus_station" order="13-">
                       <icon src="/sdcard/navit/icons/transport_bus_station.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_border_station" order="10-">
                       <icon src="/sdcard/navit/icons/poi_boundary_administrative.n.20.png"/>
                   </itemgra> 
                   <itemgra item_types="barrier_bollard" order="13-">
                       <icon src="/sdcard/navit/icons/barrier_bollard.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_castle" order="14-">
                       <icon src="/sdcard/navit/icons/tourist_castle2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_grocery" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_convenience.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_information" order="12-">
                       <icon src="/sdcard/navit/icons/amenity_information.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bank" order="14-">
                       <icon src="/sdcard/navit/icons/money_bank2.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_viewpoint" order="10-">
                       <icon src="/sdcard/navit/icons/tourist_view_point.glow.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_mine" order="10-">
                       <icon src="/sdcard/navit/icons/poi_mine.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_apparel" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_clothes.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_level_crossing" order="11-">
                       <icon src="/sdcard/navit/icons/level_crossing.xpm"/>
                   </itemgra>
                   
                   <!--<itemgra item_types="poi_rail_halt" order="11-">
                       <circle color="#ff0000" radius="3" width="3"/>
                       <circle color="#000000" radius="5" width="2" text_size="8"/>
                   </itemgra>-->s
                   <itemgra item_types="poi_rail_station" order="12"> <!-- UBAHN-->
                       <circle color="#0000A5" radius="7" width="7"/>
                   </itemgra>
                   <itemgra item_types="poi_rail_station" order="13-"> <!-- UBAHN-->
                       <circle color="#0000A5" radius="9" width="9"/>
                   </itemgra>
                                
                   <itemgra item_types="poi_rail_tram_stop" order="12">
                       <circle color="#0000A5" radius="4" width="4"/>
                   </itemgra>
                   <itemgra item_types="poi_rail_tram_stop" order="13-">
                       <circle color="#0000A5" radius="6" width="6"/>
                   </itemgra>



   <!-- HOTEL and FOOT poi_resort, poi_motel,poi_hotel,poi_restaurant,poi_cafe,poi_bar,poi_fastfood,
                   <itemgra item_types="poi_resort" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_theme_park.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_motel" order="13-">
                       <icon src="/sdcard/navit/icons/accommodation_motel.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_hotel" order="13-">
                       <icon src="/sdcard/navit/icons/accommodation_hotel.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_restaurant" order="15-">
                       <icon src="/sdcard/navit/icons/food_restaurant.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_cafe" order="15-">
                       <icon src="/sdcard/navit/icons/food_cafe.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bar" order="15-">
                       <icon src="/sdcard/navit/icons/food_bar.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_fastfood" order="15-">
                       <icon src="/sdcard/navit/icons/food_fastfood2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_pub" order="14-">
                       <icon src="/sdcard/navit/icons/food_pub.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_camp_rv" order="13-">
                       <icon src="/sdcard/navit/icons/accommodation_caravan_park.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_camping" order="10-">
                       <icon src="/sdcard/navit/icons/accommodation_camping.n.32.png"/>
                   </itemgra> -->

                   
                   
                   
   <!-- MUSEUMS and ATTRACTIONS poi_museum_history, poi_attraction,poi_theater,poi_zoo,poi_castle, poi_ruins,poi_memorial,poi_monument,poi_viewpoint  
                   <itemgra item_types="poi_museum_history" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_museum.n.32.png"/>
                   </itemgra>
                       <itemgra item_types="poi_cinema" order="14-" zoom="0">
                       <icon src="/sdcard/navit/icons/tourist_cinema2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_attraction" order="12-">
                       <icon src="/sdcard/navit/icons/tourist_attraction.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_theatre" order="8-">
                       <icon src="/sdcard/navit/icons/tourist_theatre.p.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_zoo" order="9-">
                       <icon src="/sdcard/navit/icons/tourist_zoo.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_castle" order="14-">
                       <icon src="/sdcard/navit/icons/tourist_castle2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_ruins" order="14-">
                       <icon src="/sdcard/navit/icons/tourist_ruin.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_memorial" order="16-">
                       <icon src="/sdcard/navit/icons/tourist_memorial.glow.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_monument" order="16-">
                       <icon src="/sdcard/navit/icons/tourist_monument.glow.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_fountain" order="16-">
                       <icon src="/sdcard/navit/icons/tourist_fountain.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_casino" order="13-">
                       <icon src="/sdcard/navit/icons/tourist_casino.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_cemetery" order="13-">
                       <icon src="/sdcard/navit/icons/place_of_worship_christian3.glow.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_information" order="12-">
                       <icon src="/sdcard/navit/icons/amenity_information.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_library" order="12-">
                       <icon src="/sdcard/navit/icons/amenity_library.glow.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_theatre" order="8-">
                       <icon src="/sdcard/navit/icons/tourist_theatre.p.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_zoo" order="9-">
                       <icon src="/sdcard/navit/icons/tourist_zoo.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_picnic" order="12-">
                       <icon src="/sdcard/navit/icons/tourist_picnic.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_viewpoint" order="10-">
                       <icon src="/sdcard/navit/icons/tourist_view_point.glow.24.png"/>
                   </itemgra> -->

   <!-- SHOPPING poi_shop_grocery,poi_shopping,   
                   <itemgra item_types="poi_shop_grocery" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_convenience.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bank" order="15-">
                       <icon src="/sdcard/navit/icons/money_bank2.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_atm" order="13-">
                       <icon src="/sdcard/navit/icons/money_atm2.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bureau_de_change" order="13-">
                       <icon src="/sdcard/navit/icons/money_currency_exchange.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shopping" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_supermarket.n.32.png"/>
                   </itemgra>
                       <itemgra item_types="poi_repair_service" order="12-">
                       <icon src="/sdcard/navit/icons/shopping_car_repair.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_car_dealer_parts" order="14-">
                       <icon src="/sdcard/navit/icons/shopping_car.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_apparel" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_clothes.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_computer" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_computer.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_shop_handg" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_diy.p.32.png"/>
                   </itemgra>
                       <itemgra item_types="poi_shop_garden_centre" order="15-">
                       <icon src="/sdcard/navit/icons/shopping_garden_centre.p.32.png"/>
                   </itemgra> -->
                   
                   
   <!-- SECURITY poi_hospital,poi_police,poi_pharmacy
                   <itemgra item_types="poi_hospital" order="13-">
                       <icon src="/sdcard/navit/icons/health_hospital_emergency.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_er" order="13-">
                       <icon src="/sdcard/navit/icons/health_hospital_emergency2.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_fire_station" order="12-">
                       <icon src="/sdcard/navit/icons/amenity_firestation3.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_police" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_police2.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_courthouse" order="12-">
                       <icon src="/sdcard/navit/icons/amenity_court.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_marine" order="12-">
                       <icon src="/sdcard/navit/icons/transport_marina.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_military" order="11-">
                       <icon src="/sdcard/navit/icons/military.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_mine" order="10-">
                       <icon src="/sdcard/navit/icons/poi_mine.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_forbiden_area" order="12-">
                       <icon src="/sdcard/navit/icons/forbiden_area.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_pharmacy" order="13-">
                       <icon src="/sdcard/navit/icons/health_pharmacy.n.32.png"/>
                   </itemgra> -->
                  
   <!-- SPORTS 
                   <itemgra item_types="poi_diving" order="14-">
                       <icon src="/sdcard/navit/icons/sport_diving.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bowling" order="13-">
                       <icon src="/sdcard/navit/icons/bowling.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_fish" order="9-">
                       <icon src="/sdcard/navit/icons/fish.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_golf" order="12-">
                       <icon src="/sdcard/navit/icons/sport_golf.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_drinking_water" order="12-">
                       <icon src="/sdcard/navit/icons/food_drinkingtap.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_sailing" order="12-">
                       <icon src="/sdcard/navit/icons/sport_sailing.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_swimming" order="15-">
                       <icon src="/sdcard/navit/icons/sport_swimming_indoor.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_skiing" order="11-">
                       <icon src="/sdcard/navit/icons/sport_skiing_downhill.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_sport" order="15-">
                       <icon src="/sdcard/navit/icons/sport_leisure_centre.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_stadium" order="15-">
                       <icon src="/sdcard/navit/icons/stadium.xpm"/>
                   </itemgra>
                    -->
                   
   <!-- ELSE poi_airport,highway_exit,poi_school,poi_church,poi_repair_service, poi_bank,poi_sport,poi_stadium,poi_swimmingpoi_shelter      
                   <itemgra item_types="poi_college" order="14-" zoom="0">
                       <icon src="/sdcard/navit/icons/education_colledge.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_school" order="16-">
                       <icon src="/sdcard/navit/icons/education_school_secondary.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_university" order="12-">
                       <icon src="/sdcard/navit/icons/education_university.p.32.png"/>
                   </itemgra>  
                   <itemgra item_types="poi_border_station" order="14-">
                       <icon src="/sdcard/navit/icons/poi_boundary_administrative.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_church" order="15-">
                       <icon src="/sdcard/navit/icons/place_of_worship_christian.glow.24.png"/>
                   </itemgra>
                       <itemgra item_types="poi_crossing" order="15-">
                       <icon src="/sdcard/navit/icons/crossing.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_danger_area" order="12-">
                       <icon src="/sdcard/navit/icons/danger_area.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_dangerous" order="12-">
                       <icon src="/sdcard/navit/icons/dangerous.xpm"/>
                   </itemgra>
                       <itemgra item_types="poi_emergency" order="12-">
                       <icon src="/sdcard/navit/icons/emergency.xpm"/>>  
                   <itemgra>
                   <itemgra item_types="mini_roundabout" order="13-">
                       <circle color="#ffffff" radius="3"/>
                   </itemgra>
                   <itemgra item_types="turning_circle" order="13-">
                       <circle color="#ffffff" radius="3"/>
                   </itemgra>
                           <itemgra item_types="poi_car_rent" order="15-">
                       <icon src="/sdcard/navit/icons/transport_rental_car.p.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_heliport" order="8-">
                       <icon src="/sdcard/navit/icons/transport_helicopter.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_level_crossing" order="11-">
                       <icon src="/sdcard/navit/icons/level_crossing.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_oil_field" order="12-">
                       <icon src="/sdcard/navit/icons/oil_field.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_post" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_post_box.p.32.png"/>
                   </itemgra>
                   
                   <itemgra item_types="poi_telephone" order="15-">
                       <icon src="/sdcard/navit/icons/amenity_telephone.p.32.png"/>
                   </itemgra>
                   
                   <itemgra item_types="poi_tower" order="13-">
                       <icon src="/sdcard/navit/icons/poi_tower_communications.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="power_tower" order="11-">
                       <icon src="/sdcard/navit/icons/poi_tower_power.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="power_pole" order="11-">
                          <polyline color="#918A8A" width="2">
                               <coord x="0" y="0"/>
                               <coord x="0" y="2"/>
                               <coord x="2" y="2"/>
                               <coord x="2" y="0"/>
                               <coord x="0" y="0"/>
                           </polyline>
                   </itemgra>
                   <itemgra item_types="poi_bench" order="16-">
                       <icon src="/sdcard/navit/icons/amenity_bench.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_waste_basket" order="16-">
                       <icon src="/sdcard/navit/icons/amenity_waste_bin.p.32.png"/>
                   </itemgra>  
                   <itemgra item_types="poi_taxi" order="13-">
                       <icon src="/sdcard/navit/icons/transport_taxi_rank.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poly_flats,poly_scrub,poly_marine,plantation,tundra" order="9-">
                       <polygon color="#a0a0a0"/>
                       <text text_size="5"/>
                   </itemgra>
                   <itemgra item_types="rg_segment" order="12-">
                       <polyline color="#FF089C" width="1"/>
                       <arrows color="#FF089C" width="1"/>
                   </itemgra>
                   <itemgra item_types="rg_point" order="12-">
                       <circle color="#FF089C" radius="10" text_size="7"/>
                   </itemgra> 
                   <itemgra item_types="tec_common" order="11-">
                       <icon src="/sdcard/navit/icons/traffic_camera.png" w="24" h="24"/>
                   </itemgra>
                   <itemgra item_types="tec_mobile" order="11-">
                       <icon src="/sdcard/navit/icons/traffic_camera_mobile.png" w="24" h="24"/>
                   </itemgra>
                   <itemgra item_types="poi_biergarten" order="14-">
                       <icon src="/sdcard/navit/icons/biergarten.xpm"/>
                   </itemgra>
                       <itemgra item_types="poi_post_office" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_post_office.glow.20.png"/>
                   </itemgra>
                       <itemgra item_types="poi_recycling" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_recycling.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_restroom" order="13-">
                       <icon src="/sdcard/navit/icons/amenity_toilets.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bus_stop" order="15-">
                       <circle color="#0000ff" radius="3"/>
                   </itemgra>
                   <itemgra item_types="poi_bus_station" order="15-">
                       <icon src="/sdcard/navit/icons/transport_bus_station.n.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_border_station" order="14-">
                       <icon src="/sdcard/navit/icons/poi_boundary_administrative.n.20.png"/>
                   </itemgra> 
                   <aitemgra item_types="poi__halt" order="11-">
                       <circle color="#ff0000" radius="3" width="3"/>
                       <circle color="#000000" radius="5" width="2" text_size="8"/>
                   </itemgra>
                   <itemgra item_types="poi__station" order="9-">
                       <circle color="#ff0000" radius="3" width="3"/>
                       <circle color="#000000" radius="6" width="2" text_size="8"/>
                   </itemgra>
                   <itemgra item_types="poi__tram_stop" order="10-11">
                       <circle color="#ff0000" radius="2" width="2"/>
                   </itemgra>
                   <itemgra item_types="poi__tram_stop" order="12-">
                       <circle color="#ff0000" radius="3" width="3"/>
                       <circle color="#606060" radius="5" width="2" text_size="8"/>
                   </itemgra>   -->    
               
   <!-- BARRIERS 
                   <itemgra item_types="barrier_bollard" order="13-">
                       <icon src="/sdcard/navit/icons/barrier_bollard.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_cycle" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_cycle_barrier.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_lift_gate" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_lift_gate.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_cattle_grid" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_cattle_grid.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_gate" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_gate.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_kissing_gate" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_kissing_gate.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_stile" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_stile.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="barrier_toll_booth" order="14-">
                       <icon src="/sdcard/navit/icons/barrier_toll_booth.p.32.png"/>
                   </itemgra> --> 
                            
               
               
           <!-- HIGHEST LEVEL POIs -->
           <!-- Solid background 
                   <itemgra item_types="poi_airport" order="5-">
                       <icon src="/sdcard/navit/icons/transport_airport2.n.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_airport_terminal" order="5-">
                       <icon src="/sdcard/navit/icons/transport_airport_terminal.p.32.png"/>
                   </itemgra>
                   <itemgra item_types="poi_aerodrome" order="5-">
                       <icon src="/sdcard/navit/icons/transport_aerodrome2.n.32.png"/>
                   </itemgra>      
                   <itemgra item_types="poi_fuel" order="6-">
                       <icon src="/sdcard/navit/icons/transport_fuel.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="poi_car_parking" order="13-">
                       <icon src="/sdcard/navit/icons/transport_parking.n.24.png"/>
                   </itemgra>
                   <itemgra item_types="highway_exit" order="10-">
                       <icon src="/sdcard/navit/icons/exit.png"/>
                   </itemgra>
                   <itemgra item_types="poi_bay" order="12-">
                       <icon src="/sdcard/navit/icons/bay.xpm"/>
                   </itemgra>
                   <itemgra item_types="poi_boat_ramp" order="12-">
                       <icon src="/sdcard/navit/icons/boat_ramp.png"/>
                   </itemgra>
                   <itemgra item_types="poi_peak" order="7-">
                       <icon src="/sdcard/navit/icons/poi_peak2.glow.20.png"/>
                   </itemgra>
                   <itemgra item_types="poi_public_office" order="12-">
                       <icon src="/sdcard/navit/icons/public_office.xpm"/>
                   </itemgra>
                         <itemgra item_types="traffic_signals" order="13-" zoom="0">
                       <icon src="/sdcard/navit/icons/traffic_signals.png"/>
                   </itemgra> -->
     
           <!-- Clear background -->
                  
                   <!--<itemgra item_types="nav_left_1" order="0-">
                       <icon src="/sdcard/navit/icons/nav_left_1_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_left_2" order="0-">
                       <icon src="/sdcard/navit/icons/nav_left_2_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_left_3" order="0-">
                       <icon src="/sdcard/navit/icons/nav_left_3_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_right_1" order="0-">
                       <icon src="/sdcard/navit/icons/nav_right_1_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_right_2" order="0-">
                       <icon src="/sdcard/navit/icons/nav_right_2_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_right_3" order="0-">
                       <icon src="/sdcard/navit/icons/nav_right_3_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_straight" order="0-">
                       <icon src="/sdcard/navit/icons/nav_straight_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_turnaround_left" order="0-">
                       <icon src="/sdcard/navit/icons/nav_turnaround_left_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_turnaround_right" order="0-">
                       <icon src="/sdcard/navit/icons/nav_turnaround_right_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l1" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l1_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r1" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r1_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l2" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l2_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r2" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r2_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l3" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l3_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r3" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r3_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l4" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l4_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r4" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r4_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l5" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l5_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r5" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r5_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l6" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l6_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r6" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r6_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l7" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l7_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r7" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r7_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_l8" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_l8_bk.svg" w="32" h="32"/>
                   </itemgra>
                   <itemgra item_types="nav_roundabout_r8" order="0-">
                       <icon src="/sdcard/navit/icons/nav_roundabout_r8_bk.svg" w="32" h="32"/>
                   </itemgra>-->
                   <!-- I'm not sure if the following stuff should appear in any layout. Maybe portions should only apply to the bicyle layout. -->
                   </layer>
          <layer name="GC Labels">
           <itemgra item_types="poi_gc_tradi,poi_gc_multi,poi_gc_letterbox,poi_gc_earthcache,poi_gc_virtual,poi_gc_event,poi_gc_puzzle,poi_gc_question,poi_gc_mystery,poi_gc_question,poi_gc_stages,poi_gc_reference,poi_gc_webcam" order="13-">
               <circle color="#606060" radius="0" width="0" text_size="7"/>
           </itemgra>
           </layer>
           <layer name="Important POI Labels">
                   <itemgra item_types="poi_attraction" order="14-">
                       <circle color="#606060" radius="0" width="0" text_size="15"/>
                   </itemgra>
               </layer>
           <layer name="More Important POI Labels">
                   <itemgra item_types="poi_bus_stop,poi_rail_halt,poi_rail_station,poi_rail_tram_stop," order="15-16">
                       <circle color="#606060" radius="0" width="0" text_size="18"/>
                   </itemgra>
               </layer>
               <layer name="High Level POI Labels">
                   <itemgra item_types="poi_pharmacy,poi_cinema,poi_theater,poi_restaurant,poi_zoo,poi_shopping,poi_restaurant,poi_cafe,poi_bar,poi_shop_handg,poi_shop_grocery,poi_shop_apparel" order="17-">
                       <circle color="#000000" radius="0" width="0" text_size="24"/>
                   </itemgra>
               </layer>

               <!--<layer name="Low Level POI Labels">
                   <itemgra item_types="" order="17-">
                       <circle color="#606060" radius="0" width="0" text_size="15"/>
                   </itemgra>
               </layer>-->
               
   <!-- HOTEL and FOOT poi_resort, poi_motel,poi_hotel,poi_restaurant,poi_cafe,poi_bar,poi_fastfood,-->
   <!-- MUSEUMS and ATTRACTIONS poi_museum_history, poi_attraction,poi_theater,poi_zoo,poi_castle, poi_ruins,poi_memorial,poi_monument,poi_viewpoint --> 
   <!-- SHOPPING poi_shop_grocery,poi_shopping,  -->   
   <!-- SECURITY poi_hospital,poi_police,poi_pharmacy-->
   <!-- ELSE poi_airport,highway_exit,poi_school,poi_church,poi_repair_service, poi_bank,poi_sport,poi_stadium,poi_swimmingpoi_shelter-->>
               
           
           <!-- TOWN LABELS -->
           <layer name="labels">
           <itemgra item_types="house_number" order="16-">
               <circle color="#777777" radius="3" text_size="24"/>
           </itemgra>
                   <itemgra item_types="town_label,district_label,town_label_0e0,town_label_1e0,town_label_2e0,town_label_5e0,town_label_1e1,town_label_2e1,town_label_5e1,town_label_1e2,town_label_2e2,town_label_5e2,district_label_0e0,district_label_1e0,district_label_2e0,district_label_5e0,district_label_1e1,district_label_2e1,district_label_5e1,district_label_1e2,district_label_2e2,district_label_5e2" order="11-">
                       <circle color="#000000" radius="3" text_size="30"/>
                   </itemgra>
                   <itemgra item_types="district_label_1e3,district_label_2e3,district_label_5e3" order="11-">
                       <circle color="#000000" radius="3" text_size="30"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e3,town_label_2e3,town_label_5e3" order="9">
                       <circle color="#000000" radius="3" text_size="20"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e3,town_label_2e3,town_label_5e3" order="10">
                       <circle color="#000000" radius="3" text_size="30"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e3,town_label_2e3,town_label_5e3" order="11-">
                       <circle color="#000000" radius="3" text_size="45"/>
                   </itemgra>
                   <itemgra item_types="district_label_1e4,district_label_2e4,district_label_5e4" order="9-">
                       <circle color="#000000" radius="3" text_size="30"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e4,town_label_2e4,town_label_5e4" order="7-9">
                       <circle color="#000000" radius="3" text_size="30"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e4,town_label_2e4,town_label_5e4" order="10-">
                       <circle color="#000000" radius="3" text_size="45"/>
                   </itemgra>
                                            
                   <itemgra item_types="district_label_1e5,district_label_2e5,district_label_5e5" order="8-">
                       <circle color="#000000" radius="3" text_size="40"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e5,town_label_2e5,town_label_5e5" order="3-">
                       <circle color="#000000" radius="3" text_size="50"/>
                   </itemgra>
                   <itemgra item_types="district_label_1e6,district_label_2e6,district_label_5e6" order="1-">
                       <circle color="#000000" radius="3" text_size="70"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e6,town_label_2e6,town_label_5e6" order="1-">
                       <circle color="#000000" radius="3" text_size="70"/>
                   </itemgra>
                   <itemgra item_types="town_label_1e7,district_label_1e7" order="1-">
                       <circle color="#000000" radius="3" text_size="70"/>
                   </itemgra>
           
               </layer>
           </layout>
