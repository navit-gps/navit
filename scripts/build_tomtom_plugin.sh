#!/bin/sh
# this builds a plugin for tomtom
# in case you want to build a standalone system
# https://github.com/george-hopkins/opentom
# https://github.com/gefin/opentom

set -e

export ARCH=arm-linux
cp Toolchain/$ARCH.cmake /tmp

# toolchain
export TOMTOM_SDK_DIR=/opt/tomtom-sdk
mkdir -p $TOMTOM_SDK_DIR >/dev/null 2>&1 || export TOMTOM_SDK_DIR=$HOME/tomtom-sdk
export PREFIX=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/$ARCH/sys-root
export PATH=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/bin:$PREFIX/bin/:$PATH
export CFLAGS="-O2 -I$PREFIX/include -I$PREFIX/usr/include"
export CPPFLAGS="-I$PREFIX/include -I$PREFIX/usr/include"
export LDFLAGS="-L$PREFIX/lib -L$PREFIX/usr/lib"
export CC=$ARCH-gcc
export CXX=$ARCH-g++
export LD=$ARCH-ld
export NM="$ARCH-nm -B"
export AR=$ARCH-ar
export RANLIB=$ARCH-ranlib
export STRIP=$ARCH-strip
export OBJCOPY=$ARCH-objcopy
export LN_S="ln -s"
export PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
JOBS=$(nproc --all)

echo "Jobs"
echo $JOBS

# espeak
pushd /tmp
# this one includes the precompiled voices
wget -nv -c http://freefr.dl.sourceforge.net/project/espeak/espeak/espeak-1.48/espeak-1.48.04-source.zip
unzip espeak-1.48.04-source.zip
pushd espeak-1.48.04-source
sed -i "s/PREFIX=\/usr//g" src/Makefile
sed -i "s/DATADIR=\/usr\/share\/espeak-data/DATADIR=~\/share\/espeak-data/g" src/Makefile
sed -i "s/AUDIO = portaudio/#AUDIO = portaudio/g" src/Makefile
sed -i "s/-fvisibility=hidden//g" src/Makefile
cat src/Makefile
make -C src
pushd src
make install
popd # src
popd # espeak-*
popd # /tmp


# navit
mkdir -p build
pushd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DFREETYPE_INCLUDE_DIRS=$PREFIX/include/freetype2/ -Dsupport/gettext_intl=TRUE \
-DHAVE_API_TOMTOM=TRUE -DXSLTS=tomtom -DAVOID_FLOAT=TRUE -Dmap/mg=FALSE -DUSE_PLUGINS=0 -DCMAKE_TOOLCHAIN_FILE=/tmp/$ARCH.cmake \
-DDISABLE_QT=ON -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n
make -j$JOBS
make install

# creating directories
OUT_PATH="/tmp/tomtom/sdcard"
[ -d $OUT_PATH ] && rm -rf $OUT_PATH
mkdir -p $OUT_PATH/navit/bin
mkdir -p $OUT_PATH/navit/lib
mkdir -p $OUT_PATH/navit/sdl
mkdir -p $OUT_PATH/navit/ts
mkdir -p $OUT_PATH/navit/share/fonts
mkdir -p $OUT_PATH/navit/share/icons
mkdir -p $OUT_PATH/navit/share/maps
mkdir -p $OUT_PATH/navit/share/locale

cp $PREFIX/lib/libfreetype.so.6 $OUT_PATH/navit/lib
cp $PREFIX/lib/libSDL-1.2.so.0 $OUT_PATH/navit/lib
cp $PREFIX/lib/libSDL_image-1.2.so.0 $OUT_PATH/navit/lib
cp $PREFIX/lib/libfreetype.so.6 $OUT_PATH/navit/lib
cp $PREFIX/lib/libgio-2.0.so $OUT_PATH/navit/lib
cp $PREFIX/lib/libglib-2.0.so $OUT_PATH/navit/lib/libglib-2.0.so.0
cp $PREFIX/lib/libgmodule-2.0.so $OUT_PATH/navit/lib/libgmodule-2.0.so.0
cp $PREFIX/lib/libgobject-2.0.so $OUT_PATH/navit/lib/libgobject-2.0.so.0
cp $PREFIX/lib/libgthread-2.0.so $OUT_PATH/navit/lib/libgthread-2.0.so.0
cp $PREFIX/lib/libpng16.so.16 $OUT_PATH/navit/lib
cp $PREFIX/lib/libts-1.0.so.0 $OUT_PATH/navit/lib
cp $PREFIX/lib/libxml2.so.2 $OUT_PATH/navit/lib
cp $PREFIX/lib/libz.so.1 $OUT_PATH/navit/lib
cp $TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/$ARCH/lib/libstdc++.so.5 $OUT_PATH/navit/lib
cp $PREFIX/etc/ts.conf $OUT_PATH/navit/ts

# flite
# cp $PREFIX/bin/flite* bin/

# http://forum.navit-project.org/viewtopic.php?f=17&t=568
arm-linux-gcc -O2 -I$PREFIX/include -I$PREFIX/usr/include ../contrib/tomtom/espeakdsp.c -o $OUT_PATH/navit/bin/espeakdsp

# SDL testvidinfo
cp $PREFIX/usr/bin/testvidinfo $OUT_PATH/navit/sdl

# navit executable and wrapper
cp $PREFIX/bin/navit $OUT_PATH/navit/bin
cp ../contrib/tomtom/navit-wrapper $OUT_PATH/navit/bin/navit-wrapper

# fonts
cp -r ../navit/fonts/*.ttf $OUT_PATH/navit/share/fonts

# ts
cp -r $PREFIX/lib/ts $OUT_PATH/navit/lib/
cp $PREFIX/bin/ts_* $OUT_PATH/navit/ts/

# images and xml
cp $PREFIX/share/navit/icons/*16.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*32.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*48.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*64.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/nav*.* $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/country*.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/navit.xml ./tomtom480.xml


# locale
cp -r $PREFIX/share/locale $OUT_PATH/navit/share/locale

# espeak
cp -r ~/share/espeak-data $OUT_PATH/navit/share/
cp $PREFIX/bin/espeak $OUT_PATH/navit/bin/
cp $PREFIX/lib/libespeak.so.1 $OUT_PATH/navit/lib

# add a menu button
cp -r ../contrib/tomtom/SDKRegistry/ $OUT_PATH/
cp -r ../contrib/tomtom/ts $OUT_PATH/

convert $PREFIX/share/icons/hicolor/128x128/apps/navit.png  -type truecolor -crop 100x100+12+28 -resize 48x48 $OUT_PATH/SDKRegistry/navit.bmp
convert -background none ../navit/icons/tomtom_plus.svg -resize 80x80 $OUT_PATH/navit/share/icons/tomtom_plus_80_80.png
convert -background none ../navit/icons/tomtom_minus.svg -resize 80x80 $OUT_PATH/navit/share/icons/tomtom_minus_80_80.png


cd $OUT_PATH
mkdir /output
zip -r /output/navitom_plugin.zip navit SDKRegistry ts
