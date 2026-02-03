.. _static_analysis:

Static Analysis
===============

Navit can be checked with several static analysis tools. Run ``scripts/run_static_analysis.sh`` for a quick check, or use the tools directly as described below.

Tools
-----

cppcheck (general static analyzer)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake -B build-analyze -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   cppcheck --project=build-analyze/compile_commands.json \
       --enable=warning,style,performance,portability \
       --suppress=missingIncludeSystem

clang-tidy (comprehensive linter)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake -B build-analyze -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   clang-tidy navit/plugin/driver_break/*.c -p build-analyze

Requires glib and other include paths; use ``-p build-analyze`` so compile_commands.json supplies them.

flawfinder (security vulnerability scanner)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   flawfinder --minlevel=3 navit/plugin/

Known findings (level 4+): ``driver_break_osd.c`` (snprintf format), ``driver_break_srtm.c`` (system()). Review for context; use ``// flawfinder: ignore`` for false positives.

scan-build (Clang static analyzer wrapper)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   apt install clang-tools
   scan-build -o scan-build-out cmake --build build-analyze -j4

Opens HTML report in ``scan-build-out/``.

gcc -fanalyzer (GCC built-in static analyzer)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake -B build-analyze -DENABLE_ANALYZER=ON
   cmake --build build-analyze

Can report potential bugs (e.g. NULL dereference) and may cause build failures. Use for targeted analysis.

Known Issues
------------

As of 2026-02-03, scan-build found 161 bugs project-wide. Plugin-specific issues have been addressed:

- ``driver_break_db.c``: Added error checking for sqlite3_step return codes
- ``aprs_db.c``: Added error checking for sqlite3_step return codes
- ``test_aprs_db.c``: Added null pointer checks before dereferencing station list

Most remaining issues are in core navit files (traffic.c, vehicle_file.c, util.c, callback.c) and include:

- Logic errors (83): null pointer dereferences, uninitialized values
- Memory errors (24): leaks, use-after-free
- Unused code (49): dead assignments, dead increments
