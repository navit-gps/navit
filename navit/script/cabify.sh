#! /bin/bash

function check_pocketcab()
{
	if ! which pocketpc-cab &> /dev/null
	then
		echo "You don't have pocketpc-cab installed or not in PATH"
		exit
	fi
}

SRCDIR="../.."
BASEDIR="navit"
MAPSDIR=""
CABLIST="/tmp/navit.lst"
CABNAME=""

check_pocketcab

if [ "$1" == "" ]
then
	echo "$0 cabname [sourcedir [pocketinstalldir [navit.xml mapsdir]]]"
	exit
else
	CABNAME="$1"
fi
[ "$2" != "" ] && SRCDIR="$2"
[ "$3" != "" ] && BASEDIR="$3"
if [ "$4" != "" ];
then
	NAVITXML="$4"
else
	NAVITXML=""
fi
[ "$5" != "" ] && MAPSDIR="$5"

echo "Source dir: $SRCDIR"
echo "PocketPc dir: $BASEDIR"
[ "$NAVITXML" != "" ] && echo "Navitxml: $NAVITXML"
[ "$MAPSDIR" != "" ] && echo "Maps: $MAPSDIR"

echo -n > $CABLIST.$$

for i in $SRCDIR/locale/*/*/*.mo
do
  bn=$(basename "$i")
	d=${i##$SRCDIR/}
	echo "$i $BASEDIR/$d" >> $CABLIST.$$
done

for i in $SRCDIR/navit/icons/*.xpm
do
  bn=$(basename "$i")
	echo "$i $BASEDIR/icons/" >> $CABLIST.$$
done

echo "$SRCDIR/navit/navit.exe $BASEDIR/" >> $CABLIST.$$
if [ "$NAVITXML" != ""  ]
then
echo "$NAVITXML $BASEDIR/" >> $CABLIST.$$
fi
if [ "$MAPSDIR" != "" ]
then
for i in $MAPSDIR/*.bin
do
    bn=$(basename "$i")
	echo "$i $BASEDIR/maps/$bn" >> $CABLIST.$$
done
for i in $MAPSDIR/*.txt
do
    bn=$(basename "$i")
	echo "$i $BASEDIR/maps/$bn" >> $CABLIST.$$
done
for i in $MAPSDIR/*.img
do
  bn=$(basename "$i")
	echo "$i $BASEDIR/maps/$bn" >> $CABLIST.$$
done
fi
pocketpc-cab -p "Navit Team" -a "Navit" $CABLIST.$$ $CABNAME
rm $CABLIST.$$

