#!/bin/bash

# This script automatically converts the rgb colors used for the layers in
# navit_layout_car_shipped.xml to the values appropriate for the dark layout in
# navit_layout_car_dark_shipped.xml.
# It's a bit of a mess, but gets the job done.

# Variables
textcolor='55c4bd'

# Settings
IFS='' # Keep whitespace

# Light and dark input files
ifl=navit_layout_car_shipped.xml
ifd=navit_layout_car_dark_shipped.xml

# Temporary dark output file
ofd=$ifd.new

# Iterate over all layers in dark input file
grep \<layer\ name=\" $ifd | cut -d\" -f2 | while read -r layer
do

 echo $layer

 # Create new temporary dark output file
 [ -f $ofd ] && rm -f $ofd
 touch $ofd

 # Init: not in layer
 inlayer=false
 getlayer=false

 # Read dark input file
 while read -r l
 do

  # Check to see if inside layer - open tag
  if [[ $l =~ .*\<layer\ name=\"$layer\"\>.* ]]; then
   inlayer=true
   getlayer=true
  fi

  # Check to see if inside layer - close tag
  if [[ $l =~ .*\</layer\>.* ]]; then
   inlayer=false
  fi

  # Trigger once when open tag found in dark input file
  if [ $getlayer == true ]; then

   getlayer=false # Just triggered
   inlayer=false  # Now used for light input file

   # Insert data from light input file
   while read -r l
   do

    # Check to see if inside layer - open tag
    if [[ $l =~ .*\<layer\ name=\"$layer\"\>.* ]]; then
     inlayer=true
    fi

    # Check to see if inside layer - close tag
    if [[ $l =~ .*\</layer\>.* ]]; then
     inlayer=false
    fi

    # Inside layer
    if [ $inlayer == true ]; then

     # Modify color
     if [[ $l =~ .*color=\"#[0-9a-fA-F]{6}.* ]]; then # Contains rgb(a) color

      # Get hexadecimal color
      coll=$(echo $l | cut -d# -f2 | cut -c-6)

      # Get color values in decimal
      cr=$(printf "%d" 0x${coll:0:2})
      cg=$(printf "%d" 0x${coll:2:2})
      cb=$(printf "%d" 0x${coll:4:2})

      # Modify decimal color values
      crn=$(echo $cr/16+$cg/32+$cb/32+16 | bc)
      cgn=$(echo $cr/24+$cg/12+$cb/24+12 | bc)
      cbn=$(echo $cr/16+$cg/16+$cb/ 8+ 8 | bc)

      # Convert new decimal color values to hexadecimal
      cold=$(     printf '%02x' $crn)
      cold=$cold$(printf '%02x' $cgn)
      cold=$cold$(printf '%02x' $cbn)

      # Replace old color with new hexadecimal color values
      l=$(echo $l | sed "s/#$coll/#$cold/")

     fi

     # Miscellaneous modifications
     l=$(echo $l | sed -r "s/_bk\./_wh\./") # Black to white icons
     l=$(echo $l | sed -r "s/(<text )/\1color=\"#$textcolor\" background_color=\"#000000\" /") # Add text color
     l=$(echo $l | sed -r "s/(<circle color=\"#[0-9a-fA-F]{6,8}\" )/\1background_color=\"#$textcolor\" /") # Add circle bg color

     echo $l >> $ofd # Modified line from light input file to dark output file

    fi

   done < $ifl # Read light input file

   inlayer=true # Done inserting, still in layer in dark input file

  else

   # Outside layer
   if [ $inlayer == false ]; then
    echo $l >> $ofd # Just copy as-is to dark output file
   fi

  fi

 done < $ifd # Read dark input file

 cp $ofd $ifd # Replace
 rm -f $ofd   # Clean up

done # Next layer in dark input file
