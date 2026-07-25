###################
Navit's User Manual
###################

What is Navit?
##############

Navit is a open source (GPL) car navigation system with routing engine.

It's modular design is capable of using vector maps of various formats for routing and rendering of the displayed map. It's even possible to use multiple maps at a time.

The user interfaces are designed to work well with touch screen displays. Points of Interest of various formats are displayed on the map.

The current vehicle position is either read from gpsd or directly from NMEA GPS sensors.

The routing engine not only calculates an optimal route to your destination, but also generates directions and even speaks to you.

Navit currently speaks over 70 languages!

Navit is highly customizable, from map layouts and on-screen display to the details of the routing engine.

Main Features
-------------

.. image:: navit.png

.. toctree::
   :hidden:
   :glob:
   :numbered:
   :maxdepth: 2
   :caption: Navit User Manual

   user/getting_started
   user/platforms/index
   user/configuration/basic
   user/configuration/advanced
   user/plugins/index
   user/community/index
   user/faq/index

.. __: https://github.com/navit-gps/navit/

.. toctree::
   :hidden:
   :maxdepth: 2
   :glob:
   :caption: Navit Developer Documentation

   development/index
   development/targets
   development/concepts
   development/guides
   development/programming_guidelines
   development/commit_guidelines
   development/changelog_wrapper

.. toctree::
   :hidden:
   :maxdepth: 1
   :glob:
   :caption: Other Documents

   team
   privacy

.. toctree::
   :hidden:
   :maxdepth: 1
   :glob:
   :caption: Old Wiki

   user/community/Main_Page_support_box
   user/configuration/Configuration_(old)
   user/configuration/Configuration_Display_Options
   user/configuration/Configuration_Full_list_of_options
   user/configuration/Configuration_General_Options
   user/configuration/Configuration_Layout_Options
   user/configuration/Configuration_Maps_Options
   user/configuration/Configuration_Vehicle_Options
   user/configuration/Coordinate_format
   user/configuration/Gui_internal
   user/configuration/Icons
   user/configuration/Internal_GUI
   user/configuration/Internal_GUI_Menu_configurations
   user/configuration/Layout
   user/configuration/Menu
   user/configuration/NavitConfigurator
   user/configuration/OSD
   user/configuration/OSD_Layouts
   user/configuration/QML_GUI
   user/configuration/QML2_GUI
   user/configuration/QML2_GUI_WIP
   user/configuration/Skinning_the_SDL_GUI
   user/configuration/Speech
   user/configuration/Vehicle_profile_flags
   user/configuration/Vehicleprofile
   user/configuration/vehicleprofile/vehicleprofile_bike_cycleway
   user/configuration/vehicleprofile/vehicleprofile_bike_on_asphalt
   user/configuration/vehicleprofile/vehicleprofile_car_no_highway
   user/configuration/vehicleprofile/vehicleprofile_hike_bike_hard
   user/configuration/vehicleprofile/vehicleprofile_hike_bike_on_ground_gravel
   user/configuration/maps/binfile
   user/faq/Failed_to_connect_graphics_to_gui
   user/faq/Gpsd_Troubleshooting
   user/faq/Troubleshooting


.. Indices and tables
.. ------------------

.. * :ref:`genindex`
.. * :ref:`modindex`
.. * :ref:`search`
