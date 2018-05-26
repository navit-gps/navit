#!/bin/sh

# This script scans the navit sources for attributes that
# remained in attr_def.h but are no longer used.

ATTRFILE=attr_def.h
TMPDIR=/tmp

if [ ! -f ./navit.c ] ; then
		echo "Please execute this script from navit's source folder."
		exit 1;
fi

TMPFILE=$TMPDIR/navit-cleanattr.tmp
TMPFILE2=$TMPDIR/navit-cleanattr.tmp2

if [ -f $TMPFILE ] ; then
		echo "Temporary file $TMPFILE already exists."
		echo "Please don't run this tool twice at the same time. If you are sure that no other instance of this tool is running, remove the file."
		exit 1;
fi

touch $TMPFILE
if [ $? -ne 0 ] ; then
		echo "Could not write to temporary file $TEMPFILE."
		echo "Please make sure you have write access to the temporary directory."
		exit 1;
fi


ATTRLIST=`grep 'ATTR(.*)' $ATTRFILE | sed 's#^ATTR(##' | sed 's#).*##'`

cp $ATTRFILE $TMPFILE

for ATTRNAME in $ATTRLIST ; do
		ATTR="attr_$ATTRNAME"

		grep -rI $ATTR ./* > /dev/null

		if [ $? -ne 0 ] ; then
				echo "Unused attribute: $ATTR"
				grep -v "ATTR($ATTRNAME)" $TMPFILE > $TMPFILE2
				mv $TMPFILE2 $TMPFILE
		fi
done

echo "==== Creating patch ===="
diff -U 3 $ATTRFILE $TMPFILE

rm $TMPFILE
