APRS Symbols Integration
========================

Overview
--------

The APRS plugin integrates with the ``hessu/aprs-symbols`` symbol set for APRS station icons.

Symbol Set
----------

The plugin uses symbols from:

- **hessu/aprs-symbols**: https://github.com/hessu/aprs-symbols
- Created by OH7LZB (aprs.fi author)
- Compatible with Updated APRS Symbol Set (Rev H)

Symbol Format
-------------

APRS symbols are defined by two characters:

- **Symbol Table**: Primary table ``/`` or alternate table ``\``
- **Symbol Code**: Single character indicating the specific icon

Example: ``/`` + ``>`` = Vehicle symbol.

Icon Files
----------

The plugin expects icon files in PNG format at 48x48 pixels, organized as::

   /usr/share/navit/aprs-symbols/48x48/primary/
     vehicle.png
     car.png
     bike.png
     ...

Installation
------------

Symbol PNGs are not committed to the repository. They are generated on demand
from the ``hessu/aprs-symbols`` sprite sheets, which keeps the repository small.

Generate Symbols Before Building
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run the helper script, which clones ``hessu/aprs-symbols`` and extracts the
individual 48x48 PNGs (requires Python 3 with Pillow and PyYAML):

.. code-block:: bash

   cd navit/plugin/aprs/symbols
   ./update_symbols.sh

This populates ``navit/plugin/aprs/symbols/48x48/primary/`` and
``.../48x48/alternate/``.

Build-Time Symbol Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once ``48x48/`` exists, the build system will:

- Detect symbols in ``navit/plugin/aprs/symbols/48x48/``.
- Install them to ``${CMAKE_INSTALL_PREFIX}/share/navit/aprs-symbols/48x48/``.
- Make them available to the plugin at runtime.

If ``48x48/`` is absent, the build still succeeds; symbol icons simply are not
installed and the plugin falls back to the default POI icon at runtime.

Configuration
-------------

The symbol lookup system is initialized automatically when the APRS plugin loads. The plugin automatically detects the symbol installation path using Navit's standard directories.

Symbol Lookup
-------------

The plugin includes a built-in lookup table for common APRS symbols. The lookup function:

- Maps ``symbol_table`` + ``symbol_code`` to an icon filename.
- Returns the icon path relative to Navit's icon search path.
- Returns ``NULL`` if symbol not found.

Supported Symbols
-----------------

The plugin supports symbols from:

- Primary symbol table (``/``): ~95 symbols.
- Alternate symbol table (``\``): ~95 symbols.

Common symbols include:

- ``/`` + ``>`` – Vehicle
- ``/`` + ``-`` – House
- ``/`` + ``*`` – Aircraft
- ``/`` + ``S`` – Boat
- ``/`` + ``B`` – Bike
- ``/`` + ``H`` – Hospital
- ``/`` + ``P`` – Police

See the APRS Protocol Specification for complete symbol tables.

Layout Configuration
--------------------

APRS stations use ``type_poi_custom0`` to support custom icons. Ensure your Navit layout includes:

.. code-block:: xml

   <itemgra item_types="poi_custom0" order="10-">
       <icon src="%s"/>
   </itemgra>

The ``%s`` format string is substituted with the icon path from ``attr_icon_src``.

Troubleshooting
---------------

Icons Not Displaying
~~~~~~~~~~~~~~~~~~~~

1. Verify icon files are installed:

   .. code-block:: bash

      ls /usr/share/navit/aprs-symbols/48x48/primary/vehicle.png

2. Check symbol lookup in a test build:

   .. code-block:: c

      char *icon = aprs_symbol_get_icon('/', '>');
      printf("Icon path: %s\n", icon);

3. Verify layout configuration includes custom POI icon support.

4. Check Navit debug logs for icon loading errors.

Missing Symbols
~~~~~~~~~~~~~~~

If a symbol is not found:

- Check if ``symbol_table`` and ``symbol_code`` are valid.
- Verify the icon file exists in the expected directory.
- The plugin falls back to the default POI icon if symbol not found.

References
----------

- hessu/aprs-symbols: https://github.com/hessu/aprs-symbols
- hessu/aprs-symbol-index: https://github.com/hessu/aprs-symbol-index
- APRS Protocol Specification: http://www.aprs.org
- aprs.fi: https://aprs.fi

