## Coordinates in Navit

Various parts of Navit will read geographical coordinates provided as text:

-  the [textfile](https://wiki.navit-project.org/index.php/Textfile) map format
-  the "center=" attribute in the configuration file
-  some Navit commands (e.g. set_position), which can be invoked via the [internal GUI](https://wiki.navit-project.org/index.php/Internal_GUI) or the [Dbus](https://wiki.navit-project.org/index.php/Dbus) bindings
-  the files for bookmarks and last map position (bookmarks.txt and center.txt)

This page documents the coordinate systems and formats that Navit will accept.

## Supported coordinate systems and formats

### Longitude / Latitude in decimal degrees

Longitude / latitude in degrees can be specified as signed decimal fractions:

```
-33.3553 6.334
```

That would be about 33° West, 6° North. Note that in this format longitude comes first.  The coordinates are assumed to be based on WGS84 (the coordinate system used by the GPS system, and by practically all common navigation systems).

### Latitude / Longitude in degrees and minutes

Latitude / Longitude can also be specified in degress and minutes with compass directions (N/S, E/W):

```
4808 N 1134 E
```

Latitude and longitude are multiplied by 100, so the position above corresponds to 48°8' N, 11°34' (Munich).

For greater precision you can write the minutes as decimal fractions:

```
4808.2356 N 1134.5252 E
```

That is 48°8.2356' N 11°34.5252' E, the center of the Marienplatz in Munich.

Notes:

-  This format is rather unusual (because it uses arcminutes, but  not arcseconds). It is probably easier to just use decimal fractions of  degrees.
-  The spaces are relevant for parsing. Use exactly one space between the number and the letter N/S/E/W.

### Cartesian coordinates

Internally, Navit uses a cartesian coordinate system induced by a Mercator projection. Coordinates are written as hexadecimal integers:

```
0x13a3d7 0x5d6d6d
```

or specifying a projection:

```
mg: 0x13a3d7 0x5d6d6d
```

That is again 48°8.2356' N 11°34.5252' E. The part up to and including the colon is optional, it names the projection to use. Possible values:

-  mg - the projection used by Map&Guide (the default)
-  garmin - "Garmin" projection (TODO: When would it be useful?)

This format is used internally by Navit, but is probably not very useful for other purposes.

### UTM coordinates

Navit can read coordinates in the [Universal Transverse Mercator coordinate system](http://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system) (UTM).

```
utm32U: 674499.306 5328063.675
```

```
utmref32UPU:74499.306 28063.675
```



## Development notes

The coordinates are parsed in function coord_parse() in coord.c. This code is used everywhere where Navit parses coordinates, except for the manual coordinate input in the Internal GUI (which uses its own format and parsing function).