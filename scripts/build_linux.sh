#!/bin/bash
set -e

COVERITY_VERSION="2017.07"
BUILD_PATH="linux"

cmake_opts="-Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE -DSVG2PNG:BOOL=FALSE -DSAMPLE_MAP=n -Dgraphics/gtk_drawing_area:BOOL=TRUE"

[ -d $BUILD_PATH ] || mkdir -p $BUILD_PATH
pushd $BUILD_PATH

# Build everything
    echo "Building..."
cmake ${cmake_opts} ../
make -j $(nproc --all)
make package

if [[ "$CIRCLE_ARTIFACTS" != "" ]]; then
	echo "Copying icons to artifacts..."
	cp -r navit/icons $CIRCLE_ARTIFACTS
fi
