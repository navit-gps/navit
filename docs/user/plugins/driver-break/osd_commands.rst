Driver Break OSD commands
=========================

The Driver Break plugin registers Navit **commands** when its OSD is loaded. You can bind them from ``<osd>`` elements (for example ``type="text"`` with a ``command`` attribute) using the **navit** object as the call context.

Invocation
----------

- Use the form ``navit.<command_name>(...)`` in your OSD ``command`` string, for example::

      command="navit.driver_break_suggest_stop()"

- Commands are registered after the plugin OSD initializes. Ensure ``<osd type="driver_break" .../>`` is present and enabled in ``navit.xml`` so the plugin instance exists; otherwise handlers may report that the plugin is not loaded.

- Several actions open **internal GUI** dialogs (gui_internal). On platforms without that GUI (for example Android builds), those commands fall back to user messages or simplified behaviour where implemented.

- Arguments shown in parentheses are passed through Navit's command layer as the handler **function** string (first parenthesized token). For commands that expect a keyword, that string is matched case-insensitively (for example ``car``, ``kinetic``).

Main plugin commands
--------------------

These names are defined in ``driver_break_osd.c`` (``driver_break_commands[]``).

.. list-table::
   :header-rows: 1
   :widths: 28 22 50

   * - Command
     - Arguments
     - Description
   * - ``driver_break_suggest_stop``
     - None
     - Compute or reuse rest-stop suggestions along the active route and show the suggestions dialog. Requires a route and internal GUI.
   * - ``driver_break_show_history``
     - None
     - Show rest-stop history from the SQLite database. Requires database and internal GUI.
   * - ``driver_break_configure``
     - None
     - Reserved placeholder; currently succeeds without opening a menu.
   * - ``driver_break_start_break``
     - None
     - Mark the start of a rest break (timestamp and position when available).
   * - ``driver_break_end_break``
     - None
     - End the current break, append a history entry, and clear the active break state.
   * - ``driver_break_configure_intervals``
     - None
     - Open the break-interval configuration dialog for the **current plugin travel mode** (car, truck, hiking, cycling, or motorcycle). Requires internal GUI.
   * - ``driver_break_configure_overnight``
     - None
     - Open overnight / POI-radius / related settings for the current travel mode. Requires internal GUI.
   * - ``driver_break_open_settings``
     - None
     - Set Navit's ``osd_configuration`` attribute to the Driver Break **settings layer bitmask** for the active travel mode so layered OSD menus show the right page (see :ref:`example-osd-navit-xml`).
   * - ``driver_break_set_mode``
     - One of: ``car``, ``truck``, ``hiking``, ``cycling``, ``motorcycle``
     - Set the plugin travel mode and persist it to the database. Example: ``navit.driver_break_set_mode(hiking)``.
   * - ``driver_break_toggle``
     - ``kinetic`` or ``eco``
     - Toggle **energy routing** (``kinetic`` / ``use_energy_routing``) or **ECU-weighted route cost** (``eco`` / ``use_ecu_route_cost``), and save to the database.
   * - ``driver_break_set_fuel_level``
     - Numeric literal (litres)
     - Set the current fuel level used for range estimates. Example: ``navit.driver_break_set_fuel_level(32.5)``.
   * - ``driver_break_log_fuel_stop``
     - Optional numeric literal (litres added)
     - Log a fuel stop at the current vehicle position. If a value is given, it is added to the current level (capped by tank capacity). With no argument, logs a stop with zero litres added.
   * - ``driver_break_configure_fuel``
     - None
     - Open the fuel configuration summary / setup dialog (tank, consumption, ECU options where applicable). Requires internal GUI.
   * - ``driver_break_set_drag_cd``
     - Numeric literal (dimensionless)
     - Set the aerodynamic **drag coefficient** ``Cd`` for the kinetic energy model (range 0.01 to 1.5). Persisted to the database. Example: ``navit.driver_break_set_drag_cd(0.29)``.
   * - ``driver_break_set_frontal_area``
     - Numeric literal (square metres)
     - Set **frontal area** in square metres; with ``Cd`` this defines air drag as ``0.5 * rho * Cd * A * v^2`` at sea-level air density inside the model (range 0.05 to 20). Example: ``navit.driver_break_set_frontal_area(2.15)``.
   * - ``driver_break_set_total_weight``
     - Numeric literal (kilograms)
     - Set **total mass** for the energy model (rolling resistance and elevation work; range 1 to 50000 kg). Example: ``navit.driver_break_set_total_weight(1650)``.

Legacy command aliases
----------------------

For backward compatibility, the following names call the same handlers as the ``driver_break_*`` commands in the left column:

.. list-table::
   :header-rows: 1
   :widths: 35 35

   * - Alias
     - Equivalent to
   * - ``rest_suggest_stop``
     - ``driver_break_suggest_stop``
   * - ``rest_show_history``
     - ``driver_break_show_history``
   * - ``rest_start_break``
     - ``driver_break_start_break``
   * - ``rest_end_break``
     - ``driver_break_end_break``
   * - ``rest_configure_intervals``
     - ``driver_break_configure_intervals``
   * - ``rest_configure_overnight``
     - ``driver_break_configure_overnight``

SRTM / elevation commands
-------------------------

Registered from ``driver_break_srtm_osd.c`` when the plugin loads.

.. list-table::
   :header-rows: 1
   :widths: 28 22 50

   * - Command
     - Arguments
     - Description
   * - ``srtm_download_menu``
     - None
     - Open the SRTM download manager (list of predefined regions) when internal GUI is available; otherwise print a short summary message.
   * - ``srtm_download_region``
     - Optional ``name`` (see below)
     - If the invocation supplies a **name** attribute matching a predefined region (for example ``new name("Sweden")`` in the command expression), starts an asynchronous download for that region (requires libcurl). If no name is given, behaves like opening the region picker when the internal GUI is available.
   * - ``srtm_get_elevation``
     - Optional coordinate
     - Query elevation at the given ``position_coord_geo`` when provided in the command attribute list; otherwise uses the current vehicle position. Shows a dialog (internal GUI) or a user message with the result. When output attributes are requested by the caller, may supply ``attr_height`` with the elevation value. This command **only reads** elevation from tiles already on disk (GeoTIFF or HGT per 1-degree tile); it does **not** download missing tiles.

How elevation downloads are scoped
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Downloads are driven **only** by OSD (or other) commands and by **fixed bounding boxes** attached to each predefined region name in the plugin. The plugin does **not** infer coverage from the loaded map, the active route, the current map view, or vehicle position. To get tiles for an area, pick a region (or open the manager / picker) and start a download explicitly.

Predefined region names are matched **case-sensitively** (same spelling as in the manager). The built-in list includes: **Europe**, **Germany**, **France**, **United Kingdom**, **Italy**, **Spain**, **Norway**, **Sweden**, **Poland**, **United States**, **China**, **Japan**.

From an OSD ``command`` attribute, pass a region name with Navit's ``new name("...")`` so the handler receives ``attr_name``. Example (XML-escaped quotes inside the attribute value)::

  command="navit.srtm_download_region(new name(&quot;Sweden&quot;))"

You can also wrap the attribute value in single quotes in XML if that is easier than escaping double quotes.

Where Copernicus GeoTIFF and SRTM HGT sources are enabled in the build, the downloader tries Copernicus first where applicable, then falls back to HGT; **libcurl** is required for HTTP downloads, and **GeoTIFF** support is needed to read Copernicus ``.tif`` tiles after download.

See also
--------

- :ref:`example-osd-navit-xml` and the downloadable example ``navit_driver_break_osd_example.xml``.
- :doc:`osd_example_menu_tree` for an ASCII map of menu rows and related flags.
- Navit's own documentation for the ``<osd>`` **command** attribute and expression syntax if you need to combine commands with attribute assignments (for example ``osd_configuration``).
