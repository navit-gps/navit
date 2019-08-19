#!/bin/bash
# Build Navit for Android.
#
# This script is to be run from the root of the Navit source tree. It is used by CircleCI as well as for local builds,
# in order to keep build environments as uniform as possible and CI test results meaningful.
#
# It will build Navit for all processor architectures specified in navit/android/build.gradle.
#
# When running this script locally, ensure all build dependencies are in place:
# - Packages required: cmake gettext libsaxonb-java librsvg2-bin pkg-config libprotobuf-c-dev protobuf-c-compiler
# - Android SDK installed
# - Environment variable $ANDROID_HOME points to Android SDK install location
# - Android NDK and CMake components installed via
#     sdkmanager ndk-bundle "cmake;3.6.4111459"
#   (later CMake versions from the SDK repository may also work)
#
# If any of the build steps fails, this script aborts with an error immediately.

echo Set up environment
set - e
export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/tools/bin
export JVM_OPTS="-Xmx3200m"
export GRADLE_OPTS='-Dorg.gradle.jvmargs="-Xmx2048m -XX:+HeapDumpOnOutOfMemoryError"'

# processing xml is messed up a bit after the original introduction of gradle
# so a useless install of ant here even if using gradle/ninja

sudo apt-get install -y ant

echo Run CMake
cmake ./ -Dvehicle/gpsd_dbus:BOOL=FALSE -Dsvg2png_scaling:STRING=-1,24,32,48,64,96,128,192,256 -Dsvg2png_scaling_nav:STRING=-1,24,32,48,64,96,128,192,256 -Dsvg2png_scaling_flag:STRING=-1,24,32,64,96 -DXSL_PROCESSING=y -DXSLTS=android -DANDROID=y || exit 1

echo Process icons
pushd navit/icons
make || exit 32
mkdir ../android/res/drawable-nodpi
cp ./*.png ../android/res/drawable-nodpi
pushd ../android/res/drawable-nodpi
rename 'y/A-Z/a-z/' ./*.png
popd
popd

echo Process translations
pushd po
make || exit 64
mkdir ../navit/android/res/raw
cp ./*.mo ../navit/android/res/raw
pushd ../navit/android/res/raw
rename 'y/A-Z/a-z/' ./*.mo
popd
popd



echo Process xml config files
make navit_config_xml || exit 96
pushd navit
mkdir -p ./android/assets
cp -R config ./android/assets/
popd

echo Chmod permissions
chmod a+x ./gradlew

echo Download dependencies
./gradlew -v

echo Build
./gradlew assembleDebug || exit 128

echo Build finished.
echo APK should be in "navit/android/build/outputs/apk" and can be installed with
echo ./gradlew installDebug

