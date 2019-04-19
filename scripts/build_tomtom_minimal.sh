#!/usr/bin/env bash
# this builds navit for tomtom
# in case you want to build a plugin for tomtom use build_tomtom_plugin.sh instead
# in case you want to build a standalone system
# https://github.com/george-hopkins/opentom
# https://github.com/gefin/opentom

set -e

export ARCH=arm-linux
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

mkdir -p build
pushd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DFREETYPE_INCLUDE_DIRS=$PREFIX/include/freetype2/ -Dsupport/gettext_intl=TRUE \
-DHAVE_API_TOMTOM=TRUE -DXSLTS=tomtom -DAVOID_FLOAT=TRUE -Dmap/mg=FALSE -DUSE_PLUGINS=0 -DCMAKE_TOOLCHAIN_FILE=../Toolchain/$ARCH.cmake \
-DDISABLE_QT=ON -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n
make -j$JOBS
make install

# creating directories
OUT_PATH="/tmp/tomtom/sdcard"
[ -d $OUT_PATH ] && rm -rf $OUT_PATH
mkdir -p $OUT_PATH/navit/bin
mkdir -p $OUT_PATH/navit/share/fonts
mkdir -p $OUT_PATH/navit/share/icons
mkdir -p $OUT_PATH/navit/share/maps
mkdir -p $OUT_PATH/navit/share/locale

# navit executable
cp navit/navit $OUT_PATH/navit/bin

# fonts
cp -r ../navit/fonts/*.ttf $OUT_PATH/navit/share/fonts

# images and xml
cp $PREFIX/share/navit/icons/*16.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*32.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*48.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/*64.png $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/nav*.* $OUT_PATH/navit/share/icons
cp $PREFIX/share/navit/icons/country*.png $OUT_PATH/navit/share/icons
cd ..
cp $PREFIX/share/navit/navit.xml ./tomtom480.xml

# locale
cp -r $PREFIX/share/locale $OUT_PATH/navit/share/locale


cd $OUT_PATH
mkdir /output
zip -r /output/navitom_minimal.zip navit
