.. _reporting_bugs:

Reporting Bugs
==============

All bugs and feature requests are to be reported to the Navit developers
within `Github issues <https://github.com/navit-gps/navit/issues>`__.
Please read the guidlines below before submitting your bug - this is so
that the developers receive a clear and helpful bug report.

.. _reporting_a_bug:

Reporting a bug
===============

.. note

   You should also head over to the [[Contacts#Forums|Forums]] or to the [[Contacts#IRC| IRC channel]] if you want to discuss the bug before submitting a report 
   for example, it may just be that your local configuration is wrong, which can quickly be diagnosed by the developers.

#. Head over to https://github.com/navit-gps/navit/issues and search for
   existing tickets on that bug
#. **Read the guidelines below**
#. Create and compile your bug report, and submit it on Github
#. Be patient and wait on feedback and discussion (usually after some
   days)
#. Help to reproduce the bug or give more details on your problem


Information you should provide
==============================

The following information for each type of problem will enable the
developers to very quickly isolate the section of code which is causing
problems, and hopefully fix it. Please provide the requested information
in your bug report.


For usage problems
------------------

-  Which version of navit you are using, and whether you have compiled
   it yourself or where you downloaded the install file from or how you
   installed it.
-  What operating system and distribution you are using, and what
   platform.
-  The output of the *locale* command if using an Unix-based OS
   (Linux/Mac).
-  Ideally, reproduce your problem with the shipped navit.xml. If that
   is not possible, try to find the minimal change necessary to
   reproduce it. If you are unable to narrow down the problem, include
   your complete navit.xml, as an attachment to the ticket.
-  What maps you have used, and where you have downloaded them from (if
   applicable).

In addition, please provide the following (where appropriate):


Reporting coordinates for problems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Assuming you are using `OpenStreetMap <OpenStreetMap>`__ map packages:

#. Browse at http://www.openstreetmap.org to the area
#. Pick the "share" tool from the right toolbox.
#. Copy the link from the editbox


Routing errors
~~~~~~~~~~~~~~

Problems may include illegal routes (such as the wrong way on a one-way
road), routes which are sub-optimum or no route found at all!

-  `Report the coordinates of your position and your
   destination <#Reporting_coordinates_for_problems>`__.
-  Try routing to a point between your position and destination and
   report the coordinates at which point the route fails.
-  If you are able to route, but there is something wrong with the
   route, report what Navit gave you and what you expected.


Navigation errors
~~~~~~~~~~~~~~~~~

Problems may include missing announcements, wrong and/or misleading
announcements, and unnecessary announcements. Please enable the
navigation map to diagnose such problems.

-  **Internal GUI**:

   #. Click anywhere on the map to enter the menu.
   #. Enter the *Settings* submenu, followed by the *Maps* submenu.
   #. Enable the *Navigation* map.

-  **GTK GUI**:

   #. Click on the *Map* item in the toolbar.
   #. Enable the ''navigation:' map.

You should see a navigation symbol at every point where an action is
required. If the map looks correct, but Navit provided an incorrect
announcement, the problem is most likely tracking and/or announcement
related

-  `Report the coordinates of your position and your
   destination <#Reporting_coordinates_for_problems>`__.
-  `Report the coordinate(s) where the incident
   occurs <#Reporting_coordinates_for_problems>`__.
-  Report what Navit gave you and what you expected.

Note that quite a few navigation problems occur due to bad map data -
this can only be solved by improving the data in
`OpenStreetMap <http://www.openstreetmap.org>`__


Announcement errors
~~~~~~~~~~~~~~~~~~~

Problems may include announcements too early, announcements too late,
incorrect announcements, repeated announcements, missing announcements
etc.

-  First try to determine whether this isn't a
   `tracking <#Tracking_errors>`__ and or `navigation <#Navigation>`__
   problem.
-  \* `Report the coordinate(s) where the error occurs, where you came
   from and your destination (if routing was
   enabled) <#Reporting_coordinates_for_problems>`__.
-  If possible, provide an NMEA log (see
   `Configuration <Configuration>`__) and add the time at which the
   problem occured.
-  If an incorrect vehicle position was shown, specify where you were in
   real-life, and where Navit indicated you to be.


Tracking errors
~~~~~~~~~~~~~~~

Problems may include the indication of an incorrect vehicle position, or
jumping between correct and incorrect positions.

-  `Report the coordinate(s) where the error occurs, where you came from
   and your destination (if routing was
   enabled) <#Reporting_coordinates_for_problems>`__.
-  If possible, provide an NMEA log (see
   `Configuration <Configuration>`__) and add the time at which the
   problem occured.
-  If an incorrect vehicle position was shown, specify where you were in
   real-life, and where Navit indicated you to be.


For compile/install problems
----------------------------

First, please perform a "clean" build: Delete and recreate your build
directory, then perform a complete build (cmake + make). Normally,
re-running *cmake* after an git pull or other change should be enough,
but if you run into problems, a clean build is better.

-  Which version of navit you are using.
-  What operating system and distribution you are using, and what
   platform.
-  The source of the sourcecode or installation files (provide a link if
   possible).
-  The complete output of cmake (if you don't know how to do this, leave
   this step out).
-  The complete output of make (if you don't know how to do this, leave
   this step out).

Anyway, we will review your ticket and try to improve it, if required.

Thanks for your submission, the Navit team.


See also
========

-  `Submitting patches <Submitting_patches>`__
-  `Translations <Translations>`__
-  `Commit guidelines <Commit_guidelines>`__
