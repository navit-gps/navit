#!/bin/sh

show_api_lvl () {
	case $1 in
		 1) ver='Android 1.0' ;;
		 2) ver='Android 1.1' ;;
		 3) ver='Android 1.5 "Cupcake"' ;;
		 4) ver='Android 1.6 "Donut"' ;;
		 5) ver='Android 2.0 "Eclair"' ;;
		 6) ver='Android 2.0.1 "Eclair"' ;;
		 7) ver='Android 2.1 "Eclair"' ;;
		 8) ver='Android 2.2 "Froyo"' ;;
		 9) ver='Android 2.3 "Gingerbread"' ;;
		10) ver='Android 2.3.3+ "Gingerbread"' ;;
		11) ver='Android 3.0 "Honeycomb"' ;;
		12) ver='Android 3.1 "Ice Cream Sandwich"' ;;
	esac
	echo "$1	$ver"
}

if [ -z $ANDROID_NDK ]; then
	echo "The environment variable ANDROID_NDK must be set!"
	exit
fi

platforms=$ANDROID_NDK/platforms
if [ ! -d $platforms ]; then
	platforms=$ANDROID_NDK/build/platforms
fi

if [ ! -d $platforms ]; then
	echo "The environment variable ANDROID_NDK must be pointing to the android NDK!"
	exit
fi

if [ ! -d navit -o ! -f autogen.sh ]; then
	echo "This script must be called from the navit main directory (where autogen.sh is in)"
	exit
fi

api_lvl=${1-$ANDROID_API_LVL}

while [ -z $api_lvl ]; do
	echo "The environment variable ANDROID_API_LVL is not set and the API level was also not given on the commandline."
	echo "Possible API levels for your NDK are:"
	cd $platforms
	for p in android-*; do
		[ -d $p/arch-arm ] || continue
		show_api_lvl ${p#android-}
	done
	cd - > /dev/null
	echo "Enter the desired API level (4 is recommended)"
	read api_lvl
	if [ ! -d $platforms/android-$api_lvl/arch-arm ]; then
		echo "Bad choice, retrying..."; echo ""
		unset api_lvl
	fi
done

ANDROID_API_LVL=$api_lvl
export ANDROID_API_LVL
ANDROID_PLATFORM=$platforms/android-$ANDROID_API_LVL/arch-arm
export ANDROID_PLATFORM
PATH=`echo $ANDROID_NDK/toolchains/arm-eabi-*/prebuilt/linux-x86/bin`:$PATH
export PATH

if [ ! -f Makefile.in ]; then
	echo ""; echo "*** Running autogen.sh ***"; echo ""
	./autogen.sh
fi

if [ ! -f Makefile ]; then
	echo ""; echo "*** Running configure ***"; echo ""
	./configure \
		PKG_CONFIG=arm-eabi-pkgconfig \
		RANLIB=arm-eabi-ranlib \
		AR=arm-eabi-ar \
		CC="arm-eabi-gcc -L$ANDROID_PLATFORM/usr/lib -L. -I$ANDROID_PLATFORM/usr/include" \
		CXX=arm-eabi-g++ \
		--host=arm-eabi-linux_android \
		--enable-avoid-float \
		--enable-avoid-unaligned \
		--enable-cache-size=20971520 \
		--enable-svg2png-scaling=8,16,32,48,64,96 \
		--enable-svg2png-scaling-nav=59 \
		--enable-svg2png-scaling-flag=32 \
		--with-xslts=android,plugin_menu,pedestrian_button,pedestrian \
		--with-saxon=saxonb-xslt \
		--enable-transformation-roll \
		--enable-plugin-pedestrian \
		--with-android-permissions=CAMERA \
		--with-android-project=android-$ANDROID_API_LVL
fi

set -e

echo ""; echo "*** Running make ***"; echo ""
make

echo ""; echo "*** Running make apkg ***"; echo ""
cd navit
make apkg
cd - > /dev/null

apk=`pwd`/navit/android/bin/Navit-debug.apk
if [ -f $apk ]; then
	echo "Great - looks like it worked! The result is"; echo ""
	ls -l $apk
	echo ""
else
	echo "This shouldn't happen! Something went terribly wrong..."
fi
