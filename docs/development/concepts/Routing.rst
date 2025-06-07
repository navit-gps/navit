Routing
=======

.. _how_it_works:

How it works
------------

First, a few rectangles are calculated (route_calc_selection):

-  One that does include the position and destination point with an
   additional 25% at the ends with an order of 4
-  One square-shaped of about 80 km edge length around each waypoint
   with an order of 8
-  One square-shaped of about 20 km edge length around each waypoint
   with an order of 18

Waypoints are the position, destination and any waypoints in between (if
routing with waypoints. The "order" of a road depends on which tile
level it is placed in. Refer to `binfile <binfile>`__, specifically the
`How An object Is placed in a
tile <binfile#How_An_object_Is_placed_in_a_tile>`__ section, for
details.

Then the map is queried with this rectangles and from the result a graph
is built, consisting of points and segments. Then the graph gets
flooded, beginning with the destination. This makes the graph re-useable
when the position changes. The used algorithm is
[http://en.wikipedia.org/wiki/Lifelong_Planning_A\ \* LPA*] together
with `Fibonacci-Heaps <http://en.wikipedia.org/wiki/Fibonacci_heap>`__
to quickly get the lowest value point.

When no points are left (this could be optimized, because once the
position is reached flooding the graph could stop) the graph is followed
from the position back to the destination and a route path is build.
This route path will be displayed just like a map.

LPA\* is an evolution of the
`Dijkstra <http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm>`__
algorithm, which Navit has previously used. LPA\* adds the ability to
change the cost of individual segments and recalculate the affected
parts of the graph. This was added in order to support dynamic traffic
information but could be expanded to implement other functionality. For
example, we could build and flood the route graph in an iterative
manner: add a few segments, flood the graph, determine where we should
continue expanding the graph and start the cycle again. We could also
use this to speed up recalculation if the user deviates from the
calculated route (which may require adding elements to the route graph).

.. _diagnosing_routing_problems:

Diagnosing Routing Problems
---------------------------

Turn on the route graph map in the Settings/Maps menu (Or from the Map
menu in GTK). You will see lots of arrows and numbers. The numbers
indicate the estimate of time in 10ths of seconds to reach your
destination. The arrows will show the direction to the destination. If
two very different numbers are close together but there should be a
connection, there is most likely none.

Tweaks
------

As of , the vehicle profile in
`navit.xml <Configuring/Full_list_of_options>`__ can take a
**``route_depth``** attribute. This attribute sets how the rectangles
mentioned above are built (how many rectangles, which size, which
order).
