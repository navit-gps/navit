#!/bin/bash

# This script automatically converts the rgb colors used for the polygons in the
# navit_layout_car_shipped.xml to the values appropriate for the dark layout in
# navit_layout_car_dark_shipped.xml.
# It's a bit of a mess, but gets the job done.

IFS='' # Keep whitespace

# Light and dark input files
ifl=navit_layout_car_shipped.xml
ifd=navit_layout_car_dark_shipped.xml

# Temporary dark output file
ofd=$ifd.new

# Create new temporary dark output file
[ -f $ofd ] && rm -f $ofd
touch $ofd

# Init: not in polygons layer
inpolygons=false
getpolygons=false

# Read dark input file
while read -r l
do

 # Check to see if inside polygons layer - open tag
 if [[ $l =~ .*\<layer\ name=\"polygons\"\>.* ]]; then
  inpolygons=true
  getpolygons=true
 fi

 # Check to see if inside polygons layer - close tag
 if [[ $l =~ .*\</layer\>.* ]]; then
  inpolygons=false
 fi

 # Trigger once when polygons open tag found in dark input file
 if [ $getpolygons == true ]; then

  getpolygons=false # Just triggered
  inpolygons=false  # Now used for light input file

  # Insert data from light input file
  while read -r l
  do

   # Check to see if inside polygons layer
   if [[ $l =~ .*\<layer\ name=\"polygons\"\>.* ]]; then
    inpolygons=true
   fi

   if [[ $l =~ .*\</layer\>.* ]]; then
    inpolygons=false
   fi

   # Inside polygons layer
   if [ $inpolygons == true ]; then

    if [[ $l =~ .*color=\"#[0-9a-fA-F]{6}.* ]]; then # Contains rgb(a) color
     coll=$(echo $l | cut -d# -f2 | cut -c-6)        # Get rgb color and convert
     cold=$(     printf '%02x' $(echo $(printf "%d" 0x${coll:0:2})/16+16 | bc))
     cold=$cold$(printf '%02x' $(echo $(printf "%d" 0x${coll:2:2})/10+14 | bc))
     cold=$cold$(printf '%02x' $(echo $(printf "%d" 0x${coll:4:2})/8+12  | bc))
     l=$(echo $l | sed "s/#$coll/#$cold/")           # Replace color
    fi

    l=$(echo $l | sed "s/<text /<text color=\"#55c4bd\" background_color=\"#000000\" /") # Add text color

    echo $l >> $ofd # (Modified) line from light input file to dark output file

   fi

  done < $ifl # Read light input file
  inpolygons=true # Done inserting, still in polygons layer in dark input file

 else

  # Outside polygons layer
  if [ $inpolygons == false ]; then
   echo $l >> $ofd # Just copy as-is to dark output file
  fi

 fi

done < $ifd # Read dark input file

cp $ofd $ifd # Replace
rm -f $ofd   # Clean up
