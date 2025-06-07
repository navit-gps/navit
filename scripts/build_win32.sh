#!/usr/bin/env bash
set -e

mkdir -p win32
pushd win32

cmake -DTARGET_ARCH=i686-w64-mingw32 -DCMAKE_SYSTEM_NAME=Windows \
  -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n \
  -DXSLTS=windows -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw.cmake ..

make -j $(nproc --all)
make -j $(nproc --all) package

popd

cp win32/*.exe $CIRCLE_ARTIFACTS/