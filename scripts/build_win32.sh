#!/usr/bin/env bash
set -e
<<<<<<< HEAD

apt-get update && apt-get install -y mingw-w64 mingw-w64-tools  \
  default-jdk nsis libsaxonb-java curl
=======
>>>>>>> Refactoring:mingw:simplify toolchain

mkdir win32
pushd win32

<<<<<<< HEAD
cmake -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n \
  -DXSLTS=windows -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw32.cmake ..
=======
cmake -DTARGET_ARCH=i686-w64-mingw32 -DCMAKE_SYSTEM_NAME=Windows \
  -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n \
  -DXSLTS=windows -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw.cmake ..

>>>>>>> Refactoring:mingw:simplify toolchain
make -j $(nproc --all)
make -j $(nproc --all) package

popd

cp win32/*.exe $CIRCLE_ARTIFACTS/