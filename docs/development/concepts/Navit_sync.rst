.. _navit_sync:

Navit sync
==========

.. raw:: mediawiki

   {{note
   |This Page showes only a draft of a idea and nothing of is is even programmed. This is mostly a Brainstorming
   }}

Introduction
------------

Normaly i have Navit installed on my smartphone and searching locations
with Google maps or Navit. Mostly Google Maps because its mostly way
faster. but when it goes to routing in car i just start my tomtom with
really old maps and navigate with this one.

.. _so_why_i_am_telling_this:

So why i am telling this?
~~~~~~~~~~~~~~~~~~~~~~~~~

Because i am working on NavitTom build a full Linux system for Tomtom.
So even the old maps and the Tomtom software will be gone.

.. _the_idea:

The Idea
--------

When we have Navit running on NavitTom **and** the Smartphone why not
sync them? The benefits:

-  We can create a route just on the smartphone and sync it to NavitTom.
-  Probably we can also sync maps by having a sync adapter which
   retrives the needed informations from the smartphone and sends them
   to the Navi
-  We can sync Notifications to the Navi to display then when wanted
-  We can use the Smartphone Mobile Data to retive traffic informations
   if user whants to.

.. _how_to_do_this:

How to do this?
---------------

There are two ideas how to make this happen.

First one whould be to write and standalone app to do the sync of
notifications and the other stuff we need.

Second idea would be to use the existing code and build a sync interface
for it. This could also be a separate app just with a different gui of
even build into navit.
