mkdir android-x86 && cd android-x86
cmake -DCMAKE_TOOLCHAIN_FILE=Toolchain/i686-android.cmake -DANDROID_API_VERSION=19 -DDISABLE_CXX=1 -DDISABLE_QT=1 ../ || exit -1
make || exit -1
