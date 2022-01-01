.. _augmented_reality:

Augmented Reality
=================

.. _project_reality_view:

**Project Reality View**
------------------------

In cooperation with the Geoinformation Group of Potsdam University an
Navit extension based on augmented reality will be developed. The
Android platform is the basis on which Navit is also available now.
Complete instructions, Screenshots and of course the source code will be
available in the next weeks at this site. Until then, the following
concept introduces the idea of the project.

Introduction
~~~~~~~~~~~~

Augmented Reality (AR) combines detailed real-world image data with
virtual route information. Clear and easy to follow instructions provide
maximum benefit for pedestrians in particular. The pedestrian navigation
system called Reality View is based on Navit and uses the existing
routing algorithms and menu structure. Displaying superimposed camera
images and virtual models with a high degree of congruence requires
different sensors. Along with GPS an electronic compass and an
acceleration sensor are used. Thus equipped, the device is able to
recognise the location, position and alignment and adapt the display
accordingly. Much current development to Navit, the established open
source car navigation system, is focussed on its suitability for use
with pedestrians. Pedestrian movement is much more complex than
vehicular flow. Not just highways but also paths and areas such as
pedestrian zones, parks and open public spaces have to be incorporated
into the route guidance. Stocks and structures of the underlying geodata
must therefore be partly regenerated or adapted to the requirements of
the target group. The concept is based on the experience and approach of
navigation principles in 3D-game simulation. At the screen, users have a
restricted visual range that is augmented with the aid of additional
elements. The basis for comparing 3D computer simulation and the
navigation of pedestrians is represented by the augmented perception of
the user. Analogous to operation of the camera in mobile telephones,
every movement is simultaneously shown on the display. Thus it is
possible to have a direct and frontal view of the relevant determination
points such as junctions, underpasses or building entrances. The virtual
model information required for navigation is segmented and superimposed
onto the camera image. With the aid of Reality View (cf. chapter 3) the
user’s restricted real visual field is enhanced with prescient route
information and presented as Augmented Reality (AR).

Pedestrians
~~~~~~~~~~~

Pedestrian and vehicular movement and route choice behaviour differ.
Different factors influence route choice. Unlike vehicles, the fastest
or shortest route does not always have highest priority. Crucial factors
governing route choice include the attractiveness of surroundings, shops
and restaurants en route, as well as pedestrian safety.

.. _navigation_methods:

Navigation methods
~~~~~~~~~~~~~~~~~~

Since the inception of 3D game simulation, various methods have been
developed to enhance user-computer interaction. Irrespective of game
genre and tasks set to players, 3D game simulation provides a number of
methods to support navigation. Years of experience in 3D game technology
have helped to provide a wide range of effective methods that benefit
users of computer navigation. Newly in use is a dynamic cable adapted to
the route. Like a wire stretched to the destination, this cable follows
the selected route based on a real-world situation.

.. _reality_view:

Reality View
~~~~~~~~~~~~

An integrated optical camera allows a live stream of the selected
perspective to be displayed directly on the screen. The combination of
detailed image data and precise route information enables the user to
recognise his current environment and at the same time follow the route
description. Reality View provides the basis for the the pedestrian
navigation system presented here. Implementation requires a mobile
device with a graphic display and other components. Absolute positioning
in space necessitates the use of a satellite receiver. Since
superimposing the reality view and the route information requires 3
dimensions, a compass and position sensors are also used. Shifting or
rotating the standing position can thus be registered by the device and
be displayed in the camera scene. Live images of the surroundings
required for the reality view are generated using an integrated camera.

Presentation
~~~~~~~~~~~~

The combined view of camera image and virtual model has to be adapted
for superimposition. The principal disparity is in the superimposition
of a 2D map perspective and 3D camera image. Even though the camera does
not truly generate a 3 dimensional image, it nonetheless shows raised
features and therefore view obstructions. A realistic combination of
both views thus necessitates modifying the base map to the real-world
situation. This is made possible by applying more sensors which apart
from calculating the position and orientation of the device also
determine its alignment, i.e. location with respect to the horizontal
plane. Using this information the navigation software is able to adapt
the route information displayed to the actual real-world view. Here the
focus is on determining the actual visual range based on the real-world
situation. Obstructions on the base map are blended in the maximum
visual range of the camera. The result is a reduced visual range adapted
to the real-world surroundings.

.. _base_maps:

Base Maps
~~~~~~~~~

Navigation should be based as far as possible on the use of open
geodata, i.e. geodata available for free distribution. The OpenStreetMap
project takes centre stage here. The extent to which these data are
suitable for pedestrian navigation purposes is looked into. To offset
the drawbacks of these jointly developed data, geodata of different
origins are consolidated using special data merging algorithms. Data
from commercial suppliers as well as that provided by Land Registry
offices are integrated. The result is illustrated by a combined dataset
enhanced for pedestrian navigation with the aid of separate attributes
from different sources. A drawback of jointly collected data is the
absence of attribute standardisation. The application of a standard
object type index as with data maintained by the Land Registry is a
prerequisite for use in navigation. It is therefore necessary to compile
such an index for pedestrian navigation purposes. It should specify the
source of the assumed attributes as well as the necessary and not yet
recorded objects.

Summary
~~~~~~~

The schematic display of the result shows that the development of a
pedestrian navigation system with Reality View uses a combination of
different components. The current prototype comprises a UMPC (Ultra
Mobile PC) and the various external components. Since neither size nor
weight is ideal, the prototype is suitable for demonstration and
experimental purposes only. Addition of the software to the Android
platform is pending following release of the Linux-based prototype on
the Wibrain UMPC. The latest hardware comes equipped with all the
necessary components such as GPS, electronic compass and acceleration
sensor. Together with the open and therefore easily extensible Android
platform, the ideal environment for the widespread use of Navit for
pedestrian navigation purposes.

**Pictures**
------------

File:pns-1-small.jpg File:pns-2-small.jpg

.. _menu_integration:

Menu Integration
----------------

An integration in the `Menu <Menu>`__ is possible as extension on the
top level of the navit menu, because it can be regarded as a main view
of navigation of pedestrians.

.. _download_android_.apk_package:

**Download Android .apk Package**
---------------------------------

[Navit-RealityView
Current\ `1 <http://download.navit-project.org/navit/android_armv5te/svn/navit-current.apk>`__]

**Evaluation**
--------------

canceled (31/12/2011)

**Contact**
-----------

''Dipl.-Ing. Mario Kluge

University of Potsdam

Department of Geography

Geoinformation Group

Karl-Liebknecht-Str. 24/25

14476 Potsdam/Golm

mail: mario.kluge[at]uni-potsdam.de

phone:0049 331 977 2629''
