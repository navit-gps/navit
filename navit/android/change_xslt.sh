#! /bin/bash
###########################################
#
# change android car layout into XSLT for all densities
# handle with care!
#
# Zoff <zoff@zoff.cc> 2011-02-15
#
###########################################

in="android_layout_default_new.xml"
temp="/tmp/temp_layout.tmp"
out="android_layout_default_parsed.xml"

# setup temp file
rm -f "$temp"
cp -p "$in" "$temp"

REG0='0-{round(@1@-number($LAYOUT_001_ORDER_DELTA_1))}'
REG1='{round(@1@-number($LAYOUT_001_ORDER_DELTA_1))}-'
REG2='-{round(@1@-number($LAYOUT_001_ORDER_DELTA_1))}'
REG3='{round(@1@-number($LAYOUT_001_ORDER_DELTA_1))}-{round(@2@-number($LAYOUT_001_ORDER_DELTA_1))}'
REG4='{round(@1@-number($LAYOUT_001_ORDER_DELTA_1))}'

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
  a=$(echo "$REG0"|sed -e "s#@1@#$i#")
	sed -e "s#order=\"0-${i}\"#order=\"${a}\"#g" -i "$temp"
done

# dont change order="0-" values !!
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
  a=$(echo "$REG1"|sed -e "s#@1@#$i#")
	sed -e "s#order=\"${i}-\"#order=\"${a}\"#g" -i "$temp"
done

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
  a=$(echo "$REG2"|sed -e "s#@1@#$i#")
	sed -e "s#order=\"-${i}\"#order=\"${a}\"#g" -i "$temp"
done


for j in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
	for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
    a=$(echo "$REG3"|sed -e "s#@1@#$i#"|sed -e "s#@2@#$j#")
		sed -e "s#order=\"${i}-${j}\"#order=\"${a}\"#g" -i "$temp"
	done
done

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18;do
  a=$(echo "$REG4"|sed -e "s#@1@#$i#")
	sed -e "s#order=\"${i}\"#order=\"${a}\"#g" -i "$temp"
done
cp -p "$temp" "$out"
