.. _traffic_event_list:

Traffic Event List
==================

This is a list of events and Supplementary Information items to be used
in conjunction with the proposed `Traffic Data
Model <Traffic_Data_Model>`__. They are intended to be capable of
expressing any TMC event but without the redundancies present in TMC.
The current list includes only a subset of these, namely those which can
be meaningfully represented in Navit.

Events
------

The initial model defines the following event classes:

+-------------+-------------------------------------------------------+
| Class       | Description                                           |
+=============+=======================================================+
| CONGESTION  | Traffic congestion, typically indicating the          |
|             | approximate speed                                     |
+-------------+-------------------------------------------------------+
| DELAY       | Delays (not necessarily related to road traffic),     |
|             | typically indicating the amount of extra waiting time |
+-------------+-------------------------------------------------------+
| RESTRICTION | Temporary traffic restrictions, such as road or lane  |
|             | closures or size, weight or access restrictions       |
+-------------+-------------------------------------------------------+
|             |                                                       |
+-------------+-------------------------------------------------------+

In the following tables, the F column (where present) marks events which
can only be used in messages whose forecast attribute is true.

Text in angle brackets <> refers to a quantifier, speed or length of
route attribute which is commonly used with the event. Such values are
always optional.

.. _events_in_the_congestion_class:

Events in the CONGESTION Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This class has different event types for different degrees of
congestion, giving an indication of average speed:

-  Heavy traffic, traffic [much] heavier than normal, (ES: tráfico
   verde, DE: reger Verkehr, [sehr viel] dichterer Verkehr als normal):
   Road is close to saturation. Ability to change lanes is somewhat
   restricted but average speed is close to the posted limit.
-  Slow traffic (ES: tráfico amarillo, DE: dichter Verkehr): Capacity of
   the road is reached. Speed may vary rapidly and rarely reaches normal
   speed. Average speed is significantly lower than the posted limit but
   traffic is still moving.
-  Queue (ES: tráfico rojo, DE: stockender Verkehr): Average speed 10–30
   km/h.
-  Stationary traffic (ES: tráfico rojo, DE: Stau): Average speed is
   less than 10 km/h.
-  Long queues (ES: tráfico retenido, DE: lange Staus): Significant
   periods of standstill in which drivers turn off their engines and
   some leave their vehicles.
-  Congestion (ES: congestión, DE: Verkehrsstörung): Generic category
   covering all of the previous four. Speed is significantly lower than
   the posted limit, anywhere between steadily moving traffic and long
   standstills. This event should be used when no accurate
   quantification is possible (as in the case of drivers observing a
   congestion without being in it, typically in the opposite direction
   of a dual-carriageway road).

If an explicit speed attribute is given, it takes precedence over the
speed implied by the event. However, sources should keep the event
consistent with the reported speed.

+-------------------------------+---+-------------------------------+
| Label                         | F | Description                   |
+===============================+===+===============================+
| CONGESTION_CLEARED            |   | Traffic congestion cleared    |
+-------------------------------+---+-------------------------------+
| CONGESTION_FORECAST_WITHDRAWN | F | Traffic congestion forecast   |
|                               |   | withdrawn                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_HEAVY_TRAFFIC      |   | Heavy traffic with average    |
|                               |   | speeds of                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_LONG_QUEUE         |   | Long queues with average      |
|                               |   | speeds of                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_NONE               |   | No problems to report         |
+-------------------------------+---+-------------------------------+
| CONGESTION_NORMAL_TRAFFIC     |   | Traffic has returned to       |
|                               |   | normal                        |
+-------------------------------+---+-------------------------------+
| CONGESTION_QUEUE              |   | Queuing traffic with average  |
|                               |   | speeds of                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_QUEUE_LIKELY       |   | Danger of queuing traffic     |
|                               |   | with average speeds of        |
+-------------------------------+---+-------------------------------+
| CONGESTION_SLOW_TRAFFIC       |   | Slow traffic with average     |
|                               |   | speeds of                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_STATIONARY_TRAFFIC |   | Stationary traffic (frequent  |
|                               |   | standstills)                  |
+-------------------------------+---+-------------------------------+
| CONGEST                       |   | Danger of stationary traffic  |
| ION_STATIONARY_TRAFFIC_LIKELY |   |                               |
+-------------------------------+---+-------------------------------+
| C                             |   | Traffic building up with      |
| ONGESTION_TRAFFIC_BUILDING_UP |   | average speeds of             |
+-------------------------------+---+-------------------------------+
| CONGESTION_TRAFFIC_CONGESTION |   | Traffic congestion with       |
|                               |   | average speeds of             |
+-------------------------------+---+-------------------------------+
| CONGESTION_TRAFFIC_EASING     |   | Traffic easing                |
+-------------------------------+---+-------------------------------+
| CONG                          |   | Traffic flowing freely with   |
| ESTION_TRAFFIC_FLOWING_FREELY |   | average speeds of             |
+-------------------------------+---+-------------------------------+
| CONGESTIO                     |   | Traffic heavier than normal   |
| N_TRAFFIC_HEAVIER_THAN_NORMAL |   | with average speeds of        |
+-------------------------------+---+-------------------------------+
| CONGESTIO                     |   | Traffic lighter than normal   |
| N_TRAFFIC_LIGHTER_THAN_NORMAL |   | with average speeds of        |
+-------------------------------+---+-------------------------------+
| CONGESTION_TRA                |   | Traffic very much heavier     |
| FFIC_MUCH_HEAVIER_THAN_NORMAL |   | than normal with average      |
|                               |   | speeds of (increased density  |
|                               |   | but no significant decrease   |
|                               |   | in speed)                     |
+-------------------------------+---+-------------------------------+
| CONGESTION_TRAFFIC_PROBLEM    |   | Traffic problem               |
+-------------------------------+---+-------------------------------+
|                               |   |                               |
+-------------------------------+---+-------------------------------+

.. _events_in_the_delay_class:

Events in the DELAY Class
~~~~~~~~~~~~~~~~~~~~~~~~~

======================== = ============================
Label                    F Description
======================== = ============================
DELAY_CLEARANCE            Delays cleared
DELAY_DELAY                Delays up to
DELAY_DELAY_POSSIBLE     F Delays up to possible
DELAY_FORECAST_WITHDRAWN F Delay forecast withdrawn
DELAY_LONG_DELAY           Long delays up to
DELAY_SEVERAL_HOURS        Delays of several hours
DELAY_UNCERTAIN_DURATION   Delays of uncertain duration
DELAY_VERY_LONG_DELAY      Very long delays up to
\                          
======================== = ============================

.. _events_in_the_restriction_class:

Events in the RESTRICTION Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------------------------------+----------------------------------+
| Label                            | Description                      |
+==================================+==================================+
| RESTRI                           | Traffic restrictions lifted:     |
| CTION_ACCESS_RESTRICTIONS_LIFTED | reopened for all traffic, other  |
|                                  | restrictions (overtaking etc.)   |
|                                  | remain in place                  |
+----------------------------------+----------------------------------+
| REST                             | All carriageways cleared         |
| RICTION_ALL_CARRIAGEWAYS_CLEARED |                                  |
+----------------------------------+----------------------------------+
| RESTR                            | All carriageways reopened        |
| ICTION_ALL_CARRIAGEWAYS_REOPENED |                                  |
+----------------------------------+----------------------------------+
| RESTRICTION_BATCH_SERVICE        | Batch service (to limit the      |
|                                  | amount of traffic passing        |
|                                  | through a section, unlike single |
|                                  | alternate line traffic)          |
+----------------------------------+----------------------------------+
| RESTRICTION_BLOCKED              | Blocked (refers to the entire    |
|                                  | road; separate codes exist for   |
|                                  | blockages of individual lanes or |
|                                  | carriageways)                    |
+----------------------------------+----------------------------------+
| RESTRICTION_BLOCKED_AHEAD        | Blocked ahead (at a point beyond |
|                                  | the indicated location)          |
+----------------------------------+----------------------------------+
| RESTRICTION_CARRIAGEWAY_BLOCKED  | Carriageway blocked (main        |
|                                  | carriageway, unless otherwise    |
|                                  | indicated in supplementary       |
|                                  | information)                     |
+----------------------------------+----------------------------------+
| RESTRICTION_CARRIAGEWAY_CLOSED   | Carriageway closed (main         |
|                                  | carriageway, unless otherwise    |
|                                  | indicated in supplementary       |
|                                  | information)                     |
+----------------------------------+----------------------------------+
| RESTRICTION_CLOSED               | Closed until (refers to the      |
|                                  | entire road; separate codes      |
|                                  | exist for closures of individual |
|                                  | lanes or carriageways)           |
+----------------------------------+----------------------------------+
| RESTRICTION_CLOSED_AHEAD         | Closed ahead (at a point beyond  |
|                                  | the indicated location)          |
+----------------------------------+----------------------------------+
| RESTRICTION_CONTRAFLOW           | Contraflow                       |
+----------------------------------+----------------------------------+
| RESTRICTION_ENTRY_BLOCKED        | th entry slip road blocked       |
+----------------------------------+----------------------------------+
| RESTRICTION_ENTRY_REOPENED       | Entry reopened                   |
+----------------------------------+----------------------------------+
| RESTRICTION_EXIT_BLOCKED         | th exit slip road blocked        |
+----------------------------------+----------------------------------+
| RESTRICTION_EXIT_REOPENED        | Exit reopened                    |
+----------------------------------+----------------------------------+
| R                                | Intermittent short term closures |
| ESTRICTION_INTERMITTENT_CLOSURES |                                  |
+----------------------------------+----------------------------------+
| RESTRICTION_LANE_BLOCKED         | Lanes blocked                    |
+----------------------------------+----------------------------------+
| RESTRICTION_LANE_CLOSED          | Lanes closed                     |
+----------------------------------+----------------------------------+
| RESTRICTION_OPEN                 | Open                             |
+----------------------------------+----------------------------------+
| RESTRICTION_RAMP_BLOCKED         | Ramps blocked                    |
+----------------------------------+----------------------------------+
| RESTRICTION_RAMP_CLOSED          | Ramps closed                     |
+----------------------------------+----------------------------------+
| RESTRICTION_RAMP_REOPENED        | Ramps reopened                   |
+----------------------------------+----------------------------------+
| RESTRICTION_REDUCED_LANES        | Carriageway reduced from         |
|                                  | <q:ints[1]> to <q:ints[0]> lanes |
|                                  | (<q:ints[1]> can be omitted)     |
+----------------------------------+----------------------------------+
| RESTRICTION_REOPENED             | Reopened                         |
+----------------------------------+----------------------------------+
| RESTRICTION_ROAD_CLEARED         | Road cleared                     |
+----------------------------------+----------------------------------+
| RESTRICTI                        | Single alternate line traffic    |
| ON_SINGLE_ALTERNATE_LINE_TRAFFIC | (because the affected stretch of |
|                                  | road can only be used in one     |
|                                  | direction at a time, different   |
|                                  | from batch service)              |
+----------------------------------+----------------------------------+
| RESTRICTION_SPEED_LIMIT          | Speed limit in force             |
+----------------------------------+----------------------------------+
| RESTRICTION_SPEED_LIMIT_LIFTED   | Speed limit lifted               |
+----------------------------------+----------------------------------+
|                                  |                                  |
+----------------------------------+----------------------------------+

.. _supplementary_information:

Supplementary Information
-------------------------

The initial model defines the following supplementary information
classes:

======== ============================================================
Class    Description
======== ============================================================
PLACE    Qualifiers specifying the place(s) to which the event refers
TENDENCY Traffic density development
VEHICLE  Specifies categories of vehicles to which the event applies
\        
======== ============================================================

.. _supplementary_information_in_the_place_class:

Supplementary Information in the PLACE Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

================= =====================
Label             Description
================= =====================
S_PLACE_BRIDGE    On bridges
S_PLACE_RAMP      On ramps (entry/exit)
S_PLACE_ROADWORKS In the roadworks area
S_PLACE_TUNNEL    In tunnels
\                 
================= =====================

.. _supplementary_information_in_the_tendency_class:

Supplementary Information in the TENDENCY Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------------------+---------------------------------------+
| Label                       | Description                           |
+=============================+=======================================+
| S_TENDENCY_QUEUE_DECREASING | Traffic queue length decreasing at an |
|                             | average rate of                       |
+-----------------------------+---------------------------------------+
| S_TENDENCY_QUEUE_INCREASING | Traffic queue length increasing at an |
|                             | average rate of                       |
+-----------------------------+---------------------------------------+
|                             |                                       |
+-----------------------------+---------------------------------------+

.. _supplementary_information_in_the_vehicle_class:

Supplementary Information in the VEHICLE Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

========================== ===============================
Label                      Description
========================== ===============================
S_VEHICLE_ALL              For all vehicles
S_VEHICLE_BUS              For buses only
S_VEHICLE_CAR              For cars only
S_VEHICLE_CAR_WITH_CARAVAN For cars with caravans only
S_VEHICLE_CAR_WITH_TRAILER For cars with trailers only
S_VEHICLE_HAZMAT           For hazardous loads only
S_VEHICLE_HGV              For heavy trucks only
S_VEHICLE_MOTOR            For all motor vehicles
S_VEHICLE_WITH_TRAILER     For vehicles with trailers only
\                          
========================== ===============================
