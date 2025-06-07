Maps
====

Navit can use a variety of maps from a number of sources. This includes
free and commercial material, that can be read using different map
drivers which allow different sets of functionality. The visual design
of maps can be adjusted with a `layout <layout>`__.

The following matrix shows at-a-glance the features that each supported
map type has.

+-------+-------+-------+-------+-------+-------+-------+-------+
| pro   | cov   | age   | bi    | g     | mg    | poi_g | tex   |
| vider | erage |       | nfile | armin |       | eodow | tfile |
|       |       |       |       |       |       | nload |       |
+=======+=======+=======+=======+=======+=======+=======+=======+
| `Op   | g     | daily | ..    | ..    |       |       |       |
| enStr | lobal |       | raw:: | raw:: |       |       |       |
| eetMa |       |       |  medi |  medi |       |       |       |
| p <Op |       |       | awiki | awiki |       |       |       |
| enStr |       |       |       |       |       |       |       |
| eetMa |       |       |    {{ |    {{ |       |       |       |
| p>`__ |       |       | yes}} | yes}} |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+
| `G    | g     |       | ..    | ..    |       |       |       |
| armin | lobal |       | raw:: | raw:: |       |       |       |
| m     |       |       |  medi |  medi |       |       |       |
| aps < |       |       | awiki | awiki |       |       |       |
| Garmi |       |       |       |       |       |       |       |
| n_map |       |       |    {{ |    {{ |       |       |       |
| s>`__ |       |       | yes}} | yes}} |       |       |       |
| 1     |       |       |       |       |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+
| `     | EU    | >3    |       |       | ..    |       |       |
| Marco |       | years |       |       | raw:: |       |       |
| Polo  |       |       |       |       |  medi |       |       |
| Gr    |       |       |       |       | awiki |       |       |
| osser |       |       |       |       |       |       |       |
| Re    |       |       |       |       |    {{ |       |       |
| isepl |       |       |       |       | yes}} |       |       |
| aner  |       |       |       |       |       |       |       |
| <Marc |       |       |       |       |       |       |       |
| o_Pol |       |       |       |       |       |       |       |
| o_Gro |       |       |       |       |       |       |       |
| sser_ |       |       |       |       |       |       |       |
| Reise |       |       |       |       |       |       |       |
| plane |       |       |       |       |       |       |       |
| r>`__ |       |       |       |       |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+
| `Ro   | EU    | >4    |       |       |       |       |       |
| utenp |       | years |       |       |       |       |       |
| laner |       |       |       |       |       |       |       |
| E     |       |       |       |       |       |       |       |
| uropa |       |       |       |       |       |       |       |
| 2007  |       |       |       |       |       |       |       |
|  <Rou |       |       |       |       |       |       |       |
| tenpl |       |       |       |       |       |       |       |
| aner_ |       |       |       |       |       |       |       |
| Europ |       |       |       |       |       |       |       |
| a_200 |       |       |       |       |       |       |       |
| 7>`__ |       |       |       |       |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+
| `Map  | EU    | >4    |       |       |       |       |       |
| +     |       | years |       |       |       |       |       |
| Ro    |       |       |       |       |       |       |       |
| ute < |       |       |       |       |       |       |       |
| Map_+ |       |       |       |       |       |       |       |
| _Rout |       |       |       |       |       |       |       |
| e>`__ |       |       |       |       |       |       |       |
+-------+-------+-------+-------+-------+-------+-------+-------+

1 Garmin maps need to be unlocked

Here you can see, which map driver is needed and what it supports.

+-------------+-------------+-------------+-------------+-------------+
| driver      | browsing    | searching   | routing     | status      |
+=============+=============+=============+=============+=============+
| `binfile <  | .. raw:     | .. raw:     | .. raw:     | .. raw:     |
| binfile>`__ | : mediawiki | : mediawiki | : mediawiki | : mediawiki |
|             |             |             |             |             |
|             |    {{yes}}  |    {{yes}}  |    {{yes}}  |    {{y      |
|             |             |             |             | es|active}} |
+-------------+-------------+-------------+-------------+-------------+
| `           |             |             |             |             |
| textfile <t |             |             |             |             |
| extfile>`__ |             |             |             |             |
+-------------+-------------+-------------+-------------+-------------+
| `c          |             |             |             |             |
| sv <csv>`__ |             |             |             |             |
+-------------+-------------+-------------+-------------+-------------+
| `Garmin     | .. raw:     | .. raw:     | .. raw:     | .. raw:     |
| maps <Garm  | : mediawiki | : mediawiki | : mediawiki | : mediawiki |
| in_maps>`__ |             |             |             |             |
|             |    {{yes}}  |    {{no}}   |    {{no}}   |             |
|             |             |             |             | {{no|dead}} |
+-------------+-------------+-------------+-------------+-------------+
| `Marco Polo | .. raw:     | .. raw:     | .. raw:     | .. raw:     |
| Grosser     | : mediawiki | : mediawiki | : mediawiki | : mediawiki |
| Rei         |             |             |             |             |
| seplaner <M |    {{yes}}  |    {{yes}}  |    {{yes}}  |    {{       |
| arco_Polo_G |             |             |             | no|frozen}} |
| rosser_Reis |             |             |             |             |
| eplaner>`__ |             |             |             |             |
+-------------+-------------+-------------+-------------+-------------+
| `R          | .. raw:     | .. raw:     | .. raw:     | .. raw:     |
| outenplaner | : mediawiki | : mediawiki | : mediawiki | : mediawiki |
| Europa      |             |             |             |             |
| 2           |    {{yes}}  |    {{yes}}  |    {{yes}}  |    {{       |
| 007 <Routen |             |             |             | no|frozen}} |
| planer_Euro |             |             |             |             |
| pa_2007>`__ |             |             |             |             |
+-------------+-------------+-------------+-------------+-------------+
