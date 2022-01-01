.. _tmc_translation_table:

TMC Translation Table
=====================

This list shows how to translate TMC event and supplementary information
codes into codes from the `event list <Traffic_Event_List>`__ for the
proposed `Traffic Data Model <Traffic_Data_Model>`__.

Events
------

The biggest difference when translating events from TMC into our data
model is that TMC events carry a lot of implicit information, such as
whether an event is a forecast (as opposed to describing a current
situation) or whether it applies to one or both directions. In our data
model, these must be stated explicitly. As TMC was designed to be as
bandwidth-efficient as possible, frequently used combinations of events
and extra information were assigned event codes of their own. Our data
model requires that these “convenience events” be broken down.

Translation is straightforward for many events, as there is an event
code which directly corresponds to the TMC event. For others,
translation may be a bit more complex:

-  Some TMC codes may be represented by more than one event, shown as
   EVENT_ONE + EVENT_TWO.
-  Some TMC codes may imply a supplementary information item, shown as
   EVENT(S_SUPPL_INFO)
-  Some TMC codes may imply the forecast flag (which in our data model
   needs to be set explicitly), represented as EVENT(N=F)
-  Some TMC codes may imply a quantifier, speed or length of route
   affected; represented as EVENT(Q=x), EVENT(V=x km/h) or EVENT(L=x km)
-  In one case, two TMC events which differ merely in directionality
   were consolidated into one (*RESTRICTION_INTERMITTENT_CLOSURES*);
   take care to set directionality correctly here

This table may refer to event codes which are not in the current version
of the event list; those codes are safe to ignore for the moment. For
instance, *>INCIDENT_RESCUE + RESTRICTION_CLOSED* can just be mapped to
*>RESTRICTION_CLOSED* for now.

TMC event codes which are not in this list are ignored for now.

+------+--------------------------------------------------------------+
| TMC  | Event                                                        |
+======+==============================================================+
| 1    | CONGESTION_TRAFFIC_PROBLEM                                   |
+------+--------------------------------------------------------------+
| 2    | >CONGESTION_QUEUE + CONGESTION_STATIONARY_TRAFFIC_LIKELY     |
+------+--------------------------------------------------------------+
| 16   | >INCIDENT_RESCUE + RESTRICTION_CLOSED                        |
+------+--------------------------------------------------------------+
| 24   | >RESTRICTION_CLOSED(S_PLACE_BRIDGE)                          |
+------+--------------------------------------------------------------+
| 25   | >RESTRICTION_CLOSED(S_PLACE_TUNNEL)                          |
+------+--------------------------------------------------------------+
| 26   | >RESTRICTION_BLOCKED(S_PLACE_BRIDGE)                         |
+------+--------------------------------------------------------------+
| 27   | >RESTRICTION_BLOCKED(S_PLACE_TUNNEL)                         |
+------+--------------------------------------------------------------+
| 28   | >RESTRICTION_INTERMITTENT_CLOSURES(D=1)                      |
+------+--------------------------------------------------------------+
| 55   | >CONGESTION_TRAFFIC_PROBLEM(N=F)                             |
+------+--------------------------------------------------------------+
| 56   | >CONGESTION_TRAFFIC_CONGESTION(N=F)                          |
+------+--------------------------------------------------------------+
| 57   | >CONGESTION_NORMAL_TRAFFIC(N=F)                              |
+------+--------------------------------------------------------------+
| 70   | >CONGESTION_TRAFFIC_CONGESTION(V=10 km/h)                    |
+------+--------------------------------------------------------------+
| 71   | >CONGESTION_TRAFFIC_CONGESTION(V=20 km/h)                    |
+------+--------------------------------------------------------------+
| 72   | >CONGESTION_TRAFFIC_CONGESTION(V=30 km/h)                    |
+------+--------------------------------------------------------------+
| 73   | >CONGESTION_TRAFFIC_CONGESTION(V=40 km/h)                    |
+------+--------------------------------------------------------------+
| 74   | >CONGESTION_TRAFFIC_CONGESTION(V=50 km/h)                    |
+------+--------------------------------------------------------------+
| 75   | >CONGESTION_TRAFFIC_CONGESTION(V=60 km/h)                    |
+------+--------------------------------------------------------------+
| 76   | >CONGESTION_TRAFFIC_CONGESTION(V=70 km/h)                    |
+------+--------------------------------------------------------------+
| 80   | >CONGESTION_HEAVY_TRAFFIC(N=F)                               |
+------+--------------------------------------------------------------+
| 81   | >CONGESTION_TRAFFIC_CONGESTION(N=F)                          |
+------+--------------------------------------------------------------+
| 82   | >CONSTRUCTION_ROADWORKS + CONGESTION_HEAVY_TRAFFIC(N=F)      |
+------+--------------------------------------------------------------+
| 83   | >RESTRICTION_CLOSED_AHEAD + CONGESTION_HEAVY_TRAFFIC(N=F)    |
+------+--------------------------------------------------------------+
| 84   | >ACTIVITY_MAJOR_EVENT + CONGESTION_HEAVY_TRAFFIC(N=F)        |
+------+--------------------------------------------------------------+
| 85   | >ACTIVITY_SPORTS_EVENT + CONGESTION_HEAVY_TRAFFIC(N=F)       |
+------+--------------------------------------------------------------+
| 86   | >ACTIVITY_FAIR + CONGESTION_HEAVY_TRAFFIC(N=F)               |
+------+--------------------------------------------------------------+
| 87   | >SECURITY_EVACUATION + CONGESTION_HEAVY_TRAFFIC(N=F)         |
+------+--------------------------------------------------------------+
| 88   | CONGESTION_FORECAST_WITHDRAWN                                |
+------+--------------------------------------------------------------+
| 91   | >DELAY_DELAY(S_VEHICLE_CAR)                                  |
+------+--------------------------------------------------------------+
| 101  | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 102  | >CONGESTION_STATIONARY_TRAFFIC(L=1 km)                       |
+------+--------------------------------------------------------------+
| 103  | >CONGESTION_STATIONARY_TRAFFIC(L=2 km)                       |
+------+--------------------------------------------------------------+
| 104  | >CONGESTION_STATIONARY_TRAFFIC(L=4 km)                       |
+------+--------------------------------------------------------------+
| 105  | >CONGESTION_STATIONARY_TRAFFIC(L=6 km)                       |
+------+--------------------------------------------------------------+
| 106  | >CONGESTION_STATIONARY_TRAFFIC(L=10 km)                      |
+------+--------------------------------------------------------------+
| 107  | >CONGESTION_STATIONARY_TRAFFIC(N=F)                          |
+------+--------------------------------------------------------------+
| 108  | CONGESTION_QUEUE                                             |
+------+--------------------------------------------------------------+
| 109  | >CONGESTION_QUEUE(L=1 km)                                    |
+------+--------------------------------------------------------------+
| 110  | >CONGESTION_QUEUE(L=2 km)                                    |
+------+--------------------------------------------------------------+
| 111  | >CONGESTION_QUEUE(L=4 km)                                    |
+------+--------------------------------------------------------------+
| 112  | >CONGESTION_QUEUE(L=6 km)                                    |
+------+--------------------------------------------------------------+
| 113  | >CONGESTION_QUEUE(L=10 km)                                   |
+------+--------------------------------------------------------------+
| 114  | >CONGESTION_QUEUE(N=F)                                       |
+------+--------------------------------------------------------------+
| 115  | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 116  | >CONGESTION_SLOW_TRAFFIC(L=1 km)                             |
+------+--------------------------------------------------------------+
| 117  | >CONGESTION_SLOW_TRAFFIC(L=2 km)                             |
+------+--------------------------------------------------------------+
| 118  | >CONGESTION_SLOW_TRAFFIC(L=4 km)                             |
+------+--------------------------------------------------------------+
| 119  | >CONGESTION_SLOW_TRAFFIC(L=6 km)                             |
+------+--------------------------------------------------------------+
| 120  | >CONGESTION_SLOW_TRAFFIC(L=10 km)                            |
+------+--------------------------------------------------------------+
| 121  | >CONGESTION_SLOW_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 122  | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 123  | >CONGESTION_HEAVY_TRAFFIC(N=F)                               |
+------+--------------------------------------------------------------+
| 124  | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 125  | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 126  | CONGESTION_NONE                                              |
+------+--------------------------------------------------------------+
| 127  | CONGESTION_CLEARED                                           |
+------+--------------------------------------------------------------+
| 129  | >CONGESTION_STATIONARY_TRAFFIC(L=3 km)                       |
+------+--------------------------------------------------------------+
| 130  | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 131  | >CONGESTION_QUEUE(L=3 km)                                    |
+------+--------------------------------------------------------------+
| 132  | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 133  | CONGESTION_LONG_QUEUE                                        |
+------+--------------------------------------------------------------+
| 134  | >CONGESTION_SLOW_TRAFFIC(L=3 km)                             |
+------+--------------------------------------------------------------+
| 135  | CONGESTION_TRAFFIC_EASING                                    |
+------+--------------------------------------------------------------+
| 136  | CONGESTION_TRAFFIC_CONGESTION                                |
+------+--------------------------------------------------------------+
| 137  | CONGESTION_TRAFFIC_LIGHTER_THAN_NORMAL                       |
+------+--------------------------------------------------------------+
| 138  | >CONGESTION_QUEUE(S_INSTRUCTION_APPROACH_WITH_CARE)          |
+------+--------------------------------------------------------------+
| 139  | >CONGESTION_QUEUE(S_PLACE_BEND)                              |
+------+--------------------------------------------------------------+
| 140  | >CONGESTION_QUEUE(S_PLACE_HILL)                              |
+------+--------------------------------------------------------------+
| 142  | CONGESTION_TRAFFIC_HEAVIER_THAN_NORMAL                       |
+------+--------------------------------------------------------------+
| 143  | CONGESTION_TRAFFIC_MUCH_HEAVIER_THAN_NORMAL                  |
+------+--------------------------------------------------------------+
| 200  | >INCIDENT_MULTI_VEHICLE_ACCIDENT + DELAY_DELAY               |
+------+--------------------------------------------------------------+
| 215  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC           |
+------+--------------------------------------------------------------+
| 216  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=1 km)   |
+------+--------------------------------------------------------------+
| 217  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=2 km)   |
+------+--------------------------------------------------------------+
| 218  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=4 km)   |
+------+--------------------------------------------------------------+
| 219  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=6 km)   |
+------+--------------------------------------------------------------+
| 220  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=10 km)  |
+------+--------------------------------------------------------------+
| 221  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC_LIKELY    |
+------+--------------------------------------------------------------+
| 222  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE                        |
+------+--------------------------------------------------------------+
| 223  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=1 km)                |
+------+--------------------------------------------------------------+
| 224  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=2 km)                |
+------+--------------------------------------------------------------+
| 225  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=4 km)                |
+------+--------------------------------------------------------------+
| 226  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=6 km)                |
+------+--------------------------------------------------------------+
| 227  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=10 km)               |
+------+--------------------------------------------------------------+
| 228  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE_LIKELY                 |
+------+--------------------------------------------------------------+
| 229  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC                 |
+------+--------------------------------------------------------------+
| 230  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=1 km)         |
+------+--------------------------------------------------------------+
| 231  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=2 km)         |
+------+--------------------------------------------------------------+
| 232  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=4 km)         |
+------+--------------------------------------------------------------+
| 233  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=6 km)         |
+------+--------------------------------------------------------------+
| 234  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=10 km)        |
+------+--------------------------------------------------------------+
| 235  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(N=F)            |
+------+--------------------------------------------------------------+
| 236  | >INCIDENT_ACCIDENT + CONGESTION_HEAVY_TRAFFIC                |
+------+--------------------------------------------------------------+
| 237  | >INCIDENT_ACCIDENT + CONGESTION_HEAVY_TRAFFIC(N=F)           |
+------+--------------------------------------------------------------+
| 238  | >INCIDENT_ACCIDENT + CONGESTION_TRAFFIC_FLOWING_FREELY       |
+------+--------------------------------------------------------------+
| 239  | >INCIDENT_ACCIDENT + CONGESTION_TRAFFIC_BUILDING_UP          |
+------+--------------------------------------------------------------+
| 240  | >INCIDENT_ACCIDENT + RESTRICTION_CLOSED                      |
+------+--------------------------------------------------------------+
| 247  | >INCIDENT_ACCIDENT + DELAY_DELAY                             |
+------+--------------------------------------------------------------+
| 248  | >INCIDENT_ACCIDENT + DELAY_DELAY(N=F)                        |
+------+--------------------------------------------------------------+
| 249  | >INCIDENT_ACCIDENT + DELAY_LONG_DELAY                        |
+------+--------------------------------------------------------------+
| 250  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC      |
+------+--------------------------------------------------------------+
| 251  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=1  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 252  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=2  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 253  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=4  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 254  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=6  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 255  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=10 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 256  | >INCIDENT_RUBBERNECKING +                                    |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 257  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE                   |
+------+--------------------------------------------------------------+
| 258  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=1 km)           |
+------+--------------------------------------------------------------+
| 259  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=2 km)           |
+------+--------------------------------------------------------------+
| 260  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=4 km)           |
+------+--------------------------------------------------------------+
| 261  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=6 km)           |
+------+--------------------------------------------------------------+
| 262  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=10 km)          |
+------+--------------------------------------------------------------+
| 263  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE_LIKELY            |
+------+--------------------------------------------------------------+
| 264  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC            |
+------+--------------------------------------------------------------+
| 265  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=1 km)    |
+------+--------------------------------------------------------------+
| 266  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=2 km)    |
+------+--------------------------------------------------------------+
| 267  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=4 km)    |
+------+--------------------------------------------------------------+
| 268  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=6 km)    |
+------+--------------------------------------------------------------+
| 269  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=10 km)   |
+------+--------------------------------------------------------------+
| 270  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(N=F)       |
+------+--------------------------------------------------------------+
| 271  | >INCIDENT_RUBBERNECKING + CONGESTION_HEAVY_TRAFFIC           |
+------+--------------------------------------------------------------+
| 272  | >INCIDENT_RUBBERNECKING + CONGESTION_HEAVY_TRAFFIC(N=F)      |
+------+--------------------------------------------------------------+
| 274  | >INCIDENT_RUBBERNECKING + CONGESTION_TRAFFIC_BUILDING_UP     |
+------+--------------------------------------------------------------+
| 275  | >INCIDENT_RUBBERNECKING + DELAY_DELAY                        |
+------+--------------------------------------------------------------+
| 276  | >INCIDENT_RUBBERNECKING + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 277  | >INCIDENT_RUBBERNECKING + DELAY_LONG_DELAY                   |
+------+--------------------------------------------------------------+
| 278  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC            |
+------+--------------------------------------------------------------+
| 279  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=1 km)    |
+------+--------------------------------------------------------------+
| 280  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=2 km)    |
+------+--------------------------------------------------------------+
| 281  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=4 km)    |
+------+--------------------------------------------------------------+
| 282  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=6 km)    |
+------+--------------------------------------------------------------+
| 283  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=10 km)   |
+------+--------------------------------------------------------------+
| 284  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC_LIKELY     |
+------+--------------------------------------------------------------+
| 285  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE                         |
+------+--------------------------------------------------------------+
| 286  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=1 km)                 |
+------+--------------------------------------------------------------+
| 287  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=2 km)                 |
+------+--------------------------------------------------------------+
| 288  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=4 km)                 |
+------+--------------------------------------------------------------+
| 289  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=6 km)                 |
+------+--------------------------------------------------------------+
| 290  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=10 km)                |
+------+--------------------------------------------------------------+
| 291  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE_LIKELY                  |
+------+--------------------------------------------------------------+
| 292  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC                  |
+------+--------------------------------------------------------------+
| 293  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=1 km)          |
+------+--------------------------------------------------------------+
| 294  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=2 km)          |
+------+--------------------------------------------------------------+
| 295  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=4 km)          |
+------+--------------------------------------------------------------+
| 296  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=6 km)          |
+------+--------------------------------------------------------------+
| 297  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=10 km)         |
+------+--------------------------------------------------------------+
| 298  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(N=F)             |
+------+--------------------------------------------------------------+
| 299  | >HAZARD_SHED_LOAD + CONGESTION_HEAVY_TRAFFIC                 |
+------+--------------------------------------------------------------+
| 300  | >HAZARD_SHED_LOAD + CONGESTION_HEAVY_TRAFFIC(N=F)            |
+------+--------------------------------------------------------------+
| 301  | >HAZARD_SHED_LOAD + CONGESTION_TRAFFIC_FLOWING_FREELY        |
+------+--------------------------------------------------------------+
| 302  | >HAZARD_SHED_LOAD + CONGESTION_TRAFFIC_BUILDING_UP           |
+------+--------------------------------------------------------------+
| 303  | >HAZARD_SHED_LOAD + RESTRICTION_BLOCKED                      |
+------+--------------------------------------------------------------+
| 310  | >HAZARD_SHED_LOAD + DELAY_DELAY                              |
+------+--------------------------------------------------------------+
| 311  | >HAZARD_SHED_LOAD + DELAY_DELAY(N=F)                         |
+------+--------------------------------------------------------------+
| 312  | >HAZARD_SHED_LOAD + DELAY_LONG_DELAY                         |
+------+--------------------------------------------------------------+
| 313  | >INCIDENT_BROKEN_DOWN_VEHICLE +                              |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 314  | >INCIDENT_BROKEN_DOWN_VEHICLE +                              |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 315  | >INCIDENT_BROKEN_DOWN_VEHICLE + CONGESTION_QUEUE             |
+------+--------------------------------------------------------------+
| 316  | >INCIDENT_BROKEN_DOWN_VEHICLE + CONGESTION_QUEUE_LIKELY      |
+------+--------------------------------------------------------------+
| 317  | >INCIDENT_BROKEN_DOWN_VEHICLE + CONGESTION_SLOW_TRAFFIC      |
+------+--------------------------------------------------------------+
| 318  | >INCIDENT_BROKEN_DOWN_VEHICLE + CONGESTION_SLOW_TRAFFIC(N=F) |
+------+--------------------------------------------------------------+
| 319  | >INCIDENT_BROKEN_DOWN_VEHICLE + CONGESTION_HEAVY_TRAFFIC     |
+------+--------------------------------------------------------------+
| 320  | >INCIDENT_BROKEN_DOWN_VEHICLE +                              |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 321  | >INCIDENT_BROKEN_DOWN_VEHICLE +                              |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 322  | >INCIDENT_BROKEN_DOWN_VEHICLE +                              |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 323  | >INCIDENT_BROKEN_DOWN_VEHICLE + RESTRICTION_BLOCKED          |
+------+--------------------------------------------------------------+
| 330  | >INCIDENT_BROKEN_DOWN_VEHICLE + DELAY_DELAY                  |
+------+--------------------------------------------------------------+
| 331  | >INCIDENT_BROKEN_DOWN_VEHICLE + DELAY_DELAY(N=F)             |
+------+--------------------------------------------------------------+
| 332  | >INCIDENT_BROKEN_DOWN_VEHICLE + DELAY_LONG_DELAY             |
+------+--------------------------------------------------------------+
| 348  | >INCIDENT_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC(L=3 km)   |
+------+--------------------------------------------------------------+
| 349  | >INCIDENT_ACCIDENT + CONGESTION_QUEUE(L=3 km)                |
+------+--------------------------------------------------------------+
| 350  | >INCIDENT_ACCIDENT + CONGESTION_SLOW_TRAFFIC(L=3 km)         |
+------+--------------------------------------------------------------+
| 352  | >INCIDENT_RUBBERNECKING + CONGESTION_STATIONARY_TRAFFIC(L=3  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 353  | >INCIDENT_RUBBERNECKING + CONGESTION_QUEUE(L=3 km)           |
+------+--------------------------------------------------------------+
| 354  | >INCIDENT_RUBBERNECKING + CONGESTION_SLOW_TRAFFIC(L=3 km)    |
+------+--------------------------------------------------------------+
| 356  | >HAZARD_SHED_LOAD + CONGESTION_STATIONARY_TRAFFIC(L=3 km)    |
+------+--------------------------------------------------------------+
| 357  | >HAZARD_SHED_LOAD + CONGESTION_QUEUE(L=3 km)                 |
+------+--------------------------------------------------------------+
| 358  | >HAZARD_SHED_LOAD + CONGESTION_SLOW_TRAFFIC(L=3 km)          |
+------+--------------------------------------------------------------+
| 360  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_STATIONARY_TRAFFIC |
+------+--------------------------------------------------------------+
| 361  | >INCIDENT_OVERTURNED_VEHICLE +                               |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 362  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_QUEUE              |
+------+--------------------------------------------------------------+
| 363  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_QUEUE_LIKELY       |
+------+--------------------------------------------------------------+
| 364  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_SLOW_TRAFFIC       |
+------+--------------------------------------------------------------+
| 365  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_SLOW_TRAFFIC(N=F)  |
+------+--------------------------------------------------------------+
| 366  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_HEAVY_TRAFFIC      |
+------+--------------------------------------------------------------+
| 367  | >INCIDENT_OVERTURNED_VEHICLE + CONGESTION_HEAVY_TRAFFIC(N=F) |
+------+--------------------------------------------------------------+
| 368  | >INCIDENT_OVERTURNED_VEHICLE +                               |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 369  | >INCIDENT_OVERTURNED_VEHICLE + RESTRICTION_BLOCKED           |
+------+--------------------------------------------------------------+
| 375  | >INCIDENT_OVERTURNED_VEHICLE + DELAY_DELAY                   |
+------+--------------------------------------------------------------+
| 376  | >INCIDENT_OVERTURNED_VEHICLE + DELAY_DELAY(N=F)              |
+------+--------------------------------------------------------------+
| 377  | >INCIDENT_OVERTURNED_VEHICLE + DELAY_LONG_DELAY              |
+------+--------------------------------------------------------------+
| 379  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_STATIONARY_TRAFFIC   |
+------+--------------------------------------------------------------+
| 380  | >INCIDENT_EARLIER_ACCIDENT +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 381  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_QUEUE                |
+------+--------------------------------------------------------------+
| 382  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_QUEUE_LIKELY         |
+------+--------------------------------------------------------------+
| 383  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_SLOW_TRAFFIC         |
+------+--------------------------------------------------------------+
| 385  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_HEAVY_TRAFFIC        |
+------+--------------------------------------------------------------+
| 387  | >INCIDENT_EARLIER_ACCIDENT + CONGESTION_TRAFFIC_BUILDING_UP  |
+------+--------------------------------------------------------------+
| 388  | >INCIDENT_EARLIER_ACCIDENT + DELAY_DELAY                     |
+------+--------------------------------------------------------------+
| 390  | >INCIDENT_EARLIER_ACCIDENT + DELAY_LONG_DELAY                |
+------+--------------------------------------------------------------+
| 401  | >RESTRICTION_CLOSED                                          |
+------+--------------------------------------------------------------+
| 402  | RESTRICTION_BLOCKED                                          |
+------+--------------------------------------------------------------+
| 405  | >RESTRICTION_CLOSED(S_VEHICLE_THROUGH_TRAFFIC)               |
+------+--------------------------------------------------------------+
| 408  | RESTRICTION_RAMP_CLOSED                                      |
+------+--------------------------------------------------------------+
| 410  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_STATIONARY_TRAFFIC    |
+------+--------------------------------------------------------------+
| 411  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=1 km)                        |
+------+--------------------------------------------------------------+
| 412  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=2 km)                        |
+------+--------------------------------------------------------------+
| 413  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=4 km)                        |
+------+--------------------------------------------------------------+
| 414  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=6 km)                        |
+------+--------------------------------------------------------------+
| 415  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=10 km)                       |
+------+--------------------------------------------------------------+
| 416  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 417  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE                 |
+------+--------------------------------------------------------------+
| 418  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=1 km)         |
+------+--------------------------------------------------------------+
| 419  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=2 km)         |
+------+--------------------------------------------------------------+
| 420  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=4 km)         |
+------+--------------------------------------------------------------+
| 421  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=6 km)         |
+------+--------------------------------------------------------------+
| 422  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=10 km)        |
+------+--------------------------------------------------------------+
| 423  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE_LIKELY          |
+------+--------------------------------------------------------------+
| 424  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC          |
+------+--------------------------------------------------------------+
| 425  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=1 km)  |
+------+--------------------------------------------------------------+
| 426  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=2 km)  |
+------+--------------------------------------------------------------+
| 427  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=4 km)  |
+------+--------------------------------------------------------------+
| 428  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=6 km)  |
+------+--------------------------------------------------------------+
| 429  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=10 km) |
+------+--------------------------------------------------------------+
| 430  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(N=F)     |
+------+--------------------------------------------------------------+
| 431  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_HEAVY_TRAFFIC         |
+------+--------------------------------------------------------------+
| 432  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_HEAVY_TRAFFIC(N=F)    |
+------+--------------------------------------------------------------+
| 433  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 434  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_TRAFFIC_BUILDING_UP   |
+------+--------------------------------------------------------------+
| 435  | >RESTRICTION_CLOSED_AHEAD + DELAY_DELAY                      |
+------+--------------------------------------------------------------+
| 436  | >RESTRICTION_CLOSED_AHEAD + DELAY_DELAY(N=F)                 |
+------+--------------------------------------------------------------+
| 437  | >RESTRICTION_CLOSED_AHEAD + DELAY_LONG_DELAY                 |
+------+--------------------------------------------------------------+
| 438  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_STATIONARY_TRAFFIC   |
+------+--------------------------------------------------------------+
| 439  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=1 km)                        |
+------+--------------------------------------------------------------+
| 440  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=2 km)                        |
+------+--------------------------------------------------------------+
| 441  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=4 km)                        |
+------+--------------------------------------------------------------+
| 442  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=6 km)                        |
+------+--------------------------------------------------------------+
| 443  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=10 km)                       |
+------+--------------------------------------------------------------+
| 444  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 445  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE                |
+------+--------------------------------------------------------------+
| 446  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=1 km)        |
+------+--------------------------------------------------------------+
| 447  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=2 km)        |
+------+--------------------------------------------------------------+
| 448  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=4 km)        |
+------+--------------------------------------------------------------+
| 449  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=6 km)        |
+------+--------------------------------------------------------------+
| 450  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=10 km)       |
+------+--------------------------------------------------------------+
| 451  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE_LIKELY         |
+------+--------------------------------------------------------------+
| 452  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC         |
+------+--------------------------------------------------------------+
| 453  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=1 km) |
+------+--------------------------------------------------------------+
| 454  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=2 km) |
+------+--------------------------------------------------------------+
| 455  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=4 km) |
+------+--------------------------------------------------------------+
| 456  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=6 km) |
+------+--------------------------------------------------------------+
| 457  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=10    |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 458  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(N=F)    |
+------+--------------------------------------------------------------+
| 459  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_HEAVY_TRAFFIC        |
+------+--------------------------------------------------------------+
| 460  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_HEAVY_TRAFFIC(N=F)   |
+------+--------------------------------------------------------------+
| 461  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 462  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_TRAFFIC_BUILDING_UP  |
+------+--------------------------------------------------------------+
| 463  | >RESTRICTION_BLOCKED_AHEAD + DELAY_DELAY                     |
+------+--------------------------------------------------------------+
| 464  | >RESTRICTION_BLOCKED_AHEAD + DELAY_DELAY(N=F)                |
+------+--------------------------------------------------------------+
| 465  | >RESTRICTION_BLOCKED_AHEAD + DELAY_LONG_DELAY                |
+------+--------------------------------------------------------------+
| 466  | RESTRICTION_RAMP_REOPENED                                    |
+------+--------------------------------------------------------------+
| 467  | RESTRICTION_REOPENED                                         |
+------+--------------------------------------------------------------+
| 469  | RESTRICTION_CLOSED_AHEAD                                     |
+------+--------------------------------------------------------------+
| 470  | RESTRICTION_BLOCKED_AHEAD                                    |
+------+--------------------------------------------------------------+
| 472  | RESTRICTION_ENTRY_BLOCKED                                    |
+------+--------------------------------------------------------------+
| 473  | >RESTRICTION_ENTRY_BLOCKED                                   |
+------+--------------------------------------------------------------+
| 475  | RESTRICTION_EXIT_BLOCKED                                     |
+------+--------------------------------------------------------------+
| 476  | >RESTRICTION_EXIT_BLOCKED                                    |
+------+--------------------------------------------------------------+
| 477  | RESTRICTION_RAMP_BLOCKED                                     |
+------+--------------------------------------------------------------+
| 478  | >RESTR                                                       |
|      | ICTION_CARRIAGEWAY_CLOSED(S_POSITION_CONNECTING_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 479  | >RES                                                         |
|      | TRICTION_CARRIAGEWAY_CLOSED(S_POSITION_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 480  | >RESTRICTI                                                   |
|      | ON_CARRIAGEWAY_CLOSED(S_POSITION_RIGHT_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 481  | >RESTRICT                                                    |
|      | ION_CARRIAGEWAY_CLOSED(S_POSITION_LEFT_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 485  | >RESTRI                                                      |
|      | CTION_CARRIAGEWAY_BLOCKED(S_POSITION_CONNECTING_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 486  | >REST                                                        |
|      | RICTION_CARRIAGEWAY_BLOCKED(S_POSITION_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 487  | >RESTRICTIO                                                  |
|      | N_CARRIAGEWAY_BLOCKED(S_POSITION_RIGHT_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 488  | >RESTRICTI                                                   |
|      | ON_CARRIAGEWAY_BLOCKED(S_POSITION_LEFT_PARALLEL_CARRIAGEWAY) |
+------+--------------------------------------------------------------+
| 492  | >RESTRICTION_CLOSED(S_VEHICLE_MOTOR)                         |
+------+--------------------------------------------------------------+
| 495  | >RESTRICTION_CLOSED_AHEAD +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC(L=3 km)                        |
+------+--------------------------------------------------------------+
| 496  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_QUEUE(L=3 km)         |
+------+--------------------------------------------------------------+
| 497  | >RESTRICTION_CLOSED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=3 km)  |
+------+--------------------------------------------------------------+
| 498  | >RESTRICTION_BLOCKED_AHEAD +                                 |
|      | CONGESTION_STATIONARY_TRAFFIC(L=3 km)                        |
+------+--------------------------------------------------------------+
| 499  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_QUEUE(L=3 km)        |
+------+--------------------------------------------------------------+
| 513  | RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 521  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC     |
+------+--------------------------------------------------------------+
| 522  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC(L=1 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 523  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC(L=2 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 524  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC(L=4 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 525  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC(L=6 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 526  | >RESTRICTION_LANE_CLOSED +                                   |
|      | CONGESTION_STATIONARY_TRAFFIC(L=10 km)                       |
+------+--------------------------------------------------------------+
| 527  | >RESTRICTION_LANE_CLOSED +                                   |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 528  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE                  |
+------+--------------------------------------------------------------+
| 529  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=1 km)          |
+------+--------------------------------------------------------------+
| 530  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=2 km)          |
+------+--------------------------------------------------------------+
| 531  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=4 km)          |
+------+--------------------------------------------------------------+
| 532  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=6 km)          |
+------+--------------------------------------------------------------+
| 533  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=10 km)         |
+------+--------------------------------------------------------------+
| 534  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE_LIKELY           |
+------+--------------------------------------------------------------+
| 535  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC           |
+------+--------------------------------------------------------------+
| 536  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=1 km)   |
+------+--------------------------------------------------------------+
| 537  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=2 km)   |
+------+--------------------------------------------------------------+
| 538  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=4 km)   |
+------+--------------------------------------------------------------+
| 539  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=6 km)   |
+------+--------------------------------------------------------------+
| 540  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=10 km)  |
+------+--------------------------------------------------------------+
| 541  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(N=F)      |
+------+--------------------------------------------------------------+
| 542  | >RESTRICTION_LANE_CLOSED + CONGESTION_HEAVY_TRAFFIC          |
+------+--------------------------------------------------------------+
| 543  | >RESTRICTION_LANE_CLOSED + CONGESTION_HEAVY_TRAFFIC(N=F)     |
+------+--------------------------------------------------------------+
| 544  | >RESTRICTION_LANE_CLOSED + CONGESTION_TRAFFIC_FLOWING_FREELY |
+------+--------------------------------------------------------------+
| 545  | >RESTRICTION_LANE_CLOSED + CONGESTION_TRAFFIC_BUILDING_UP    |
+------+--------------------------------------------------------------+
| 546  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 547  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 548  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) + CONGESTION_QUEUE       |
+------+--------------------------------------------------------------+
| 549  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 550  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 551  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 552  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 553  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 554  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 555  | >RESTRICTION_REDUCED_LANES(Q=$Q, 1) +                        |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 556  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 557  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 558  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) + CONGESTION_QUEUE       |
+------+--------------------------------------------------------------+
| 559  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 560  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 561  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 562  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 563  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 564  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 565  | >RESTRICTION_REDUCED_LANES(Q=$Q, 2) +                        |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 566  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 567  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 568  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) + CONGESTION_QUEUE       |
+------+--------------------------------------------------------------+
| 569  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 570  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 571  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 572  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 573  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 574  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 575  | >RESTRICTION_REDUCED_LANES(Q=$Q, 3) +                        |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 576  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC      |
+------+--------------------------------------------------------------+
| 577  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=1  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 578  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=2  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 579  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=4  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 580  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=6  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 581  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=10 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 582  | >RESTRICTION_CONTRAFLOW +                                    |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 583  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE                   |
+------+--------------------------------------------------------------+
| 584  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=1 km)           |
+------+--------------------------------------------------------------+
| 585  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=2 km)           |
+------+--------------------------------------------------------------+
| 586  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=4 km)           |
+------+--------------------------------------------------------------+
| 587  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=6 km)           |
+------+--------------------------------------------------------------+
| 588  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=10 km)          |
+------+--------------------------------------------------------------+
| 589  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE_LIKELY            |
+------+--------------------------------------------------------------+
| 590  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC            |
+------+--------------------------------------------------------------+
| 591  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=1 km)    |
+------+--------------------------------------------------------------+
| 592  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=2 km)    |
+------+--------------------------------------------------------------+
| 593  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=4 km)    |
+------+--------------------------------------------------------------+
| 594  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=6 km)    |
+------+--------------------------------------------------------------+
| 595  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=10 km)   |
+------+--------------------------------------------------------------+
| 596  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(N=F)(N=F)  |
+------+--------------------------------------------------------------+
| 597  | >RESTRICTION_CONTRAFLOW + CONGESTION_HEAVY_TRAFFIC           |
+------+--------------------------------------------------------------+
| 598  | >RESTRICTION_CONTRAFLOW + CONGESTION_HEAVY_TRAFFIC(N=F)(N=F) |
+------+--------------------------------------------------------------+
| 599  | >RESTRICTION_CONTRAFLOW + CONGESTION_TRAFFIC_FLOWING_FREELY  |
+------+--------------------------------------------------------------+
| 600  | >RESTRICTION_CONTRAFLOW + CONGESTION_TRAFFIC_BUILDING_UP     |
+------+--------------------------------------------------------------+
| 604  | >RESTRICTION_NARROW_LANES + CONGESTION_STATIONARY_TRAFFIC    |
+------+--------------------------------------------------------------+
| 605  | >RESTRICTION_NARROW_LANES +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 606  | >RESTRICTION_NARROW_LANES + CONGESTION_QUEUE                 |
+------+--------------------------------------------------------------+
| 607  | >RESTRICTION_NARROW_LANES + CONGESTION_QUEUE_LIKELY          |
+------+--------------------------------------------------------------+
| 608  | >RESTRICTION_NARROW_LANES + CONGESTION_SLOW_TRAFFIC          |
+------+--------------------------------------------------------------+
| 609  | >RESTRICTION_NARROW_LANES + CONGESTION_SLOW_TRAFFIC(N=F)     |
+------+--------------------------------------------------------------+
| 610  | >RESTRICTION_NARROW_LANES + CONGESTION_HEAVY_TRAFFIC         |
+------+--------------------------------------------------------------+
| 611  | >RESTRICTION_NARROW_LANES + CONGESTION_HEAVY_TRAFFIC(N=F)    |
+------+--------------------------------------------------------------+
| 612  | >RESTRICTION_NARROW_LANES +                                  |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 613  | >RESTRICTION_NARROW_LANES + CONGESTION_TRAFFIC_BUILDING_UP   |
+------+--------------------------------------------------------------+
| 614  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 615  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 616  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_QUEUE                                             |
+------+--------------------------------------------------------------+
| 617  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 618  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 619  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 620  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 621  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 622  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 623  | >RESTRICTION_CONTRAFLOW + RESTRICTION_NARROW_LANES +         |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 626  | >RESTRICTION_BLOCKED_AHEAD + CONGESTION_SLOW_TRAFFIC(L=3 km) |
+------+--------------------------------------------------------------+
| 627  | >RESTRICTION_CLOSED(S_VEHICLE_WITHOUT_CAT)                   |
+------+--------------------------------------------------------------+
| 628  | >RESTRICTION_CLOSED(S_VEHICLE_EVEN_PLATE)                    |
+------+--------------------------------------------------------------+
| 629  | >RESTRICTION_CLOSED(S_VEHICLE_ODD_PLATE)                     |
+------+--------------------------------------------------------------+
| 630  | RESTRICTION_OPEN                                             |
+------+--------------------------------------------------------------+
| 631  | RESTRICTION_ROAD_CLEARED                                     |
+------+--------------------------------------------------------------+
| 632  | RESTRICTION_ENTRY_REOPENED                                   |
+------+--------------------------------------------------------------+
| 633  | RESTRICTION_EXIT_REOPENED                                    |
+------+--------------------------------------------------------------+
| 634  | RESTRICTION_ALL_CARRIAGEWAYS_REOPENED                        |
+------+--------------------------------------------------------------+
| 635  | >RESTRICTION_REOPENED(S_VEHICLE_MOTOR)                       |
+------+--------------------------------------------------------------+
| 636  | RESTRICTION_ACCESS_RESTRICTIONS_LIFTED                       |
+------+--------------------------------------------------------------+
| 651  | >RESTRICTION_LANE_CLOSED + CONGESTION_STATIONARY_TRAFFIC(L=3 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 652  | >RESTRICTION_LANE_CLOSED + CONGESTION_QUEUE(L=3 km)          |
+------+--------------------------------------------------------------+
| 653  | >RESTRICTION_LANE_CLOSED + CONGESTION_SLOW_TRAFFIC(L=3 km)   |
+------+--------------------------------------------------------------+
| 654  | >RESTRICTION_CONTRAFLOW + CONGESTION_STATIONARY_TRAFFIC(Q=3  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 655  | >RESTRICTION_CONTRAFLOW + CONGESTION_QUEUE(Q=3 km)           |
+------+--------------------------------------------------------------+
| 656  | >RESTRICTION_CONTRAFLOW + CONGESTION_SLOW_TRAFFIC(Q=3 km)    |
+------+--------------------------------------------------------------+
| 663  | RESTRICTION_ALL_CARRIAGEWAYS_CLEARED                         |
+------+--------------------------------------------------------------+
| 664  | RESTRICTION_CARRIAGEWAY_CLOSED                               |
+------+--------------------------------------------------------------+
| 665  | >RESTRICTION_CLOSED(D=2)                                     |
+------+--------------------------------------------------------------+
| 666  | RESTRICTION_INTERMITTENT_CLOSURES                            |
+------+--------------------------------------------------------------+
| 680  | >RESTRICTION_REOPENED(S_VEHICLE_THROUGH_TRAFFIC)             |
+------+--------------------------------------------------------------+
| 710  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC      |
+------+--------------------------------------------------------------+
| 711  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=1  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 712  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=2  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 713  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=4  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 714  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=6  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 715  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=10 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 716  | >CONSTRUCTION_ROADWORKS +                                    |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 717  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE                   |
+------+--------------------------------------------------------------+
| 718  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=1 km)           |
+------+--------------------------------------------------------------+
| 719  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=2 km)           |
+------+--------------------------------------------------------------+
| 720  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=4 km)           |
+------+--------------------------------------------------------------+
| 721  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=6 km)           |
+------+--------------------------------------------------------------+
| 722  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=10 km)          |
+------+--------------------------------------------------------------+
| 723  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE_LIKELY            |
+------+--------------------------------------------------------------+
| 724  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC            |
+------+--------------------------------------------------------------+
| 725  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=1 km)    |
+------+--------------------------------------------------------------+
| 726  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=2 km)    |
+------+--------------------------------------------------------------+
| 727  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=4 km)    |
+------+--------------------------------------------------------------+
| 728  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=6 km)    |
+------+--------------------------------------------------------------+
| 729  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=10 km)   |
+------+--------------------------------------------------------------+
| 730  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(N=F)       |
+------+--------------------------------------------------------------+
| 731  | >CONSTRUCTION_ROADWORKS + CONGESTION_HEAVY_TRAFFIC           |
+------+--------------------------------------------------------------+
| 732  | >CONSTRUCTION_ROADWORKS + CONGESTION_HEAVY_TRAFFIC(N=F)      |
+------+--------------------------------------------------------------+
| 733  | >CONSTRUCTION_ROADWORKS + CONGESTION_TRAFFIC_FLOWING_FREELY  |
+------+--------------------------------------------------------------+
| 734  | >CONSTRUCTION_ROADWORKS + CONGESTION_TRAFFIC_BUILDING_UP     |
+------+--------------------------------------------------------------+
| 735  | >CONSTRUCTION_ROADWORKS + RESTRICTION_CLOSED                 |
+------+--------------------------------------------------------------+
| 742  | >CONSTRUCTION_ROADWORKS +                                    |
|      | RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 747  | >CONSTRUCTION_ROADWORKS + DELAY_DELAY                        |
+------+--------------------------------------------------------------+
| 748  | >CONSTRUCTION_ROADWORKS + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 749  | >CONSTRUCTION_ROADWORKS + DELAY_LONG_DELAY                   |
+------+--------------------------------------------------------------+
| 750  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 751  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=1 km)                        |
+------+--------------------------------------------------------------+
| 752  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=2 km)                        |
+------+--------------------------------------------------------------+
| 753  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=4 km)                        |
+------+--------------------------------------------------------------+
| 754  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=6 km)                        |
+------+--------------------------------------------------------------+
| 755  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=10 km)                       |
+------+--------------------------------------------------------------+
| 756  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 757  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE            |
+------+--------------------------------------------------------------+
| 758  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=1 km)    |
+------+--------------------------------------------------------------+
| 759  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=2 km)    |
+------+--------------------------------------------------------------+
| 760  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=4 km)    |
+------+--------------------------------------------------------------+
| 761  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=6 km)    |
+------+--------------------------------------------------------------+
| 762  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=10 km)   |
+------+--------------------------------------------------------------+
| 763  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE_LIKELY     |
+------+--------------------------------------------------------------+
| 764  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC     |
+------+--------------------------------------------------------------+
| 765  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC(L=1 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 766  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC(L=2 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 767  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC(L=4 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 768  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC(L=6 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 769  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_SLOW_TRAFFIC(L=10 km)                             |
+------+--------------------------------------------------------------+
| 770  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 771  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_HEAVY_TRAFFIC    |
+------+--------------------------------------------------------------+
| 772  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 773  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 774  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 775  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 780  | >CONSTRUCTION_RESURFACING_WORK + DELAY_DELAY                 |
+------+--------------------------------------------------------------+
| 781  | >CONSTRUCTION_RESURFACING_WORK + DELAY_DELAY(N=F)            |
+------+--------------------------------------------------------------+
| 782  | >CONSTRUCTION_RESURFACING_WORK + DELAY_LONG_DELAY            |
+------+--------------------------------------------------------------+
| 783  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_STATIONARY_TRAFFIC    |
+------+--------------------------------------------------------------+
| 784  | >HAZARD_ROAD_MARKING_WORK +                                  |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 785  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_QUEUE                 |
+------+--------------------------------------------------------------+
| 786  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_QUEUE_LIKELY          |
+------+--------------------------------------------------------------+
| 787  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_SLOW_TRAFFIC          |
+------+--------------------------------------------------------------+
| 788  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_SLOW_TRAFFIC(N=F)     |
+------+--------------------------------------------------------------+
| 789  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_HEAVY_TRAFFIC         |
+------+--------------------------------------------------------------+
| 790  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_HEAVY_TRAFFIC(N=F)    |
+------+--------------------------------------------------------------+
| 791  | >HAZARD_ROAD_MARKING_WORK +                                  |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 792  | >HAZARD_ROAD_MARKING_WORK + CONGESTION_TRAFFIC_BUILDING_UP   |
+------+--------------------------------------------------------------+
| 799  | >CONSTRUCTION_BRIDGE_DEMOLITION + RESTRICTION_CLOSED         |
+------+--------------------------------------------------------------+
| 812  | >CONSTRUCTION_ROADWORKS + CONGESTION_STATIONARY_TRAFFIC(L=3  |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 813  | >CONSTRUCTION_ROADWORKS + CONGESTION_QUEUE(L=3 km)           |
+------+--------------------------------------------------------------+
| 814  | >CONSTRUCTION_ROADWORKS + CONGESTION_SLOW_TRAFFIC(L=3 km)    |
+------+--------------------------------------------------------------+
| 818  | >CONSTRUCTION_RESURFACING_WORK +                             |
|      | CONGESTION_STATIONARY_TRAFFIC(L=3 km)                        |
+------+--------------------------------------------------------------+
| 819  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_QUEUE(L=3 km)    |
+------+--------------------------------------------------------------+
| 820  | >CONSTRUCTION_RESURFACING_WORK + CONGESTION_SLOW_TRAFFIC(L=3 |
|      | km)                                                          |
+------+--------------------------------------------------------------+
| 825  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 826  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 827  | >HAZARD_SLOW_MOVING_MAINTENANCE + CONGESTION_QUEUE           |
+------+--------------------------------------------------------------+
| 828  | >HAZARD_SLOW_MOVING_MAINTENANCE + CONGESTION_QUEUE_LIKELY    |
+------+--------------------------------------------------------------+
| 829  | >HAZARD_SLOW_MOVING_MAINTENANCE + CONGESTION_SLOW_TRAFFIC    |
+------+--------------------------------------------------------------+
| 830  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 831  | >HAZARD_SLOW_MOVING_MAINTENANCE + CONGESTION_HEAVY_TRAFFIC   |
+------+--------------------------------------------------------------+
| 832  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 833  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 834  | >HAZARD_SLOW_MOVING_MAINTENANCE +                            |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 840  | >CONSTRUCTION_WATER_MAINS + DELAY_DELAY                      |
+------+--------------------------------------------------------------+
| 841  | >CONSTRUCTION_WATER_MAINS + DELAY_DELAY(N=F)                 |
+------+--------------------------------------------------------------+
| 842  | >CONSTRUCTION_WATER_MAINS + DELAY_LONG_DELAY                 |
+------+--------------------------------------------------------------+
| 843  | >CONSTRUCTION_GAS_MAINS + DELAY_DELAY                        |
+------+--------------------------------------------------------------+
| 844  | >CONSTRUCTION_GAS_MAINS + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 845  | >CONSTRUCTION_GAS_MAINS + DELAY_LONG_DELAY                   |
+------+--------------------------------------------------------------+
| 846  | >CONSTRUCTION_BURIED_CABLES + DELAY_DELAY                    |
+------+--------------------------------------------------------------+
| 847  | >CONSTRUCTION_BURIED_CABLES + DELAY_DELAY(N=F)               |
+------+--------------------------------------------------------------+
| 848  | >CONSTRUCTION_BURIED_CABLES + DELAY_LONG_DELAY               |
+------+--------------------------------------------------------------+
| 849  | >CONSTRUCTION_BURIED_SERVICES + DELAY_DELAY                  |
+------+--------------------------------------------------------------+
| 850  | >CONSTRUCTION_BURIED_SERVICES + DELAY_DELAY(N=F)             |
+------+--------------------------------------------------------------+
| 851  | >CONSTRUCTION_BURIED_SERVICES + DELAY_LONG_DELAY             |
+------+--------------------------------------------------------------+
| 925  | >HAZARD_STORM_DAMAGE + RESTRICTION_BLOCKED                   |
+------+--------------------------------------------------------------+
| 926  | >HAZARD_FALLEN_TREE + RESTRICTION_BLOCKED                    |
+------+--------------------------------------------------------------+
| 928  | >HAZARD_FLOODING + CONGESTION_STATIONARY_TRAFFIC             |
+------+--------------------------------------------------------------+
| 929  | >HAZARD_FLOODING + CONGESTION_STATIONARY_TRAFFIC_LIKELY      |
+------+--------------------------------------------------------------+
| 930  | >HAZARD_FLOODING + CONGESTION_QUEUE                          |
+------+--------------------------------------------------------------+
| 931  | >HAZARD_FLOODING + CONGESTION_QUEUE_LIKELY                   |
+------+--------------------------------------------------------------+
| 932  | >HAZARD_FLOODING + CONGESTION_SLOW_TRAFFIC                   |
+------+--------------------------------------------------------------+
| 933  | >HAZARD_FLOODING + CONGESTION_SLOW_TRAFFIC(N=F)              |
+------+--------------------------------------------------------------+
| 934  | >HAZARD_FLOODING + CONGESTION_HEAVY_TRAFFIC                  |
+------+--------------------------------------------------------------+
| 935  | >HAZARD_FLOODING + CONGESTION_HEAVY_TRAFFIC(N=F)             |
+------+--------------------------------------------------------------+
| 936  | >HAZARD_FLOODING + CONGESTION_TRAFFIC_FLOWING_FREELY         |
+------+--------------------------------------------------------------+
| 937  | >HAZARD_FLOODING + CONGESTION_TRAFFIC_BUILDING_UP            |
+------+--------------------------------------------------------------+
| 938  | >HAZARD_FLOODING + RESTRICTION_CLOSED                        |
+------+--------------------------------------------------------------+
| 939  | >HAZARD_FLOODING + DELAY_DELAY                               |
+------+--------------------------------------------------------------+
| 940  | >HAZARD_FLOODING + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 941  | >HAZARD_FLOODING + DELAY_LONG_DELAY                          |
+------+--------------------------------------------------------------+
| 943  | >HAZARD_AVALANCHE + RESTRICTION_CLOSED                       |
+------+--------------------------------------------------------------+
| 945  | >HAZARD_ROCKFALLS + RESTRICTION_CLOSED                       |
+------+--------------------------------------------------------------+
| 947  | >HAZARD_LANDSLIPS + RESTRICTION_CLOSED                       |
+------+--------------------------------------------------------------+
| 949  | >HAZARD_SUBSIDENCE + RESTRICTION_CLOSED                      |
+------+--------------------------------------------------------------+
| 950  | >HAZARD_SUBSIDENCE +                                         |
|      | RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 956  | >HAZARD_COLLAPSED_SEWER + RESTRICTION_CLOSED                 |
+------+--------------------------------------------------------------+
| 957  | >HAZARD_BURST_WATER_MAIN + RESTRICTION_CLOSED                |
+------+--------------------------------------------------------------+
| 958  | >HAZARD_BURST_WATER_MAIN + DELAY_DELAY                       |
+------+--------------------------------------------------------------+
| 959  | >HAZARD_BURST_WATER_MAIN + DELAY_DELAY(N=F)                  |
+------+--------------------------------------------------------------+
| 960  | >HAZARD_BURST_WATER_MAIN + DELAY_LONG_DELAY                  |
+------+--------------------------------------------------------------+
| 961  | >HAZARD_GAS_LEAK + RESTRICTION_CLOSED                        |
+------+--------------------------------------------------------------+
| 962  | >HAZARD_GAS_LEAK + DELAY_DELAY                               |
+------+--------------------------------------------------------------+
| 963  | >HAZARD_GAS_LEAK + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 964  | >HAZARD_GAS_LEAK + DELAY_LONG_DELAY                          |
+------+--------------------------------------------------------------+
| 965  | >HAZARD_SERIOUS_FIRE + RESTRICTION_CLOSED                    |
+------+--------------------------------------------------------------+
| 966  | >HAZARD_SERIOUS_FIRE + DELAY_DELAY                           |
+------+--------------------------------------------------------------+
| 967  | >HAZARD_SERIOUS_FIRE + DELAY_DELAY(N=F)                      |
+------+--------------------------------------------------------------+
| 968  | >HAZARD_SERIOUS_FIRE + DELAY_LONG_DELAY                      |
+------+--------------------------------------------------------------+
| 969  | >INCIDENT_CLEARANCE + RESTRICTION_CLOSED                     |
+------+--------------------------------------------------------------+
| 980  | >HAZARD_OBSTRUCTION + RESTRICTION_BLOCKED                    |
+------+--------------------------------------------------------------+
| 982  | >HAZARD_SPILLAGE + RESTRICTION_BLOCKED                       |
+------+--------------------------------------------------------------+
| 987  | >HAZARD_FALLEN_POWER_CABLES + RESTRICTION_BLOCKED            |
+------+--------------------------------------------------------------+
| 993  | >HAZARD_AVALANCHE_RISK + RESTRICTION_CLOSED                  |
+------+--------------------------------------------------------------+
| 995  | >HAZARD_ICE_BUILDUP + RESTRICTION_CLOSED                     |
+------+--------------------------------------------------------------+
| 997  | >HAZARD_ICE_BUILDUP +                                        |
|      | RESTRICTION_SINGLE_ALTERNATE_LINE_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 1027 | >HAZARD_COLLAPSED_SEWER + DELAY_DELAY                        |
+------+--------------------------------------------------------------+
| 1028 | >HAZARD_COLLAPSED_SEWER + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 1029 | >HAZARD_COLLAPSED_SEWER + DELAY_LONG_DELAY                   |
+------+--------------------------------------------------------------+
| 1212 | >WEATHER_STRONG_WIND +                                       |
|      | RESTRICTION_CLOSED(S_VEHICLE_HIGH_SIDED)                     |
+------+--------------------------------------------------------------+
| 1215 | >RESTRICTION_REOPENED(S_VEHICLE_HIGH_SIDED)                  |
+------+--------------------------------------------------------------+
| 1338 | >RESTRICTION_SMOG_ALERT +                                    |
|      | RESTRICTION_CLOSED(S_VEHICLE_MOTOR)                          |
+------+--------------------------------------------------------------+
| 1485 | >SECURITY_INCIDENT + RESTRICTION_CLOSED                      |
+------+--------------------------------------------------------------+
| 1486 | >SECURITY_INCIDENT + DELAY_DELAY                             |
+------+--------------------------------------------------------------+
| 1487 | >SECURITY_INCIDENT + DELAY_DELAY(N=F)                        |
+------+--------------------------------------------------------------+
| 1488 | >SECURITY_INCIDENT + DELAY_LONG_DELAY                        |
+------+--------------------------------------------------------------+
| 1489 | >SECURITY_POLICE_CHECKPOINT + DELAY_DELAY                    |
+------+--------------------------------------------------------------+
| 1490 | >SECURITY_POLICE_CHECKPOINT + DELAY_DELAY(N=F)               |
+------+--------------------------------------------------------------+
| 1491 | >SECURITY_POLICE_CHECKPOINT + DELAY_LONG_DELAY               |
+------+--------------------------------------------------------------+
| 1495 | >SECURITY_EVACUATION + CONGESTION_HEAVY_TRAFFIC              |
+------+--------------------------------------------------------------+
| 1517 | >ACTIVITY_MAJOR_EVENT + CONGESTION_STATIONARY_TRAFFIC        |
+------+--------------------------------------------------------------+
| 1518 | >ACTIVITY_MAJOR_EVENT + CONGESTION_STATIONARY_TRAFFIC_LIKELY |
+------+--------------------------------------------------------------+
| 1519 | >ACTIVITY_MAJOR_EVENT + CONGESTION_QUEUE                     |
+------+--------------------------------------------------------------+
| 1520 | >ACTIVITY_MAJOR_EVENT + CONGESTION_QUEUE_LIKELY              |
+------+--------------------------------------------------------------+
| 1521 | >ACTIVITY_MAJOR_EVENT + CONGESTION_SLOW_TRAFFIC              |
+------+--------------------------------------------------------------+
| 1522 | >ACTIVITY_MAJOR_EVENT + CONGESTION_SLOW_TRAFFIC(N=F)         |
+------+--------------------------------------------------------------+
| 1523 | >ACTIVITY_MAJOR_EVENT + CONGESTION_HEAVY_TRAFFIC             |
+------+--------------------------------------------------------------+
| 1524 | >ACTIVITY_MAJOR_EVENT + CONGESTION_HEAVY_TRAFFIC(N=F)        |
+------+--------------------------------------------------------------+
| 1525 | >ACTIVITY_MAJOR_EVENT + CONGESTION_TRAFFIC_FLOWING_FREELY    |
+------+--------------------------------------------------------------+
| 1526 | >ACTIVITY_MAJOR_EVENT + CONGESTION_TRAFFIC_BUILDING_UP       |
+------+--------------------------------------------------------------+
| 1527 | >ACTIVITY_MAJOR_EVENT + RESTRICTION_CLOSED                   |
+------+--------------------------------------------------------------+
| 1528 | >ACTIVITY_MAJOR_EVENT + DELAY_DELAY                          |
+------+--------------------------------------------------------------+
| 1529 | >ACTIVITY_MAJOR_EVENT + DELAY_DELAY(N=F)                     |
+------+--------------------------------------------------------------+
| 1530 | >ACTIVITY_MAJOR_EVENT + DELAY_LONG_DELAY                     |
+------+--------------------------------------------------------------+
| 1531 | >ACTIVITY_SPORTS_EVENT + CONGESTION_STATIONARY_TRAFFIC       |
+------+--------------------------------------------------------------+
| 1532 | >ACTIVITY_SPORTS_EVENT +                                     |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 1533 | >ACTIVITY_SPORTS_EVENT + CONGESTION_QUEUE                    |
+------+--------------------------------------------------------------+
| 1534 | >ACTIVITY_SPORTS_EVENT + CONGESTION_QUEUE_LIKELY             |
+------+--------------------------------------------------------------+
| 1535 | >ACTIVITY_SPORTS_EVENT + CONGESTION_SLOW_TRAFFIC             |
+------+--------------------------------------------------------------+
| 1536 | >ACTIVITY_SPORTS_EVENT + CONGESTION_SLOW_TRAFFIC(N=F)        |
+------+--------------------------------------------------------------+
| 1537 | >ACTIVITY_SPORTS_EVENT + CONGESTION_HEAVY_TRAFFIC            |
+------+--------------------------------------------------------------+
| 1538 | >ACTIVITY_SPORTS_EVENT + CONGESTION_HEAVY_TRAFFIC(N=F)       |
+------+--------------------------------------------------------------+
| 1539 | >ACTIVITY_SPORTS_EVENT + CONGESTION_TRAFFIC_FLOWING_FREELY   |
+------+--------------------------------------------------------------+
| 1540 | >ACTIVITY_SPORTS_EVENT + CONGESTION_TRAFFIC_BUILDING_UP      |
+------+--------------------------------------------------------------+
| 1541 | >ACTIVITY_SPORTS_EVENT + RESTRICTION_CLOSED                  |
+------+--------------------------------------------------------------+
| 1542 | >ACTIVITY_SPORTS_EVENT + DELAY_DELAY                         |
+------+--------------------------------------------------------------+
| 1543 | >ACTIVITY_SPORTS_EVENT + DELAY_DELAY(N=F)                    |
+------+--------------------------------------------------------------+
| 1544 | >ACTIVITY_SPORTS_EVENT + DELAY_LONG_DELAY                    |
+------+--------------------------------------------------------------+
| 1545 | >ACTIVITY_FAIR + CONGESTION_STATIONARY_TRAFFIC               |
+------+--------------------------------------------------------------+
| 1546 | >ACTIVITY_FAIR + CONGESTION_STATIONARY_TRAFFIC_LIKELY        |
+------+--------------------------------------------------------------+
| 1547 | >ACTIVITY_FAIR + CONGESTION_QUEUE                            |
+------+--------------------------------------------------------------+
| 1548 | >ACTIVITY_FAIR + CONGESTION_QUEUE_LIKELY                     |
+------+--------------------------------------------------------------+
| 1549 | >ACTIVITY_FAIR + CONGESTION_SLOW_TRAFFIC                     |
+------+--------------------------------------------------------------+
| 1550 | >ACTIVITY_FAIR + CONGESTION_SLOW_TRAFFIC(N=F)                |
+------+--------------------------------------------------------------+
| 1551 | >ACTIVITY_FAIR + CONGESTION_HEAVY_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 1552 | >ACTIVITY_FAIR + CONGESTION_HEAVY_TRAFFIC(N=F)               |
+------+--------------------------------------------------------------+
| 1553 | >ACTIVITY_FAIR + CONGESTION_TRAFFIC_FLOWING_FREELY           |
+------+--------------------------------------------------------------+
| 1554 | >ACTIVITY_FAIR + CONGESTION_TRAFFIC_BUILDING_UP              |
+------+--------------------------------------------------------------+
| 1555 | >ACTIVITY_FAIR + RESTRICTION_CLOSED                          |
+------+--------------------------------------------------------------+
| 1556 | >ACTIVITY_FAIR + DELAY_DELAY                                 |
+------+--------------------------------------------------------------+
| 1557 | >ACTIVITY_FAIR + DELAY_DELAY(N=F)                            |
+------+--------------------------------------------------------------+
| 1558 | >ACTIVITY_FAIR + DELAY_LONG_DELAY                            |
+------+--------------------------------------------------------------+
| 1559 | >ACTIVITY_PARADE + RESTRICTION_CLOSED                        |
+------+--------------------------------------------------------------+
| 1560 | >ACTIVITY_PARADE + DELAY_DELAY                               |
+------+--------------------------------------------------------------+
| 1561 | >ACTIVITY_PARADE + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 1562 | >ACTIVITY_PARADE + DELAY_LONG_DELAY                          |
+------+--------------------------------------------------------------+
| 1563 | >ACTIVITY_STRIKE + RESTRICTION_CLOSED                        |
+------+--------------------------------------------------------------+
| 1564 | >ACTIVITY_STRIKE + DELAY_DELAY                               |
+------+--------------------------------------------------------------+
| 1565 | >ACTIVITY_STRIKE + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 1566 | >ACTIVITY_STRIKE + DELAY_LONG_DELAY                          |
+------+--------------------------------------------------------------+
| 1567 | >ACTIVITY_DEMONSTRATION + RESTRICTION_CLOSED                 |
+------+--------------------------------------------------------------+
| 1568 | >ACTIVITY_DEMONSTRATION + DELAY_DELAY                        |
+------+--------------------------------------------------------------+
| 1569 | >ACTIVITY_DEMONSTRATION + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 1570 | >ACTIVITY_DEMONSTRATION + DELAY_LONG_DELAY                   |
+------+--------------------------------------------------------------+
| 1571 | >SECURITY_ALERT + CONGESTION_STATIONARY_TRAFFIC              |
+------+--------------------------------------------------------------+
| 1572 | >SECURITY_ALERT + CONGESTION_STATIONARY_TRAFFIC_LIKELY       |
+------+--------------------------------------------------------------+
| 1573 | >SECURITY_ALERT + CONGESTION_QUEUE                           |
+------+--------------------------------------------------------------+
| 1574 | >SECURITY_ALERT + CONGESTION_QUEUE_LIKELY                    |
+------+--------------------------------------------------------------+
| 1575 | >SECURITY_ALERT + CONGESTION_SLOW_TRAFFIC                    |
+------+--------------------------------------------------------------+
| 1576 | >SECURITY_ALERT + CONGESTION_SLOW_TRAFFIC(N=F)               |
+------+--------------------------------------------------------------+
| 1577 | >SECURITY_ALERT + CONGESTION_HEAVY_TRAFFIC                   |
+------+--------------------------------------------------------------+
| 1578 | >SECURITY_ALERT + CONGESTION_HEAVY_TRAFFIC(N=F)              |
+------+--------------------------------------------------------------+
| 1579 | >SECURITY_ALERT + CONGESTION_TRAFFIC_BUILDING_UP             |
+------+--------------------------------------------------------------+
| 1580 | >SECURITY_ALERT + RESTRICTION_CLOSED                         |
+------+--------------------------------------------------------------+
| 1581 | >SECURITY_ALERT + DELAY_DELAY                                |
+------+--------------------------------------------------------------+
| 1582 | >SECURITY_ALERT + DELAY_DELAY(N=F)                           |
+------+--------------------------------------------------------------+
| 1583 | >SECURITY_ALERT + DELAY_LONG_DELAY                           |
+------+--------------------------------------------------------------+
| 1584 | CONGESTION_NORMAL_TRAFFIC                                    |
+------+--------------------------------------------------------------+
| 1586 | >SECURITY_ALERT + CONGESTION_TRAFFIC_FLOWING_FREELY          |
+------+--------------------------------------------------------------+
| 1601 | DELAY_DELAY                                                  |
+------+--------------------------------------------------------------+
| 1602 | >DELAY_DELAY(Q=15 min)                                       |
+------+--------------------------------------------------------------+
| 1603 | >DELAY_DELAY(Q=30 min)                                       |
+------+--------------------------------------------------------------+
| 1604 | >DELAY_DELAY(Q=1 h)                                          |
+------+--------------------------------------------------------------+
| 1605 | >DELAY_DELAY(Q=2 h)                                          |
+------+--------------------------------------------------------------+
| 1606 | DELAY_SEVERAL_HOURS                                          |
+------+--------------------------------------------------------------+
| 1607 | >DELAY_DELAY(N=F)                                            |
+------+--------------------------------------------------------------+
| 1608 | DELAY_LONG_DELAY                                             |
+------+--------------------------------------------------------------+
| 1609 | >DELAY_DELAY(S_VEHICLE_HEAVY)                                |
+------+--------------------------------------------------------------+
| 1610 | >DELAY_DELAY(Q=15 min, S_VEHICLE_HGV)                        |
+------+--------------------------------------------------------------+
| 1611 | >DELAY_DELAY(Q=30 min, S_VEHICLE_HGV)                        |
+------+--------------------------------------------------------------+
| 1612 | >DELAY_DELAY(Q=1 h, S_VEHICLE_HGV)                           |
+------+--------------------------------------------------------------+
| 1613 | >DELAY_DELAY(Q=2 h, S_VEHICLE_HGV)                           |
+------+--------------------------------------------------------------+
| 1614 | >DELAY_SEVERAL_HOURS(S_VEHICLE_HGV)                          |
+------+--------------------------------------------------------------+
| 1621 | >DELAY_DELAY(Q=5 min)                                        |
+------+--------------------------------------------------------------+
| 1622 | >DELAY_DELAY(Q=10 min)                                       |
+------+--------------------------------------------------------------+
| 1623 | >DELAY_DELAY(Q=20 min)                                       |
+------+--------------------------------------------------------------+
| 1624 | >DELAY_DELAY(Q=25 min)                                       |
+------+--------------------------------------------------------------+
| 1625 | >DELAY_DELAY(Q=40 min)                                       |
+------+--------------------------------------------------------------+
| 1626 | >DELAY_DELAY(Q=50 min)                                       |
+------+--------------------------------------------------------------+
| 1627 | >DELAY_DELAY(Q=90 min)                                       |
+------+--------------------------------------------------------------+
| 1628 | >DELAY_DELAY(Q=3 h)                                          |
+------+--------------------------------------------------------------+
| 1629 | >DELAY_DELAY(Q=4 h)                                          |
+------+--------------------------------------------------------------+
| 1630 | >DELAY_DELAY(Q=5 h)                                          |
+------+--------------------------------------------------------------+
| 1631 | DELAY_VERY_LONG_DELAY                                        |
+------+--------------------------------------------------------------+
| 1632 | DELAY_UNCERTAIN_DURATION                                     |
+------+--------------------------------------------------------------+
| 1642 | >DELAY_DELAY(S_VEHICLE_HGV)                                  |
+------+--------------------------------------------------------------+
| 1643 | >DELAY_DELAY(S_VEHICLE_BUS)                                  |
+------+--------------------------------------------------------------+
| 1648 | DELAY_CLEARANCE                                              |
+------+--------------------------------------------------------------+
| 1650 | DELAY_DELAY_POSSIBLE                                         |
+------+--------------------------------------------------------------+
| 1653 | >DELAY_LONG_DELAY(N=F)                                       |
+------+--------------------------------------------------------------+
| 1654 | >DELAY_VERY_LONG_DELAY(N=F)                                  |
+------+--------------------------------------------------------------+
| 1657 | >TRANSPORT_IRREGULAR_RAIL_SERVICE + DELAY_DELAY              |
+------+--------------------------------------------------------------+
| 1658 | >TRANSPORT_IRREGULAR_BUS_SERVICE + DELAY_DELAY               |
+------+--------------------------------------------------------------+
| 1663 | >DELAY_CLEARANCE(N=F)                                        |
+------+--------------------------------------------------------------+
| 1680 | >DELAY_DELAY(N=F)                                            |
+------+--------------------------------------------------------------+
| 1681 | >DELAY_SEVERAL_HOURS(N=F)                                    |
+------+--------------------------------------------------------------+
| 1682 | >RESTRICTION_CLOSED_AHEAD + DELAY_DELAY(N=F)                 |
+------+--------------------------------------------------------------+
| 1683 | >CONSTRUCTION_ROADWORKS + DELAY_DELAY(N=F)                   |
+------+--------------------------------------------------------------+
| 1684 | >HAZARD_FLOODING + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 1685 | >ACTIVITY_MAJOR_EVENT + DELAY_DELAY(N=F)                     |
+------+--------------------------------------------------------------+
| 1686 | >ACTIVITY_STRIKE + DELAY_DELAY(N=F)                          |
+------+--------------------------------------------------------------+
| 1687 | >DELAY_SEVERAL_HOURS(N=F)(S_VEHICLE_HGV)                     |
+------+--------------------------------------------------------------+
| 1688 | >DELAY_LONG_DELAY(N=F)                                       |
+------+--------------------------------------------------------------+
| 1689 | >DELAY_VERY_LONG_DELAY(N=F)                                  |
+------+--------------------------------------------------------------+
| 1690 | DELAY_FORECAST_WITHDRAWN                                     |
+------+--------------------------------------------------------------+
| 1740 | >HAZARD_ABNORMAL_LOAD + CONGESTION_SLOW_TRAFFIC +            |
|      | DELAY_DELAY                                                  |
+------+--------------------------------------------------------------+
| 1741 | >HAZARD_CONVOY + CONGESTION_SLOW_TRAFFIC + DELAY_DELAY       |
+------+--------------------------------------------------------------+
| 1756 | >HAZARD_ABNORMAL_LOAD + DELAY_DELAY                          |
+------+--------------------------------------------------------------+
| 1757 | >HAZARD_ABNORMAL_LOAD + DELAY_DELAY(N=F)                     |
+------+--------------------------------------------------------------+
| 1758 | >HAZARD_ABNORMAL_LOAD + DELAY_LONG_DELAY                     |
+------+--------------------------------------------------------------+
| 1759 | >HAZARD_CONVOY + DELAY_DELAY                                 |
+------+--------------------------------------------------------------+
| 1760 | >HAZARD_CONVOY + DELAY_DELAY(N=F)                            |
+------+--------------------------------------------------------------+
| 1761 | >HAZARD_CONVOY + DELAY_LONG_DELAY                            |
+------+--------------------------------------------------------------+
| 1807 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 1808 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 1809 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_QUEUE                                             |
+------+--------------------------------------------------------------+
| 1810 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 1811 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 1812 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 1813 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 1814 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 1815 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 1816 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 1817 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING + DELAY_DELAY   |
+------+--------------------------------------------------------------+
| 1818 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | DELAY_DELAY(N=F)                                             |
+------+--------------------------------------------------------------+
| 1819 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_NOT_WORKING +               |
|      | DELAY_LONG_DELAY                                             |
+------+--------------------------------------------------------------+
| 1820 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_STATIONARY_TRAFFIC                                |
+------+--------------------------------------------------------------+
| 1821 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_STATIONARY_TRAFFIC_LIKELY                         |
+------+--------------------------------------------------------------+
| 1822 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE + CONGESTION_QUEUE  |
+------+--------------------------------------------------------------+
| 1823 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_QUEUE_LIKELY                                      |
+------+--------------------------------------------------------------+
| 1824 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_SLOW_TRAFFIC                                      |
+------+--------------------------------------------------------------+
| 1825 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_SLOW_TRAFFIC(N=F)                                 |
+------+--------------------------------------------------------------+
| 1826 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_HEAVY_TRAFFIC                                     |
+------+--------------------------------------------------------------+
| 1827 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_HEAVY_TRAFFIC(N=F)                                |
+------+--------------------------------------------------------------+
| 1828 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_TRAFFIC_FLOWING_FREELY                            |
+------+--------------------------------------------------------------+
| 1829 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE +                   |
|      | CONGESTION_TRAFFIC_BUILDING_UP                               |
+------+--------------------------------------------------------------+
| 1830 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE + DELAY_DELAY       |
+------+--------------------------------------------------------------+
| 1831 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE + DELAY_DELAY(N=F)  |
+------+--------------------------------------------------------------+
| 1832 | >EQUIPMENT_STATUS_LEVEL_CROSSING_FAILURE + DELAY_LONG_DELAY  |
+------+--------------------------------------------------------------+
| 1858 | >HAZARD_SNOWPLOW + DELAY_DELAY                               |
+------+--------------------------------------------------------------+
| 1868 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_WORKING_INCORRECTLY +       |
|      | DELAY_DELAY                                                  |
+------+--------------------------------------------------------------+
| 1869 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_WORKING_INCORRECTLY +       |
|      | DELAY_DELAY(N=F)                                             |
+------+--------------------------------------------------------------+
| 1870 | >EQUIPMENT_STATUS_TRAFFIC_LIGHTS_WORKING_INCORRECTLY +       |
|      | DELAY_LONG_DELAY                                             |
+------+--------------------------------------------------------------+
| 1876 | >EQUIPMENT_STATUS_TEMPORARY_TRAFFIC_LIGHTS_NOT_WORKING +     |
|      | DELAY_DELAY                                                  |
+------+--------------------------------------------------------------+
| 1877 | >EQUIPMENT_STATUS_TEMPORARY_TRAFFIC_LIGHTS_NOT_WORKING +     |
|      | DELAY_DELAY(N=F)                                             |
+------+--------------------------------------------------------------+
| 1878 | >EQUIPMENT_STATUS_TEMPORARY_TRAFFIC_LIGHTS_NOT_WORKING +     |
|      | DELAY_LONG_DELAY                                             |
+------+--------------------------------------------------------------+
| 1880 | >EQUIPMENT_STATUS_TRAFFIC_SIGNAL_CONTROL_NOT_WORKING +       |
|      | DELAY_DELAY                                                  |
+------+--------------------------------------------------------------+
| 1884 | >EQUIPMENT_STATUS_TRAFFIC_SIGNAL_CONTROL_NOT_WORKING +       |
|      | DELAY_DELAY(N=F)                                             |
+------+--------------------------------------------------------------+
| 1885 | >EQUIPMENT_STATUS_TRAFFIC_SIGNAL_CONTROL_NOT_WORKING +       |
|      | DELAY_LONG_DELAY                                             |
+------+--------------------------------------------------------------+
| 2000 | >RESTRICTION_SMOG_ALERT + RESTRICTION_CLOSED                 |
+------+--------------------------------------------------------------+

.. _supplementary_information:

Supplementary Information
-------------------------

Translation is straightforward for the most part, with many TMC codes
having a direct complement in the Supplementary Information list. There
are a few exceptions:

-  Unlike in TMC, Supplementary Information items can have quantifiers.
-  Some TMC SI codes map to events in our traffic data model.

TMC SI codes which are not in the table are ignored for the moment.

=== =======================================
TMC Supplementary Information
=== =======================================
48  S_PLACE_BRIDGE
52  >RESTRICTION_BATCH_SERVICE
71  S_VEHICLE_CAR
75  S_VEHICLE_CAR_WITH_TRAILER
76  S_VEHICLE_CAR_WITH_CARAVAN
77  S_VEHICLE_WITH_TRAILER
78  S_VEHICLE_HGV
79  S_VEHICLE_BUS
82  S_VEHICLE_HAZMAT
86  S_VEHICLE_ALL
102 >RESTRICTION_SPEED_LIMIT
161 S_PLACE_TUNNEL
162 S_PLACE_RAMP
235 >S_TENDENCY_QUEUE_DECREASING(Q=10 km/h)
236 >S_TENDENCY_QUEUE_DECREASING(Q=20 km/h)
237 >S_TENDENCY_QUEUE_DECREASING(Q=30 km/h)
238 >S_TENDENCY_QUEUE_DECREASING(Q=40 km/h)
241 S_PLACE_ROADWORKS
245 >S_TENDENCY_QUEUE_INCREASING
246 >S_TENDENCY_QUEUE_INCREASING(Q=10 km/h)
247 >S_TENDENCY_QUEUE_INCREASING(Q=20 km/h)
248 >S_TENDENCY_QUEUE_INCREASING(Q=30 km/h)
249 >S_TENDENCY_QUEUE_INCREASING(Q=40 km/h)
255 >S_TENDENCY_QUEUE_DECREASING
=== =======================================
