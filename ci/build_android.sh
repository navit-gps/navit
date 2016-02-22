#!/bin/bash
red='\e[0;31m'
grn='\e[0;32m'
yel='\e[1;33m'
off='\e[0m'

# setup var's to perform environment setup and cmake
export START_PATH=~/
export SOURCE_PATH=$START_PATH"/"${CIRCLE_PROJECT_REPONAME}"/"
export CMAKE_FILE=$SOURCE_PATH"/Toolchain/arm-eabi.cmake"

export ANDROID_NDK="/usr/local/android-ndk/"
export ANDROID_NDK_BIN=$ANDROID_NDK"/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin"
 
export ANDROID_SDK="/usr/local/android-sdk-linux/"
export ANDROID_SDK_PLATFORM_TOOLS=$ANDROID_SDK"/platform-tools"

export ANDROID_TOOLS_CHECK=$ANDROID_SDK"/tools"

export ANDROID_PLATFORM_TOOLS_CHECK=$ANDROID_SDK"/platform-tools"

export ANDROID_BUILD_TOOLS="23.0.1"
export ANDROID_BUILD_CHECK=$ANDROID_SDK"/build-tools/"$BUILD_TOOLS

export ANDROID_PLATFORM_LATEST="android-21"
export ANDROID_PLATFORM_MIN="android-7"
export BUILD_PATH=$START_PATH"/android-build"
export ANDROID_ENV=$ANDROID_NDK_BIN:$ANDROID_SDK_TOOLS:$ANDROID_SDK_PLATFORM_TOOLS

# $ANDROID_HOME is /usr/local/android-sdk-linux for the android env. provided by CircleCI

# If path already has our environment no need to set it
if echo "$ANDROID_ENV" | grep -q "$PATH"; then
  echo -e "${grn}" "    Android PATH configuration... ALREADY SET" "${off}"
  echo
else
  echo -e "${grn}" "    Android PATH configuration... EXPORTED" "${off}"
  export PATH=$ANDROID_ENV:$PATH
  echo
fi

mkdir -p $BUILD_PATH
cd $BUILD_PATH
export PATH=$ANDROID_NDK_BIN:$ANDROID_SDK_TOOLS:$ANDROID_SDK_PLATFORM_TOOLS:$PATH
android list targets
mkdir $CIRCLE_ARTIFACTS/android/
cp ~/navit/navit/android/CMakeLists.txt $CIRCLE_ARTIFACTS/android/

cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_FILE -DCACHE_SIZE='(20*1024*1024)' -DAVOID_FLOAT=1 -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n -DANDROID_API_VERSION=21 -DANDROID_NDK_API_VERSION=19 $SOURCE_PATH
make || exit 1
if [[ "${CIRCLE_BRANCH}" == "master" ]]; then
  make apkg-release && mv navit/android/bin/Navit-release-unsigned.apk $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-release-unsigned.apk || exit 1
else
  make apkg && mv navit/android/bin/Navit-debug.apk $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-debug.apk || exit 1
fi

cp ~/android-build/navit/*.xml $CIRCLE_ARTIFACTS/android/

echo
echo "Build leftovers :"
ls navit/android/bin/
