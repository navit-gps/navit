Changelog
=========

.. raw:: mediawiki

   {{warning|1='''The content of this document has been moved to:''' https://github.com/navit-gps/navit/blob/trunk/CHANGELOG.md . It is only kept here for archiving purposes.}}

.. _navit_0.5.0:

Navit 0.5.0
-----------

This release includes way too many changes for them to be listed here
(over 2500 of them since the previous release), but one of the major
improvements is the rewrite of the navigation engine, and a new
renderer, based upon Cairo instead of GDK with nice features like
anti-aliasing and some new translations.

.. _navit_0.0.4:

Navit 0.0.4
-----------

This release contains a lot of bugfixes and other small improvements. If
you want, have a look at the roadmap status for `0.1.0 <0.1.0>`__ :
http://trac.navit-project.org/milestone/version%200.1.0

.. _navit_0.0.3:

Navit 0.0.3
-----------

Major improvements over 0.0.2 :

-  OpenStreetMap support has been greatly enhanced (navit now uses a
   binfile driver)
-  garmin support has been greatly enhanced (but routing is still being
   worked on)
-  some improvements for using navit under Openmoko (thanks to the OM
   contributors!)
-  map interaction in GTK : you can now 'slide' the map by pressing
   mouse and dragging map

And of course, a lot of bugfixes and other small improvements. See `the
full list <0.0.3>`__ for more details.

.. _navit_0.0.2:

Navit 0.0.2
-----------

Three months after the first public release, we've bundled a new version
of NavIt.

This release includes, amongst other things :

-  enhanced make install, with translations in 8 languages
-  better support for osm data, with new binfile format faster than old
   text driver
-  package building includes download of a osm sample map, so you can
   see how it looks and works without having to buy additional software
-  Improved configuration through navit.xml
-  new 'demo' vehicule which allows you to test NavIt, without even
   having a GPS!
-  the long awaited Garmin IMG driver
-  vehicle logging facility, so you can keep a log of your tracks and
   share them to OSM
-  several others bugfixes and enhancements. For a complete list, please
   see http://navit.sourceforge.net/wiki/index.php/Cvs_since_0.0.1

We're also happy to announce our partnership with
`short-circuit.com <http://www.short-circuit.com>`__. You will get 5%
discount from them if you tell them that you're a NavIt user!
