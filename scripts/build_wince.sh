#!/usr/bin/env bash
set -e

mkdir -p wince
pushd wince
#
cmake \
  -DTARGET_ARCH=arm-mingw32ce -DCMAKE_SYSTEM_NAME=WindowsCETest \
  -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw.cmake \
  -DXSLTS=windows,wince -DCACHE_SIZE=10485760 -Dsvg2png_scaling:STRING=16,32 \
  -Dsvg2png_scaling_nav:STRING=32 -Dsvg2png_scaling_flag=16 -DSAMPLE_MAP=n ..
make VERBOSE=1

test -d output && rm -rf output
mkdir output
cp navit/navit.exe output/
cp navit/navit.xml output/
cp navit/navit_layout*.xml output/
cp -r locale/ output/
cp -r navit/icons/ output/
cp -r ../navit/support/espeak/espeak-data/ output/
mkdir output/maps
rm -rf output/icons/CMakeFiles/ icons/cmake_install.cmake

cd output/
zip -r navit.zip  .

cd ../..
bash ./navit/script/cabify.sh wince/output/navit.cab wince/
