#!/bin/sh
# this builds navit for tomtom
# in case you want to build a plugin for tomtom use build_tomtom_plugin.sh instead
# in case you want to build a standalone system
# https://github.com/george-hopkins/opentom
# https://github.com/gefin/opentom

set -e

bash ~/navit/ci/setup_tomtom_requirements.sh


# navit
cd ~/navit
sed -i "s|set ( TOMTOM_SDK_DIR /opt/tomtom-sdk )|set ( TOMTOM_SDK_DIR $TOMTOM_SDK_DIR )|g" /tmp/$ARCH.cmake
mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DFREETYPE_INCLUDE_DIRS=$PREFIX/include/freetype2/ -Dsupport/gettext_intl=TRUE \
-DHAVE_API_TOMTOM=TRUE -DXSLTS=tomtom -DAVOID_FLOAT=TRUE -Dmap/mg=FALSE -DUSE_PLUGINS=0 -DCMAKE_TOOLCHAIN_FILE=/tmp/$ARCH.cmake \
-DDISABLE_QT=ON -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n
make -j$JOBS
make install
cd ..


# creating directories
OUT_PATH="/tmp/tomtom/sdcard"
rm -rf $OUT_PATH
mkdir -p $OUT_PATH
cd $OUT_PATH
mkdir -p navit
cd navit
mkdir -p bin share
cd share 
mkdir -p fonts
cd ..


# navit executable
cp $PREFIX/bin/navit bin/


# fonts
cp -r ~/navit/navit/fonts/*.ttf $OUT_PATH/navit/share/fonts

# images and xml
cd share
mkdir xpm
cd xpm
cp $PREFIX/share/navit/xpm/*16.png ./
cp $PREFIX/share/navit/xpm/*32.png ./
cp $PREFIX/share/navit/xpm/*48.png ./
cp $PREFIX/share/navit/xpm/*64.png ./
cp $PREFIX/share/navit/xpm/nav*.* ./
cp $PREFIX/share/navit/xpm/country*.png ./
cd ..
cp $PREFIX/share/navit/navit.xml ./tomtom480.xml
mkdir -p maps


# locale
cp -r $PREFIX/share/locale ./


cd $OUT_PATH
zip -r $CIRCLE_ARTIFACTS/navitom_minimal.zip navit
