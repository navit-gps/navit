.. _driver-break-osd-example-menu-tree:

Driver Break: OSD example menu tree
===================================

This page is an ASCII map of the layered text OSD in ``navit_driver_break_osd_example.xml``.
Each **flag** is the Navit ``osd_configuration`` value that row sets or belongs to.
The entry line stays visible when the menu is closed (``enable_expression="1"`` in XML; the
file header also documents why plain ``osd_configuration="-1"`` is not used for that row).

For the full config, see :download:`navit_driver_break_osd_example.xml` in this directory
or `navit_driver_break_osd_example.xml on GitHub <https://github.com/Supermagnum/navit/blob/feature/driver-break/docs/user/plugins/driver-break/navit_driver_break_osd_example.xml>`__.

Rows that call ``navit.driver_break_configure_intervals()`` or ``navit.driver_break_configure_overnight()``
open the plugin's combined internal-GUI dialogs; several labels map to the same command.

For **cycling** rest stops, ``process_cycling_stops()`` also loads **cycling service POIs** (map: bike shop, repair, rental, parking; Overpass: ``amenity=charging_station`` for e-bikes, ``shop=bicycle``, ``amenity=bicycle_repair_station`` when libcurl is available) within the **general POI search radius** from Configure overnight, in addition to water and cabin radii on the same dialog.

.. code-block:: text

    Break Plugin  (always visible; opens flag 1)
    |
    +-- Main menu  (flag 1)
        |
        +-- Travel modes  (flag 2)
        |       Travel mode  (title row; stays on flag 2)
        |       Back  -> flag 1
        |       Car         -> set mode car, flag 1
        |       Truck       -> set mode truck, flag 1
        |       Hiking      -> set mode hiking, flag 1
        |       Cycling     -> set mode cycling, flag 1
        |       Motorcycle  -> set mode motorcycle, flag 1
        |
        +-- Break settings  (driver_break_open_settings() -> 4, 8, 16, 32, or 128 by active mode)
        |   |
        |   +-- Car  (flag 4)
        |   |       Soft limit hours           -> configure_intervals()
        |   |       Max drive hours            -> configure_intervals()
        |   |       Break interval (h)         -> configure_intervals()
        |   |       Break duration (min)       -> configure_intervals()
        |   |       POI: cafe/rest...          -> configure_intervals()
        |   |       Back  -> flag 1
        |   |
        |   +-- Truck  (flag 16)
        |   |       Break after (h, EU)        -> configure_intervals()
        |   |       Break duration (min)       -> configure_intervals()
        |   |       Max daily drive (h)        -> configure_intervals()
        |   |       POI: truck stops           -> configure_intervals()
        |   |       Back  -> flag 1
        |   |
        |   +-- Hiking page 1  (flag 8)
        |   |       Main break (km)            -> configure_intervals()
        |   |       Alt break (km)             -> configure_intervals()
        |   |       Max daily (km)             -> configure_intervals()
        |   |       More hiking settings...    -> flag 256
        |   |       Back  -> flag 1
        |   |
        |   |   Hiking page 2  (flag 256)
        |   |       POI: water (km)            -> configure_overnight()
        |   |       POI: cabins (km)           -> configure_overnight()
        |   |       POI: network huts          -> configure_overnight()
        |   |       DNT / network prio         -> configure_intervals()
        |   |       Pilgrimage / hiking        -> configure_intervals()
        |   |       Overnight vs buildings     -> configure_overnight()
        |   |       Overnight vs glaciers      -> configure_overnight()
        |   |       Back  -> flag 8
        |   |
        |   +-- Cycling  (flag 32)
        |   |       Main break (km)            -> configure_intervals()
        |   |       Alt break (km)             -> configure_intervals()
        |   |       Max daily (km)             -> configure_intervals()
        |   |       POI: water (km)            -> configure_overnight()
        |   |       POI: cabins (km)           -> configure_overnight()
        |   |       POI search radius (km)   -> configure_overnight()
        |   |        (general radius for cycling service POIs: charging/e-bikes, shop, repair, etc.)
        |   |       Back  -> flag 1
        |   |
        |   +-- Motorcycle page 1  (flag 128)
        |   |       Soft limit (time)          -> configure_intervals()
        |   |       Max ride (time)            -> configure_intervals()
        |   |       Break interval             -> configure_intervals()
        |   |       Break duration             -> configure_intervals()
        |   |       More motorcycle settings... -> flag 512
        |   |       Back  -> flag 1
        |   |
        |   |   Motorcycle page 2  (flag 512)
        |   |       POI: scenic / fuel...      -> configure_intervals()
        |   |       POI: cabins / tour         -> configure_overnight()
        |   |       Overnight vs buildings     -> configure_overnight()
        |   |       Overnight vs glaciers      -> configure_overnight()
        |   |       Scenic / tour routes       -> configure_intervals()
        |   |       Back  -> flag 128
        |
        +-- Routing mode  (flag 64)
        |       Kinetic (toggle)               -> driver_break_toggle(kinetic); labels static in XML
        |       Eco / ECU (toggle)             -> driver_break_toggle(eco); labels static in XML
        |       Back  -> flag 1
        |
        +-- Elevation data  (flag 1024)
        |       Elevation  (title row; stays on flag 1024)
        |       Back  -> flag 1
        |       Download regions             -> srtm_download_menu()
        |       Pick region                  -> srtm_download_region()
        |       Download Norway              -> srtm_download_region(new name("Norway"))
        |       Download Sweden              -> srtm_download_region(new name("Sweden"))
        |       Download Germany             -> srtm_download_region(new name("Germany"))
        |       Elevation here               -> srtm_get_elevation()
        |
        +-- Close  -> flag 0
