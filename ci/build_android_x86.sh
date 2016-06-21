export START_PATH=~/
export SOURCE_PATH=$START_PATH"/"${CIRCLE_PROJECT_REPONAME}"/"
export CMAKE_FILE=$SOURCE_PATH"/Toolchain/i686-android.cmake"
export ANDROID_NDK="/usr/local/android-ndk/"
export ANDROID_NDK_BIN=$ANDROID_NDK"/toolchains/x86-4.9/prebuilt/linux-x86_64/bin"
export ANDROID_SDK="/usr/local/android-sdk-linux/"
export ANDROID_SDK_PLATFORM_TOOLS=$ANDROID_SDK"/platform-tools"
export PATH=$ANDROID_NDK_BIN:$ANDROID_SDK_PLATFORM_TOOLS:$PATH

mkdir android-x86 && cd android-x86

android list targets

cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_FILE -DAVOID_FLOAT=1 -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n -DANDROID_API_VERSION=21 -DANDROID_NDK_API_VERSION=19 -DDISABLE_CXX=1 -DDISABLE_QT=1 ../ || exit -1
make || exit -1

if [[ "${CIRCLE_BRANCH}" == "master" ]]; then
  make apkg-release && mv navit/android/bin/Navit-release-unsigned.apk $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-x86-release-unsigned.apk || exit 1
else
  make apkg && mv navit/android/bin/Navit-debug.apk $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-x86-debug.apk || exit 1
fi

#cp ~/android-build/navit/*.xml $CIRCLE_ARTIFACTS/android/

echo
echo "Build leftovers :"
ls navit/android/bin/
