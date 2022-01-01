QA
==

| routing QA is a github repo, on CircleCI it compiles Navit, fetches
  some maps and then runs routing and navigation tests
| https://github.com/navit-gps/routing-qa
| A test case is made of start and end coordinates and a succes
  criterion
| sample
  https://github.com/navit-gps/routing-qa/blob/master/Fremont_drive_bypass.yaml

The output of the test is

-  a screenshot
   https://210-32961764-gh.circle-artifacts.com/0/tmp/circle-artifacts.tPYpN6N/Fremont_drive_bypass.yamlmetric.png
-  a gpx track
   https://210-32961764-gh.circle-artifacts.com/0/tmp/circle-artifacts.tPYpN6N/Fremont_drive_bypass.yamlmetric.gpx
-  a GeoJSON structure
   https://210-32961764-gh.circle-artifacts.com/0/tmp/circle-artifacts.tPYpN6N/Fremont_drive_bypass.yamlmetric.geojson

.. _relation_to_the_navit_main_github_repo:

Relation to the Navit main Github repo
--------------------------------------

A branch in the routing-QA repo runs the code in a branch in the Navit
repo with the same name. If you run the master branch of routing-QA it
will compile code from the corresponding master branch of Navit.

Suppose you create a branch in the Navit repo with the name wiki_test
and you want to check how it affects routing and navigation then you
just create a branch with the same name wiki_test in the routing-QA repo
and run it. Now you can compare the tests of the code in your branch
with the tests that ran on the master-branch.

Dev's with a fork of Navit on github can fork the routing-QA as well,
just change a few links in circle.yml to make it fetch code from your
fork instead of the Navit repo.

Then compare the results of the code in your fork against the results of
the Navit repo
https://300-40190699-gh.circle-artifacts.com/0/tmp/circle-artifacts.o5xOHXn/Fremont_drive_bypass.yamlmetric.png

.. _easy_submit_of_a_testcase:

Easy submit of a testcase
-------------------------

In case you want to report a single routing or navigation problem there
is no need to get a Github and CircleCi account. On Openstreetmap find
the right location, move the markers for start and end of the route as
in `OSM
sample <http://www.openstreetmap.org/directions?engine=osrm_car&route=38.235%2C-122.461%3B38.237%2C-122.460#map=18/38.23568/-122.46071>`__
And then post the link together with your bugreport on the forum, on IRC
or Github or whatever medium you prefer. Everybody can see the situation
on OSM right away and somebody will copy/paste the coordinates into a
testcase.
