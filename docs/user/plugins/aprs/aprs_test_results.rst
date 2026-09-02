APRS Plugin Test Results
========================

Test Execution Date
-------------------

**Date**: 2026-01-28
**Last Updated**: 2026-01-28 (Valgrind memory check)
**Build Configuration**: Plugin enabled for testing
**Test Framework**: Custom unit tests
**Memory Check**: Valgrind 3.22.0

Test Summary
------------

The APRS plugin tests verify packet decoding, position parsing, and database operations.

Available Tests
---------------

The following test executables are available:

- ``test_aprs_decode`` - AX.25 packet decoding and position parsing tests
- ``test_aprs_db`` - Database operation tests
- ``test_aprs_dsp`` - DSP operation tests (SDR plugin)

Test Results
------------

Decode Tests (test_aprs_decode)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: PASSED

**Test Output**::

   Running APRS decode tests...
   All decode tests passed!

**Test Cases**:

- AX.25 frame decoding
- Callsign encoding/decoding
- Position parsing (ddmm.mm format)
- Latitude/longitude validation
- Invalid input handling

**Result**: All decode tests passed successfully.

**Valgrind Memory Check**:

- **Status**: PASSED (after fixes)
- **Memory Leaks**: 0 bytes lost (fixed from leaks in error paths)
- **Errors**: 0 errors from 0 contexts
- **Fixes Applied**:
  - Added ``aprs_packet_free()`` calls on all error return paths in ``aprs_decode_ax25()``
  - Added ``aprs_position_free()`` calls on all error return paths in ``aprs_parse_position()``
  - Refactored test assertions to ensure cleanup functions are always called
  - Fixed latitude parsing issue (incorrect 3-digit degree parsing for 8-character latitude strings)

Database Tests (test_aprs_db)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Status**: PASSED

**Test Cases**:

- Station insertion and retrieval
- Range-based queries
- Station expiration
- Database initialization

**Result**: All database tests passed successfully.

**Valgrind Memory Check**:

- **Status**: PASSED
- **Memory Leaks**: 0 bytes lost
- **Errors**: 0 errors from 0 contexts

Running Tests
-------------

To run the available tests:

.. code-block:: bash

   cd build
   ./navit/plugin/aprs/tests/test_aprs_decode
   ./navit/plugin/aprs/tests/test_aprs_db

To run tests with Valgrind memory checking:

.. code-block:: bash

   cd build
   valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all \
     ./navit/plugin/aprs/tests/test_aprs_decode
   valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all \
     ./navit/plugin/aprs/tests/test_aprs_db

Memory Leak Fixes
-----------------

The following memory leaks were identified and fixed:

1. **AX.25 Decode Error Paths**: Added ``aprs_packet_free()`` calls before all ``return 0;`` statements in ``aprs_decode_ax25()`` to ensure allocated callsigns and information fields are freed on error.

2. **Position Parse Error Paths**: Added ``aprs_position_free()`` calls before all ``return 0;`` statements in ``aprs_parse_position()`` to ensure allocated position data is freed on error.

3. **Test Assertion Cleanup**: Refactored ``TEST_ASSERT`` macro usage in test files to use an ``int ret`` variable pattern, ensuring cleanup functions are always called before test function returns.

4. **Latitude Parsing Fix**: Modified ``parse_ddmm_mm()`` to check input string length (8 for latitude, 9 for longitude) and use appropriate sscanf format, preventing incorrect parsing of latitude strings.

All fixes ensure proper memory management and prevent resource leaks.
