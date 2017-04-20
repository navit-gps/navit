[![Build Status](https://img.shields.io/circleci/project/github/navit-gps/navit/trunk.svg)](https://circleci.com/gh/navit-gps/navit)
[![OpenCollective](https://opencollective.com/navit/backers/badge.svg)](#backers) 
[![OpenCollective](https://opencollective.com/navit/sponsors/badge.svg)](#sponsors)

Navit on Android tablet:

![navit on android](http://wiki.openstreetmap.org/w/images/2/25/Navit_android.png)

Navit on Linux based Carputer:

![navit-nuc-osd](https://github.com/pgrandin/navit-nuc-layout/raw/master/screenshot.png)

<p>
<a href="https://play.google.com/store/apps/details?id=org.navitproject.navit"><img src="http://switzerland.tasis.com/uploaded/images2/appstore_button_google.png" height="100"/></a>

<a href="https://f-droid.org/repository/browse/?fdfilter=navit&fdid=org.navitproject.navit"><img src="https://upload.wikimedia.org/wikipedia/commons/thumb/0/0d/Get_it_on_F-Droid.svg/200px-Get_it_on_F-Droid.svg.png" height="100"/></a>
</p>

NavIT
=====

Navit is a open source (GPL) car navigation system with routing engine.

It's modular design is capable of using vector maps of various formats
for routing and rendering of the displayed map. It's even possible to
use multiple maps at a time.

The GTK+ or SDL user interfaces are designed to work well with touch
screen displays. Points of Interest of various formats are displayed
on the map.

The current vehicle position is either read from gpsd or directly from
NMEA GPS sensors.

The routing engine not only calculates an optimal route to your
destination, but also generates directions and even speaks to you.

Navit currently speaks over 70 languages!

You can help translating via our web based translation page :
 http://translations.launchpad.net/navit/trunk/+pots/navit


For help or more information, please refer to the wiki :
 http://wiki.navit-project.org

If you don't know where to start, we recommend you to read the 
Interactive Help : http://wiki.navit-project.org/index.php/Interactive_help


Maps:
=====

The best navigation system is useless without maps. Those three maps
are known to work:

- OpenStreetMaps : display, routing, but street name search isn't complete
 (see http://wiki.navit-project.org/index.php/OpenStreetMaps )

- Grosser Reiseplaner and compliant maps : full support
 (see http://wiki.navit-project.org/index.php/European_maps )

- Garmin maps : display, routing, search is being worked on
 (see http://wiki.navit-project.org/index.php/Garmin_maps )


GPS Support:
============

Navit read the current vehicle position :
- directly from a file
- from gpsd (local or remote)
- from udp server (friends tracking) (experimental)


Routing algorithm
=================

NavIt uses a Dijkstra algorithm for routing. The routing starts at the
destination by assigning a value to each point directly connected to
destination point. The value represents the estimated time needed to
pass this distance.

Now the point with the lowest value is choosen using the Fibonacci
heap and a value is assigned to connected points whos are
unevaluated or whos current value ist greater than the new one.

The search is repeated until the origin is found.

Once the origin is reached, all that needs to be done is to follow the
points with the lowest values to the destination.


Backers
=======
Support us with a monthly donation and help us continue our activities. [[Become a backer](https://opencollective.com/navit#backer)]

<a href="https://opencollective.com/navit/backer/0/website" target="_blank"><img src="https://opencollective.com/navit/backer/0/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/1/website" target="_blank"><img src="https://opencollective.com/navit/backer/1/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/2/website" target="_blank"><img src="https://opencollective.com/navit/backer/2/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/3/website" target="_blank"><img src="https://opencollective.com/navit/backer/3/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/4/website" target="_blank"><img src="https://opencollective.com/navit/backer/4/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/5/website" target="_blank"><img src="https://opencollective.com/navit/backer/5/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/6/website" target="_blank"><img src="https://opencollective.com/navit/backer/6/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/7/website" target="_blank"><img src="https://opencollective.com/navit/backer/7/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/8/website" target="_blank"><img src="https://opencollective.com/navit/backer/8/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/9/website" target="_blank"><img src="https://opencollective.com/navit/backer/9/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/10/website" target="_blank"><img src="https://opencollective.com/navit/backer/10/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/11/website" target="_blank"><img src="https://opencollective.com/navit/backer/11/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/12/website" target="_blank"><img src="https://opencollective.com/navit/backer/12/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/13/website" target="_blank"><img src="https://opencollective.com/navit/backer/13/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/14/website" target="_blank"><img src="https://opencollective.com/navit/backer/14/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/15/website" target="_blank"><img src="https://opencollective.com/navit/backer/15/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/16/website" target="_blank"><img src="https://opencollective.com/navit/backer/16/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/17/website" target="_blank"><img src="https://opencollective.com/navit/backer/17/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/18/website" target="_blank"><img src="https://opencollective.com/navit/backer/18/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/19/website" target="_blank"><img src="https://opencollective.com/navit/backer/19/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/20/website" target="_blank"><img src="https://opencollective.com/navit/backer/20/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/21/website" target="_blank"><img src="https://opencollective.com/navit/backer/21/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/22/website" target="_blank"><img src="https://opencollective.com/navit/backer/22/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/23/website" target="_blank"><img src="https://opencollective.com/navit/backer/23/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/24/website" target="_blank"><img src="https://opencollective.com/navit/backer/24/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/25/website" target="_blank"><img src="https://opencollective.com/navit/backer/25/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/26/website" target="_blank"><img src="https://opencollective.com/navit/backer/26/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/27/website" target="_blank"><img src="https://opencollective.com/navit/backer/27/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/28/website" target="_blank"><img src="https://opencollective.com/navit/backer/28/avatar.svg"></a>
<a href="https://opencollective.com/navit/backer/29/website" target="_blank"><img src="https://opencollective.com/navit/backer/29/avatar.svg"></a>

Sponsors
=========
Become a sponsor and get your logo on our README on Github with a link to your site. [[Become a sponsor](https://opencollective.com/navit#sponsor)]

<a href="https://opencollective.com/navit/sponsor/0/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/0/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/1/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/1/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/2/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/2/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/3/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/3/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/4/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/4/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/5/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/5/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/6/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/6/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/7/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/7/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/8/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/8/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/9/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/9/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/10/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/10/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/11/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/11/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/12/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/12/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/13/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/13/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/14/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/14/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/15/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/15/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/16/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/16/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/17/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/17/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/18/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/18/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/19/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/19/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/20/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/20/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/21/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/21/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/22/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/22/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/23/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/23/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/24/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/24/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/25/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/25/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/26/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/26/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/27/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/27/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/28/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/28/avatar.svg"></a>
<a href="https://opencollective.com/navit/sponsor/29/website" target="_blank"><img src="https://opencollective.com/navit/sponsor/29/avatar.svg"></a>

