#!/bin/bash
set -e

apt-get update && apt-get install -y wget

export ARCH="x86"
export START_PATH=~/
export SOURCE_PATH="${START_PATH}/${CIRCLE_PROJECT_REPONAME}/"
export CMAKE_FILE=$SOURCE_PATH"/Toolchain/i686-android.cmake"
export ANDROID_NDK=~/android-ndk-r11c
export ANDROID_NDK_BIN=$ANDROID_NDK"/toolchains/x86-4.9/prebuilt/linux-x86_64/bin"
export ANDROID_SDK="/usr/local/android-sdk-linux/"
export ANDROID_SDK_PLATFORM_TOOLS=$ANDROID_SDK"/platform-tools"
export PATH=$ANDROID_NDK_BIN:$ANDROID_SDK_PLATFORM_TOOLS:$PATH
export BUILD_PATH=android-${ARCH}

export ANDROID_SDK_HOME=/opt/android-sdk-linux
export ANDROID_HOME=/opt/android-sdk-linux
export PATH=${PATH}:${ANDROID_SDK_HOME}/tools:${ANDROID_SDK_HOME}/platform-tools:/opt/tools


wget -nv -c http://dl.google.com/android/repository/android-ndk-r11c-linux-x86_64.zip
[ -d ~/android-ndk-r11c ] || unzip -q -d ~ android-ndk-r11c-linux-x86_64.zip

[ -d $BUILD_PATH ] || mkdir -p $BUILD_PATH
pushd $BUILD_PATH

android list targets

cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain/i686-android.cmake -DAVOID_FLOAT=1 -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n -DANDROID_API_VERSION=25 -DANDROID_NDK_API_VERSION=19 -DDISABLE_CXX=1 -DDISABLE_QT=1 ..
make -j $(nproc --all)

if [[ "${CIRCLE_BRANCH}" == "master" ]]; then
  make -j $(nproc --all) apkg-release && mv navit/android/bin/Navit-release-unsigned.apk navit/android/bin/navit-$CIRCLE_SHA1-${ARCH}-release-unsigned.apk || exit 1
else
  make -j $(nproc --all) apkg && mv navit/android/bin/Navit-debug.apk navit/android/bin/navit-$CIRCLE_SHA1-${ARCH}-debug.apk || exit 1
fi

echo
echo "Build leftovers :"
find .
