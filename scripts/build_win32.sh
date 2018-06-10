#!/usr/bin/env bash
set -e
apt-get update && apt-get install -y mingw32 mingw32-binutils mingw32-runtime default-jdk nsis libsaxonb-java

mkdir win32
pushd win32
cmake -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n -DXSLTS=windows -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw32.cmake ../ && make -j $(nproc --all)  && make -j $(nproc --all) package
popd

cp win32/*.exe $CIRCLE_ARTIFACTS/
