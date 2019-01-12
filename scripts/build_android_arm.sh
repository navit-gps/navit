#!/bin/bash
# Build Navit for Android on ARM.
#
# This script is to be run from the root of the Navit source tree. It is used by CircleCI as well as for local builds,
# in order to keep build environments as uniform as possible and CI test results meaningful.
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
export PATH=$PATH:$ANDROID_HOME/tools
export JVM_OPTS="-Xmx3200m"
export GRADLE_OPTS='-Dorg.gradle.jvmargs="-Xmx2048m -XX:+HeapDumpOnOutOfMemoryError"'

echo Run CMake
cmake ./ -Dvehicle/gpsd_dbus:BOOL=FALSE -Dsvg2png_scaling:STRING=-1,24,32,48,64,96,128,192,256 -Dsvg2png_scaling_nav:STRING=-1,24,32,48,64,96,128,192,256 -Dsvg2png_scaling_flag:STRING=-1,24,32,64,96 -DUSE_PLUGINS=n -DBUILD_MAPTOOL=n -DXSL_PROCESSING=y -DXSLTS=android -DANDROID=y -DSAMPLE_MAP=n || exit 1

echo Process icons
cd navit/icons
make || exit 32
mkdir ../android/res/drawable-nodpi
rename 'y/A-Z/a-z/' ./*.png
cp ./*.png ../android/res/drawable-nodpi
cd ../../

echo Process translations
cd po
make || exit 64
mkdir ../navit/android/res/raw
rename 'y/A-Z/a-z/' ./*.mo
cp ./*.mo ../navit/android/res/raw
cd ../

echo Process xml config files
make navit_config_xml || exit 96
cd navit
mkdir -p ./android/assets
cp -R config ./android/assets/
cd ../

echo Chmod permissions
chmod a+x ./gradlew

echo Download dependencies
./gradlew -v

echo Build
./gradlew assembleDebug || exit 128

echo Build finished.
echo APK should be in "navit/android/build/outputs/apk" and can be installed with
echo ./gradlew installDebug

