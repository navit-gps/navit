.. _turn_restrictions:

Turn Restrictions
=================

Turn restrictions are currently processed in the route graph in the
following manner:

-  Ways which are part of turn restrictions are added to the graph as
   items of type ``type_street_turn_restriction_no`` or
   ``type_street_turn_restriction_only``.
-  The ``via`` point (or the end points of the ``via`` way) gets the
   ``RP_TURN_RESTRICTION`` flag set.
-  After all ways have been added to the route graph, turn restrictions
   are processed further: For each segment which attaches to a point
   with the ``RP_TURN_RESTRICTION`` flag set, a copy of the point
   (without the flag) is created. The segment, as well as all other
   segments which are part of allowed turns, is cloned; the clone is
   marked as oneway (in the direction of the allowed turn) and attaches
   to the copy of the point. This means that we may have multiple points
   at the same coordinates—one more than we have segments for each turn
   restriction.
-  When calculating a route, segments are considered impassable if they
   begin or end at a point with the ``RP_TURN_RESTRICTION`` flag set.
   That way, the route will use one of the cloned subsets and only
   follow allowed turns.

As of August 2018, this seems to be fully implemented for turn
restrictions where ``via`` is a node.
