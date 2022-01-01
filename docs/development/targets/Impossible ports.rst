.. _impossible_ports:

Impossible ports
================

| Even if NAVIT is highly portable, a few platforms can't be supported.
  Here we explain why.
| In general we need an possible C compiler target for our code and all
  included libs (see `dependencies <dependencies>`__) and of course a
  device with enough resources.

.. _java_me:

Java ME
-------

Phones in the pre-smartphone area supported third-party applications
with the limited java mobile edition. Unfortunately this excludes native
C development that NAVIT requires.

.. _windows_phone_7:

Windows Phone 7
---------------

This older version of Windows Phone just supports third-party apps
developed with .Net technology. Unfortunately, this excludes native C
development that NAVIT requires
