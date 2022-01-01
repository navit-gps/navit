.. _command_interface:

Command interface
=================

Commands
--------

Commands available in the internal GUI:

+----------------------------------+----------------------------------+
| Command                          | Meaning                          |
+==================================+==================================+
| **``abort_navigation()``**       | Cancels navigation: The route is |
|                                  | cleared and Navit switches to    |
|                                  | tracking mode.                   |
+----------------------------------+----------------------------------+
| **``about()``**                  | Displays the About screen.       |
+----------------------------------+----------------------------------+
| **``back()``**                   | Returns to the previous menu     |
|                                  | page.                            |
+----------------------------------+----------------------------------+
| **``back_to_map()``**            | Leaves the menu and returns to   |
|                                  | map view.                        |
+----------------------------------+----------------------------------+
| **``bookmarks()``**              | Shows bookmarks.                 |
+----------------------------------+----------------------------------+
| **``formerdests()``**            | Shows list of former             |
|                                  | destinations.                    |
+----------------------------------+----------------------------------+
| **``get_data()``**               | FIXME: description to be         |
|                                  | completed                        |
+----------------------------------+----------------------------------+
| **``locale()``**                 | Shows locale information.        |
+----------------------------------+----------------------------------+
| **``log()``**                    | FIXME: seems to bring up a       |
|                                  | dialog allowing the user to      |
|                                  | enter a message which will get   |
|                                  | written to the debug log; useful |
|                                  | to mark a position in the log    |
+----------------------------------+----------------------------------+
| **``me                           | Brings up the menu (may be used  |
| nu(``\ ``fromosd,``\ ````\ ``hre | as **``gui.menu()``** ). First   |
| f,``\ ````\ ``replace``\ ``)``** | optional argument, fromosd, is   |
|                                  | to be specified as 1 when        |
|                                  | calling this command from an osd |
|                                  | button by the tap or mouse       |
|                                  | click, otherwise it may be       |
|                                  | omitted or set to zero. You may  |
|                                  | get directly into the sub-menu   |
|                                  | by specifying its href anchor    |
|                                  | like this:                       |
|                                  | **``gui.menu("#Actions")``** .   |
|                                  | Also, if href is specified, this |
|                                  | command may close currently      |
|                                  | opened sub-menu and open another |
|                                  | one on its place if 'replace' is |
|                                  | non-zero                         |
|                                  | numeric                          |
|                                  | :**``gui.menu("#Actions",1)``**. |
+----------------------------------+----------------------------------+
| **``osd_s                        | Sets the attribute of an OSD     |
| et_attr(``\ ``name``\ ``,``\ ``a | item. The OSD item must have a   |
| ttr``\ ``,``\ ``value``\ ``)``** | **name** attribute matching the  |
|                                  | **name** parameter in the call.  |
|                                  | **attr** is the name of the      |
|                                  | attribute, and **value** is the  |
|                                  | new value.                       |
+----------------------------------+----------------------------------+
| **``pois(``\ ``position,f        | Displays POIs around given       |
| ilter,isaddressfilter``\ ``)``** | point, possibly filtering them   |
|                                  | by text string as it would be    |
|                                  | entered in POI filtering dialog. |
|                                  | Second and third arguments may   |
|                                  | be omitted.                      |
+----------------------------------+----------------------------------+
| **``posit                        | Presents possible operations on  |
| ion(``\ ``position,``\ ````\ ``t | a position (set as current       |
| ext,``\ ````\ ``flags``\ ``)``** | location, set as destination     |
|                                  | etc.). ``position`` is a         |
|                                  | coordinate-type attribute (e.g.  |
|                                  | **``position_coord_geo``**).     |
+----------------------------------+----------------------------------+
| **``quit()``**                   | Closes Navit.                    |
+----------------------------------+----------------------------------+
| **``redraw_map()``**             | Redraws the map view.            |
+----------------------------------+----------------------------------+
| **``route_description()``**      | Shows a turn-by-turn description |
|                                  | of the active route when in      |
|                                  | navigation mode.                 |
+----------------------------------+----------------------------------+
| **``route_height_profile()``**   | Shows a height profile of the    |
|                                  | active route when in navigation  |
|                                  | mode.                            |
+----------------------------------+----------------------------------+
| **``setting_layout()``**         | Presents a selection of          |
|                                  | available screen layouts.        |
+----------------------------------+----------------------------------+
| **``setting_maps()``**           | Presents a dialog to switch      |
|                                  | between available mapsets.       |
+----------------------------------+----------------------------------+
| **``setting_rules()``**          | Presents a dialog for setting    |
|                                  | various internal options.        |
+----------------------------------+----------------------------------+
| **``setting_vehicle()``**        | Presents a dialog for selecting  |
|                                  | the active vehicle.              |
+----------------------------------+----------------------------------+
| **``spaw                         | | Launches the external program  |
| n(``\ ``command``\ ``,``\ ``para |   specified by **command**,      |
| m1``\ ``,``\ ``param2``\ ``)``** |   passing it the parameters      |
|                                  |   specified as additional        |
|                                  |   arguments. This takes place in |
|                                  |   a fire-and-forget fashion,     |
|                                  |   i.e. navit does not wait for   |
|                                  |   the program to finish or       |
|                                  |   evaluate its result.           |
|                                  | | Use as **``gui.spawn()``**     |
|                                  |   when calling from an OSD item. |
|                                  | | FIXME: is the number of        |
|                                  |   parameters fixed or variable?  |
|                                  |   Limitations?                   |
+----------------------------------+----------------------------------+
| **``town()``**                   | Presents a dialog for selecting  |
|                                  | an address, starting with a      |
|                                  | town.                            |
+----------------------------------+----------------------------------+
| **``                             | Writes an attribute. Used by the |
| write(``\ ``attribute``\ ``)``** | GUI menu in conjunction with     |
|                                  | **``<script>``** to display the  |
|                                  | content of an attribute in a     |
|                                  | menu item.                       |
+----------------------------------+----------------------------------+
|                                  |                                  |
+----------------------------------+----------------------------------+
