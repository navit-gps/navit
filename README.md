# Navit [![Build Status](https://img.shields.io/circleci/project/github/navit-gps/navit/trunk.svg)](https://circleci.com/gh/navit-gps/navit) [![CodeFactor](https://www.codefactor.io/repository/github/navit-gps/navit/badge)](https://www.codefactor.io/repository/github/navit-gps/navit) [![Translation](https://hosted.weblate.org/widgets/navit/-/svg-badge.svg)](https://hosted.weblate.org/engage/navit/)

_A copylefted libre software car-navigation system with its own routing engine_.

On an Android tablet \
![navit on android](https://raw.githubusercontent.com/navit-gps/navit/trunk/contrib/images/androidtablet.png) \
On a Linux-based carputer \
![navit-nuc-osd](https://user-images.githubusercontent.com/13802408/198998955-4293553f-0de1-4f72-bfe7-0e72b6207a68.png) \
[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="130">](https://f-droid.org/packages/org.navitproject.navit/)
[<img src="https://play.google.com/intl/en_us/badges/images/generic/en-play-badge.png"
     alt="Get it on Google Play"
     height="130">](https://play.google.com/store/apps/details?id=org.navitproject.navit) \
Modular design with routing and rendering of one or more vector maps in various formats. \
GTK and SDL user-interfaces with touch-screen displays. \
Current vehicle position from gpsd or directly from NMEA (GPS) sensors. \
Optimal routes and directions spoken in 70+ languages. \
Points of interest (POIs) in many formats.

Help and more info available on [the wiki](https://wiki.navit-project.org/index.php/Main_Page). \
The [Reporting Bugs](http://wiki.navit-project.org/index.php/Reporting_Bugs) document helps you file issues.

Maps
====

[OpenStreetMap](https://wiki.navit-project.org/index.php/OpenStreetMap) — display, routing, incomplete street-name search. \
[Grosser Reiseplaner](https://wiki.navit-project.org/index.php/Marco_Polo_Grosser_Reiseplaner) and compliant maps — full support. \
[Garmin maps](https://wiki.navit-project.org/index.php/Garmin_maps) —display, routing, search is being worked on.

GPS Support
===========

Current vehicle position from \
— a file or port. \
— gpsd (local or remote). \
— the location service of several mobile platforms. \
— a UDP server (friends tracking) (experimental).

Translation
===========
The [Hosted Weblate](https://hosted.weblate.org/projects/navit/) platform is used to manage translations, which runs [Weblate](https://weblate.org).
<a href="https://hosted.weblate.org/engage/navit/">
<img src="https://hosted.weblate.org/widgets/navit/-/horizontal-auto.svg" alt="Translation status" />
</a>

Routing algorithm
=================

Uses [LPA*](https://wikiless.org/wiki/Lifelong_Planning_A*) starting at the destination by assigning \
a value to each point directly connected to the destination point. \
It represents estimated time needed to reach the destination from that point. \
A Fibonacci-heap search for the point with the lowest value (to find \
a value then assigned to connected points either unevaluated or whose \
current value is greater than the new one) is repeated until the origin is found. \
Once reaching the origin, the lowest-value points are followed to the destination.

the (experimental) traffic module re-evaluates route-graph portions as segment costs change. \
It can process traffic reports and find a way around problems.
