.. _internal_guimenu_configurations:

Internal GUI/Menu configurations
================================

The menu used in the `Internal GUI <Internal_GUI>`__ is defined by a
html-like syntax inside the ... tags within navit.xml. As a result, the
menu offers a variety of configuration options to better suite the user.

.. _installing_alternative_configurations:

Installing Alternative Configurations
=====================================

If you want to try one of the configurations shown below, simply copy
the code for the configuration (including the leading and trailing
``]]>``) and paste over the top of the default configuration within the
... tags in ``navit.xml``. Make sure that all traces of a previous
configuration are wiped out, as only one configuration can be present in
``navit.xml`` at one time.

.. _default_configuration:

Default Configuration
=====================

Below is the default menu configuration found in a freshly-installed
version of navit.xml.

::

   <![CDATA[
   <html>
       <a name='Main Menu'><text>Main menu</text>
            <img cond='button' src='gui_map'><script>position(click_coord_geo,_("Map Point"),8|16|32|64|256|1024)</script></img>
            <a href='#Actions'><img src='gui_actions'>Actions</img></a>
           <img cond='flags&amp;2' src='gui_map' onclick='back_to_map()'><text>Show Map</text></img>
           <a href='#Settings'><img src='gui_settings'><text>Settings</text></img></a>
           <a href='#Tools'><img src='gui_tools'><text>Tools</text></img></a>
           <a href='#Route'><img src='gui_settings'><text>Route</text></img></a>
                   <img src='gui_about'  onclick='about()'><text>About</text></img>
       </a>
               
       <a name='Actions'><text>Actions</text>
           <img src='gui_bookmark' onclick='bookmarks()'><text>Bookmarks</text></img>
           <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256)'>
                   <script>write(click_coord_geo)</script></img>
           <img cond='position_coord_geo' src='gui_vehicle' onclick='position(position_coord_geo,_("Vehicle Position"),8|32|64|128|256)'>
                   <script>write(position_coord_geo)</script></img>
           <img src='gui_town' onclick='town()'><text>Town</text></img>
           <img src='gui_quit' onclick='quit()'><text>Quit</text></img>
           <img cond='navit.route.route_status&amp;52' src='gui_stop' onclick='abort_navigation()'><text>Stop Navigation</text></img>
       </a>

       <a name='Settings'><text>Settings</text>
           <a href='#Settings Display'><img src='gui_display'><text>Display</text></img></a>
           <img src='gui_maps' onclick='setting_maps()'><text>Maps</text></img>
           <img src='gui_vehicle' onclick='setting_vehicle()'><text>Vehicle</text></img>
           <img src='gui_rules' onclick='setting_rules()'><text>Rules</text></img>
       </a>

       <a name='Settings Display'><text>Display</text>
           <img src='gui_display' onclick='setting_layout()'><text>Layout</text></img>
           <img cond='fullscreen==0' src='gui_fullscreen' onclick='fullscreen=1'><text>Fullscreen</text></img>
           <img cond='fullscreen==1' src='gui_leave_fullscreen' onclick='fullscreen=0'><text>Window Mode</text></img>
           <img cond='navit.pitch==0' src='gui_map' onclick='navit.pitch=pitch;redraw_map();back_to_map()'><text>3D</text></img>
           <img cond='navit.pitch!=0' src='gui_map' onclick='navit.pitch=0;redraw_map();back_to_map()'><text>2D</text></img>
       </a>

       <a name='Tools'><text>Tools</text>
           <img src='gui_actions' onclick='locale()'><text>Show Locale</text></img>
       </a>

       <a name='Route'><text>Route</text>
           <img src='gui_actions' onclick='route_description()'><text>Description</text></img>
           <img src='gui_actions' onclick='route_height_profile()'><text>Height Profile</text></img>
       </a>

   </html>
   ]]>

.. _alternative_configurations:

Alternative Configurations
==========================

.. _netbook_configuration_1:

Netbook Configuration 1
-----------------------

`right|300px|text-topFeatures <image:Screenshot-Navit-NetbookMenu.png>`__:

-  `Map Point <Internal_GUI#Map_Point>`__ is in the main menu. It also
   appears in the "Go To" menu.
-  `Actions <Internal_GUI#Actions>`__ has been renamed "Go To", and
   given a different icon.
-  "Exit" (old "Quit") has moved to the Main Menu.
-  `Tools <Internal_GUI#Tools>`__ and `About <Internal_GUI#About>`__
   have been removed.

| 

::

           <img cond='flags&amp;2' src='gui_map' onclick='back_to_map()'><text>Show Map</text></img>       
           <!-- Main Menu -->
           <a name='Main Menu'><text>Main menu</text>

               <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256|1024)'>Map Point</img>
               <a href='#Actions'><img src='cursor'>Go To</img></a>
               <a href='#Settings'><img src='gui_settings'><text>Settings</text></img></a>
               <a href='#Route'><img src='gui_log'><text>Route Info</text></img></a>
               <img src='gui_quit' onclick='quit()'><text>Exit</text></img>
               <img cond='navit.route.route_status&amp;amp;52' src='gui_stop' onclick='abort_navigation()'><text>Stop Navigation</text></img>
           
           </a>
           
           <!-- Actions -->
           <a name='Actions'><text>Go To</text>

               <img src='gui_bookmark' onclick='bookmarks()'><text>Bookmarks</text></img>
               <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256|1024)'><script>write(click_coord_geo)</script></img>
               <img cond='position_coord_geo' src='gui_vehicle' onclick='position(position_coord_geo,_("Vehicle Position"),8|32|64|128|256)'><script>write(position_coord_geo)</script></img>
               <img src='gui_town' onclick='town()'><text>Town</text></img>
               
           </a>

           <!-- Settings -->
           <a name='Settings'><text>Settings</text>
               <a href='#Settings Display'><img src='gui_display'><text>Display</text></img></a>
               <img src='gui_maps' onclick='setting_maps()'><text>Maps</text></img>
               <img src='gui_vehicle' onclick='setting_vehicle()'><text>Vehicle</text></img>
               <img src='gui_rules' onclick='setting_rules()'><text>Rules</text></img>
           </a>

           <!-- Display -->
           <a name='Settings Display'><text>Display</text>
               <img src='gui_display' onclick='setting_layout()'><text>Layout</text></img>
               <img cond='fullscreen==0' src='gui_fullscreen' onclick='fullscreen=1'><text>Fullscreen</text></img>
               <img cond='fullscreen==1' src='gui_leave_fullscreen' onclick='fullscreen=0'><text>Window Mode</text></img>
               <img cond='navit.pitch==0' src='gui_map' onclick='navit.pitch=pitch;redraw_map();back_to_map()'><text>3D</text></img>
               <img cond='navit.pitch!=0' src='gui_map' onclick='navit.pitch=0;redraw_map();back_to_map()'><text>2D</text></img>
           </a>
           
           <!-- Route -->
           <a name='Route'><text>Route Information</text>
               <img src='gui_log' onclick='route_description()'><text>Description</text></img>
               <img src='gui_log' onclick='route_height_profile()'><text>Height Profile</text></img>
           </a>
       </html>
   ]]>

.. _wvga_configuration_1:

WVGA Configuration 1
--------------------

Features of this menu:

-  Main menu:

   -  Actions
   -  Settings
   -  Route (if a route is active)
   -  Quit

-  Actions submenu:

   -  Bookmarks
   -  Town selection
   -  GPS position
   -  Vehicle position

-  Settings submenu:

   -  Fullscreen yes/no
   -  Map Selection
   -  3D/2D
   -  About

-  Route submenu:

   -  Vehicle Selection
   -  Route Description
   -  Route Height Profile
   -  Stop Navigation (if a route is active)

::

   <![CDATA[
   <html>
       <a name='Main Menu'><text>Main menu</text>
           <a href='#Actions'>   <img src='gui_actions'>                    <text>Actions</text></img></a>
           <a href='#Settings'>  <img src='gui_rules'>                      <text>Settings</text></img></a>
                                         <img src='gui_quit'    onclick='quit()'>   <text>Quit</text></img>
                   <a cond='navit.route.route_status&amp;amp;52' href='#Route'>
                                         <img src='gui_vehicle'>                    <text>Route</text></img></a>
       </a>
               
       <a name='Actions'><text>Actions</text>
                   <img src='gui_bookmark'                          onclick='bookmarks()'> <text>Bookmarks</text></img>
                   <img src='gui_town'                              onclick='town()'>      <text>Town</text></img>
                   <img cond='click_coord_geo'    src='gui_map'     onclick='position(click_coord_geo,_("Map Point"),8|32|48|128)'>            <script>write(click_coord_geo)</script> </img>
                   <img cond='position_coord_geo' src='gui_vehicle' onclick='position(position_coord_geo,_("Vehicle Position"),8|32|48|128)'>  <script>write(position_coord_geo)</script> </img>
       </a>
               
       <a name='Settings'><text>Settings</text>
                   <img cond='fullscreen==0'  src='gui_fullscreen'       onclick='fullscreen=1'>      <text>Fullscreen</text></img>
                   <img cond='fullscreen==1'  src='gui_leave_fullscreen' onclick='fullscreen=0'>      <text>Window Mode</text></img>
                   <img                       src='gui_maps'             onclick='setting_maps()'>    <text>Maps</text></img>
                   
                   <img cond='navit.pitch==0' src='gui_map'              onclick='navit.pitch=24;  redraw_map();back_to_map()'> <text>3D</text> </img>
                   <img cond='navit.pitch!=0' src='gui_map'              onclick='navit.pitch=0;   redraw_map();back_to_map()'> <text>2D</text> </img>
                   <img                       src='gui_about'            onclick='about()'>           <text>About</text></img>
       </a>

       <a name='Route'><text>Route</text>
                   <img src='gui_vehicle_pedestrian'   onclick='setting_vehicle();back()'>          <text>Vehicle</text></img>
                 
                   <img src='gui_town'     onclick='route_description()'>                           <text>Description</text></img>
                   <img src='gui_zoom_in'  onclick='route_height_profile()'>                        <text>Height Profile</text></img>
                   <img src='gui_stop'     onclick='abort_navigation();redraw_map();back_to_map()'> <text>Stop Navigation</text></img>
       </a>
   </html>
   ]]>

.. _qvga_square_240x240_configuration_1_german:

QVGA Square (240x240) Configuration 1 (German)
----------------------------------------------

**Features**:

-  optimization for faster handling (e.g. all the "main actions" are in
   the first screen)
-  reduced numbers of screens (= menus)
-  included some additional features (e.g. autozoom, fixed-to-north,
   fixed-to-street, ...)
-  deleted some not (yet) used features (e.g. "Height Profile", ...)

Example: With the first tap on the screen you got the Main-Menu. Second
tap on bookmarks (1a), third tap on wanted target and forth tab on "set
as target" you can set up for navigation with in 4 taps at all. Thats
pretty fast.

**The three screens** (= menus):

-  1.) Main => select target to navigate or doing something with a
   position (actual GPS or Mappoint)
-  2.) Tuning => most used setups while navigating
-  3.) Setup => rarely used (system)settings

| 

`right|240px|text-top|Main-Menu <image:Screenshot-Navit-QVGA-Main-Menu.png>`__

In the **Main-Menu** you can:

-  1a) Do something with your bookmarks (e.g. navigate to a bookmark)
-  1b) Do something with an adress (e.g. navigate to an adress)
-  1c) Do something with your "map-position" (e.g. save as a bookmark,
   navigate to, search POIs nearby, ...)
-  1d) Do something with your "GPS-position" (e.g. save as a bookmark,
   navigate to, search POIs nearby, ...)
-  1e) Exit navit
-  1f) Go to the next menu "Tuning"

| 
| `right|240px|text-top|Tuning-Menu <image:Screenshot-Navit-QVGA-Tuning-Menu.png>`__

In the **Tuning-Menu** you can:

-  2a) Toggle map fixed to north on/off (my default is "fixed to north
   off")
-  2b) Toggle "autozoom" on/off (my default is "autozoom on")
-  2c) Toggle GPS-position fixed to street on/off (my default is "fixed
   to street on")
-  2d) Toggle fullscreen on/off (my default is "fullscreen on")
-  2e) Toggle view 3d-view on/off (my default is "3d-view off")
-  2f) Go to the next menu "Setup"

| (...all default values are defined and could be changed in the
  "navit.xml")
| `right|240px|text-top|Setup-Menu <image:Screenshot-Navit-QVGA-Setup-Menu.png>`__

In the **Setup-Menu** you can:

-  3a) Call the "vehicle-menu" (e.g. there you can change your actual
   vehicle-profile)
-  3b) Call the "map-menu" (e.g. there you can change your current map)
-  3c) Call the "about-site"
-  3d) Delete current navigaition-route (...only visible while
   navigating)

| 

::

   <![CDATA[

   <html>
   <a name='Main Menu'><text>Main menu</text>
       <img src='gui_bookmark' onclick='bookmarks()'><text>Bookmarks</text></img>
       <img src='gui_town' onclick='town()'><text>Town</text></img>
       <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256)'><script>write(click_coord_geo)</script></img>
       <img cond='position_coord_geo' src='gui_vehicle' onclick='position(position_coord_geo,_("Vehicle Position"),8|32|64|128|256)'><script>write(position_coord_geo)</script></img>
       <img src='gui_quit' onclick='quit()'><text>Quit</text></img>
       <a href='#Tuning'><img src='gui_arrow_right'><text>Tuning</text></img></a>
   </a>



   <a name='Tuning'><text>Tuning</text>
       <img cond='navit.orientation<0' src='gui_stop' onclick='navit.orientation=0;redraw_map();back_to_map()'><text>Norden</text></img>
       <img cond='navit.orientation>=0' src='gui_active' onclick='navit.orientation=-1;redraw_map();back_to_map()'><text>Norden</text></img>

       <img cond='navit.autozoom_active!=0' src='gui_active' onclick='navit.autozoom_active=0;redraw_map();back_to_map()'><text>AutoZoom</text></img>
       <img cond='navit.autozoom_active==0' src='gui_stop' onclick='navit.autozoom_active=1;redraw_map();back_to_map()'><text>AutoZoom</text></img>

       <img cond='navit.tracking==1' src='gui_active' onclick='navit.tracking=0;redraw_map();back_to_map()'><text>"Auf Strasse"</text></img>
       <img cond='navit.tracking==0' src='gui_stop' onclick='navit.tracking=1;redraw_map();back_to_map()'><text>"Auf Strasse"</text></img>

       <img cond='fullscreen==1' src='gui_active' onclick='fullscreen=0;redraw_map();back_to_map()'><text>Vollbild</text></img>
       <img cond='fullscreen==0' src='gui_stop' onclick='fullscreen=1;redraw_map();back_to_map()'><text>Vollbild</text></img>

       <img cond='navit.pitch==0' src='gui_stop' onclick='navit.pitch=60;redraw_map();back_to_map()'><text>3D</text></img>
       <img cond='navit.pitch!=0' src='gui_active' onclick='navit.pitch=0;redraw_map();back_to_map()'><text>3D</text></img>

       <a href='#Setup'><img src='gui_arrow_right'>Setup</img></a>
   </a>



   <a name='Setup'><text>Setup</text>
       <img src='gui_vehicle' onclick='setting_vehicle()'><text>Vehicle</text></img>
       <img src='gui_maps' onclick='setting_maps()'><text>Karten</text></img>
       <img src='gui_about'  onclick='about()'><text>About</text></img>
       <img cond='navit.route.route_status&amp;52' src='gui_stop' onclick='abort_navigation();redraw_map();back_to_map()'><text>Stop-Route</text></img>
   </a>

   </html>

   ]]>

.. _android_configuration:

Android Configuration
---------------------

This is the configuration I use on my Android Phone. It is similar to
the QVGA Square (240x240) configuration. Placeholders are used for
conditional items when not in use.

::

   <html>
     <a name='Main Menu'>
       <text>Main menu</text>
       <img src='gui_town' onclick='town()'><text>Town</text></img>
       
       <a href='#Actions'><img src='gui_actions'><text>Route</text></img></a>   
       
       <img cond='navit.route.route_status&amp;amp;52' src='gui_actions' onclick='route_description()'><text>Description</text></img>
       <img cond='!(navit.route.route_status&amp;amp;52)' src='heliport'><text></text></img>
       
       <a href='#Settings'><img src='gui_rules'><text>Settings</text></img></a>
       
       <img src='gui_map' onclick='back_to_map()'><text>Show Map</text></img>
       
       <img src='gui_quit' onclick='quit()'><text>Quit</text></img>
     </a>
     
     <a name='Actions'>
       <text>Actions</text>
       <img src='gui_town' onclick='town()'><text>Town</text></img>
       
       <img src='gui_bookmark' onclick='bookmarks()'><text>Bookmarks</text></img>
       
       <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256)'><text>Map 
                                                                                                                         Position</text></img>
       <img cond='!click_coord_geo' src='heliport'><text></text></img>
       
       <img cond='position_coord_geo' src='gui_vehicle' onclick='position(position_coord_geo,_("Vehicle Position"),8|32|64|128|256)'><text>Vehicle 
                                                                                                                                           Position</text></img>
       <img cond='!position_coord_geo' src='heliport'><text></text></img>
       
       <a cond='navit.route.route_status&amp;amp;52' href='#Route'><img src='gui_actions' onclick='route_description()'><text>Route 
                                                                                                                          Info</text></img></a>
       <img cond='!(navit.route.route_status&amp;amp;52)' src='heliport'><text></text></img>
       
       <img cond='navit.route.route_status&amp;amp;52' src='gui_stop' onclick='abort_navigation();redraw_map();back_to_map()'><text>Stop 
                                                                                                                                Navigation</text></img>
       <img cond='!(navit.route.route_status&amp;amp;52)' src='heliport'><text></text></img>
     </a>
     
     <a name='Settings'>
       <text>Settings</text>
       <img cond='navit.pitch==0' src='gui_stop' onclick='navit.pitch=60;redraw_map();back_to_map()'><text>3D</text></img>
       <img cond='navit.pitch!=0' src='gui_active' onclick='navit.pitch=0;redraw_map();back_to_map()'><text>3D</text></img>
       
       <img cond='navit.orientation==-1' src='gui_stop' onclick='navit.orientation=0;redraw_map();back_to_map()'><text>Einnorden</text></img>
       <img cond='navit.orientation>=0' src='gui_active' onclick='navit.orientation=-1;redraw_map();back_to_map()'><text>Einnorden</text></img>
       
       <img cond='navit.autozoom_active!=0' src='gui_active' onclick='navit.autozoom_active=0;redraw_map();back_to_map()'><text>AutoZoom</text></img>
       <img cond='navit.autozoom_active==0' src='gui_stop' onclick='navit.autozoom_active=1;redraw_map();back_to_map()'><text>AutoZoom</text></img>
       
       <img cond='navit.tracking==1' src='gui_active' onclick='navit.tracking=0;redraw_map();back_to_map()'><text>Map
                                                                                                                  Tracking</text></img>
       <img cond='navit.tracking==0' src='gui_stop' onclick='navit.tracking=1;redraw_map();back_to_map()'><text>Map
                                                                                                                Tracking</text></img>
       
       <a href='#Main Menu'><img src='gui_arrow_left'><text>Back</text></img></a>
       
       <a href='#Settings Display'><img src='gui_arrow_right'><text>More</text></img></a>
     </a>
     
     <a name='Settings Display'>
       <text>More Settings</text>
       <img src='gui_vehicle' onclick='setting_vehicle()'><text>Vehicle</text></img>
       
       <img src='gui_maps' onclick='setting_maps()'><text>Maps</text></img>
       
       <img src='gui_display' onclick='setting_layout()'><text>Display
                                                               Layout</text></img>
       
       <img src='gui_about'  onclick='about()'><text>About</text></img>
       
       <a href='#Settings'><img src='gui_arrow_left'> <text>Back</text></img></a>
     </a>
     
     <a name='Tools'>
       <text>Tools</text>
       <img src='gui_actions' onclick='locale()'><text>Show Locale</text></img>
     </a>
     
     <a name='Route'>
       <text>Route</text>
       <img src='gui_actions' onclick='route_description()'><text>Description</text></img>
       
       <img src='gui_actions' onclick='route_height_profile()'><text>Height Profile</text></img>
     </a>
   </html>

.. _at_android_gui:

0606.at Android GUI
-------------------

The Internal GUI configurations which is used in the 0606.at Android
Layout (only the font-size have to be set to 466 instead of 700 for MDPI
Versions)

See also here: `OSD_Layouts#0606.at Android
Layout <OSD_Layouts#0606.at_Android_Layout>`__

::

   <gui type="internal" enabled="yes" font_size="700" ><![CDATA[
                   <html>
                   <a name='Main Menu'><text>Main menu</text>
             <img src='gui_town' onclick='town()'><text>Town</text></img>
             <img cond='click_coord_geo' src='gui_map' onclick='position(click_coord_geo,_("Map Point"),8|16|32|64|256)'><script>write(click_coord_geo)</script></img>
                       <img src='gui_bookmark' onclick='bookmarks()'><text>Bookmarks</text></img>
                       <img cond='flags&amp;2' src='gui_map' onclick='back_to_map()'><text>Show Map</text></img>
                       <a href='#Settings'><img src='gui_settings'><text>Settings</text></img></a>
             <a href='#Route'><img src='gui_settings'><text>Route</text></img></a>      
             <img src='gui_quit' onclick='quit()'><text>Quit</text></img>
                   </a>
               <a name='Settings'><text>Settings</text>
                   <a href='#Settings Display'><img src='gui_display'><text>Display</text></img></a>
                   <img src='gui_maps' onclick='setting_maps()'><text>Maps</text></img>
                   <img src='gui_vehicle' onclick='setting_vehicle()'><text>Vehicle</text></img>
                   <img src='gui_rules' onclick='setting_rules()'><text>Rules</text></img>
           <img src='gui_about'  onclick='about()'><text>About</text></img>
           <img cond='navit.orientation<0' src='gui_stop' onclick='navit.orientation=0;redraw_map();back_to_map()'><text>Norden</text></img>
       <img cond='navit.orientation>=0' src='gui_active' onclick='navit.orientation=-1;redraw_map();back_to_map()'><text>Norden</text></img>

       <img cond='navit.autozoom_active!=0' src='gui_active' onclick='navit.autozoom_active=0;redraw_map();back_to_map()'><text>AutoZoom</text></img>
       <img cond='navit.autozoom_active==0' src='gui_stop' onclick='navit.autozoom_active=1;redraw_map();back_to_map()'><text>AutoZoom</text></img>

       <img cond='navit.tracking==1' src='gui_active' onclick='navit.tracking=0;redraw_map();back_to_map()'><text>"Auf Strasse"</text></img>
       <img cond='navit.tracking==0' src='gui_stop' onclick='navit.tracking=1;redraw_map();back_to_map()'><text>"Auf Strasse"</text></img>
           <a href='#Tools'><img src='gui_tools'><text>Tools</text></img></a>
               </a>
               <a name='Settings Display'><text>Display</text>
                   <img src='gui_display' onclick='setting_layout()'><text>Layout</text></img>
                   <img cond='fullscreen==0' src='gui_fullscreen' onclick='fullscreen=1'><text>Fullscreen</text></img>
                   <img cond='fullscreen==1' src='gui_leave_fullscreen' onclick='fullscreen=0'><text>Window Mode</text></img>
               </a>
               <a name='Tools'><text>Tools</text>
                   <img src='gui_actions' onclick='locale()'><text>Show Locale</text></img>
           <img cond='navit.pitch==0' src='gui_map' onclick='navit.pitch=pitch;redraw_map();back_to_map()'><text>3D</text></img>
                   <img cond='navit.pitch!=0' src='gui_map' onclick='navit.pitch=0;redraw_map();back_to_map()'><text>2D</text></img>
               </a>
               <a name='Route'><text>Route</text>
                   <img src='gui_actions' onclick='route_description()'><text>Description</text></img>
                   <img src='gui_actions' onclick='route_height_profile()'><text>Height Profile</text></img>
           <img cond='navit.route.route_status&amp;52' src='gui_stop' onclick='abort_navigation()'><text>Stop
   Navigation</text></img>
               </a>
               </html>
           ]]></gui>
