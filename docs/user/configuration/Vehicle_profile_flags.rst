.. _vehicle_profile_flags:

Vehicle profile flags
=====================

.. _flags_in_vehicleprofile:

flags in vehicleprofile
-----------------------

vehicleprofile defines routing rules for navit. A road must at least
have these flags to be considered for routing. The bit definition from

| ``0 (0x00000001): ONEWAY``
| ``1 (0x00000002): ONEWAYREV``
| ``2 (0x00000004): SEGMENTED``
| ``3 (0x00000008): ROUNDABOUT``
| ``4 (0x00000010): ROUNDABOUT_VALID``
| ``5 (0x00000020): ONEWAY_EXCEPTION``
| ``6 (0x00000040): SPEED_LIMIT``
| ``7 (0x00000080): RESERVED``
| ``8 (0x00000100): SIZE_OR_WEIGHT_LIMIT``
| ``9 (0x00000200): THROUGH_TRAFFIC_LIMIT``
| ``10(0x00000400): TOLL``
| ``11(0x00000800): SEASONAL``
| ``12(0x00001000): UNPAVED``
| ``13(0x00002000): FORD``
| ``14(0x00004000): UNDERGROUND``
| ``15(0x00008000): /*unused*/``
| ``16(0x00010000): /*unused*/``
| ``17(0x00020000): /*unused*/``
| ``18(0x00040000): /*unused*/``
| ``19(0x00080000): DANGEROUS_GOODS   ``
| ``20(0x00100000): EMERGENCY_VEHICLES``
| ``21(0x00200000): TRANSPORT_TRUCK   ``
| ``22(0x00400000): DELIVERY_TRUCK    ``
| ``23(0x00800000): PUBLIC_BUS        ``
| ``24(0x01000000): TAXI              ``
| ``25(0x02000000): HIGH_OCCUPANCY_CAR``
| ``26(0x04000000): CAR     ``
| ``27(0x08000000): MOTORCYCLE        ``
| ``28(0x10000000): MOPED             ``
| ``29(0x20000000): HORSE             ``
| ``30(0x40000000): BIKE              ``
| ``31(0x80000000): PEDESTRIAN``

As an example: 0x4000000 is for cars
