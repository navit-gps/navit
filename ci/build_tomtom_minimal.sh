#!/bin/sh
# this builds navit for tomtom
# in case you want to build a plugin for tomtom use build_tomtom_plugin.sh instead
# in case you want to build a standalone system
# https://github.com/george-hopkins/opentom
# https://github.com/gefin/opentom

set -e

source ~/navit/ci/setup_tomtom_requirements.sh



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
