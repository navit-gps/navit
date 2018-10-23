#!/usr/bin/env bash
set -e
export nixpaste="curl -F 'text=<-' http://nixpaste.lbr.uno"

apt-get update && apt-get install -y mingw-w64 mingw-w64-tools  \
  default-jdk nsis libsaxonb-java curl

mkdir win32
pushd win32
if ! cmake -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n \
  -DXSLTS=windows -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw32.cmake ../
then
    find .
    #nixpaste < ./CMakeFiles/CMakeError.log
    exit 1
fi

if ! make -j $(nproc --all)
then
    echo  "make"
    make -d
    nixpaste < ./Makefile || cat ./Makefile
    exit 1
fi

if ! make -j $(nproc --all) package
then
    echo "make package"
    nixpaste < ./Makefile
    exit 1
fi
popd

cp win32/*.exe $CIRCLE_ARTIFACTS/
