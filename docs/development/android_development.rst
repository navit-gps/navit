===================
Android Development
===================

Developing for Android
======================

Pre-requisites
--------------

For Navit
~~~~~~~~~

 * Ensure that your system has the correct dependencies installed to build Navit.
 * Checkout the latest copy of Navit from git. The instructions on this page assume you have checked-out a copy to `~/src/navit/`

.. code-block:: bash

    cd ~/src
    git clone git@github.com:navit-gps/navit.git

For Android
~~~~~~~~~~~

 * Download the Android `SDK <http://dl.google.com/android/android-sdk_r18-linux.tgz>` and `NDK <http://dl.google.com/android/ndk/android-ndk-r8-linux-x86.tar.bz2>`
 * Unzip the SDK and NDK to a directory of your choice. The following instructions assume that the SDK and NDK have been unzipped to `~/src`.
 * Ensure that the following paths are on your PATH environment variable:
  `path-to-sdk/tools`
  `path-to-sdk/platform-tools`
  `path-to-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin`
 * On Ubuntu you can accomplish this using the following command (assuming the SDK and NDK have been unzipped to `~/src`):

.. code-block:: bash

    export ANDROID_NDK=~/src/android-ndk-r8
    export ANDROID_SDK=~/src/android-sdk-linux
    export PATH=$PATH:$ANDROID_NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin:$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools

Double-check the version numbers in the paths and adapt if required. If you are running a 64-bit version of the NDK, replace `linux-x86` with `linux-x86_64`.

Prepare Android SDK:
 * run android in command line - this will show a GUI for preparing Android SDK
 * select Android 2.2 (API 8) and click 'Install N packages...' button - this will download SDK packages that support API 8

Compiling
---------

**We have recently switched from CMake to gradle for the Android build. CMake is no longer supported for the Android build, see the section on gradle for details.**

Assuming you've followed the previous steps, you're probably setup to start compiling. Ensure that:
 * You have a java-jdk installed on your system. On Ubuntu: `sudo apt-get install openjdk-6-jdk`. When multiple version of java are installed `update-java-alternatives -s java-1.6.0-openjdk-i386`
 * On Ubuntu, ensure that ant1.8 is installed: `sudo apt-get install ant1.8`
 * Make sure that you have saxonb-xslt installed: `sudo apt-get install libsaxonb-java`

With cmake
----------

 * Ensure that you have **CMake 2.8** installed.
 * Create a build directory - this will be the directory into which the Android version of Navit will be built. Assume we've made one in `~/src` as follows:

.. code-block:: bash

    cd ~/src
    mkdir android-build

 * Optional: Add a `SOURCE_PATH` environment variable to your system, pointing to the directory in which you checked out a copy of Navit. You can leave out this step, but make sure you change `$SOURCE_PATH` in the next steps to the actual path of the directory.  `export SOURCE_PATH=~/src/navit`
 * Step into the build directory: `cd ~/src/android-build`
 * Run CMake (ensure that you're in the build directory when you do this!):

.. code-block:: bash

    cmake -DCMAKE_TOOLCHAIN_FILE=$SOURCE_PATH/Toolchain/arm-eabi.cmake -DCACHE_SIZE="(20*1024*1024)" -DAVOID_FLOAT=1 -DANDROID_PERMISSIONS="CAMERA" -DANDROID_API_VERSION=8 -DXSLT_PROCESSOR=/usr/bin/saxonb-xslt $SOURCE_PATH

 * Build the apk package:

.. code-block:: bash

    make
    make apkg

 * The previous commands have now created a package called `Navit-debug.apk` in the following directory: `~/src/android-build/navit/android/bin`
 * Copy the package to your device (i.e. sd-card) and run it from there (through a file-manager, for example), or
 * If debug bridge (adb) is enabled run: `adb install navit/android/bin/Navit-debug.apk`
 * Later, to reinstall already installed Navit app run: `adb install -r navit/android/bin/Navit-debug.apk`

With cmake on Windows
---------------------

 * install CMake 2.8 if you haven't already, add folder with exe to PATH
 * install MinGW or use MinGW included in the git windows folder (e.g. C:\msysgit\mingw\ ), add folder with exe to PATH
 * install saxon .NET version from <http://sourceforge.net/projects/saxon/files/Saxon-HE/9.3/SaxonHE9-3-0-4N-setup.exe/download>,
 * rename `transform.exe` to `saxon.exe`, add folder with exe to PATH
 * create a build dir (i.e. `mkdir android-build`)
 * run from build dir ( replace `$SOURCE_PATH`):

.. code-block:: bash

    cmake -DCMAKE_TOOLCHAIN_FILE=$SOURCE_PATH/Toolchain/arm-eabi.cmake -DCACHE_SIZE="(20*1024*1024)" -DAVOID_FLOAT=1 -DANDROID_PERMISSIONS="CAMERA" $SOURCE_PATH -G "MinGW Makefiles"

 * run `MinGW32-make`
 * run `MinGW32-make apkg`
 * install `Navit-debug.apk` (in `<build path>/navit/android/bin`) to your device
   * copy `navit/android/bin/Navit-debug.apk` to your device (i.e. sd-card) and run it from there or
   * if debug bridge (`adb`) is enabled run `adb install navit/android/bin/Navit-debug.apk`

With gradle
-----------

**Note: this section is still under construction and may not yet be accurate!**

These setup instructions are for a machine that does not have Android Studio installed. If you have Android Studio, some of them may not be necessary (or can be accomplished in a different way).

Make sure you have the following Android SDK components installed (if not, install them using Android SDK Manager):

 * Android SDK Platform-tools, version 25.0.3 or later
 * Android SDK Build-tools, version 27.0.3

Make sure you have the `ANDROID_HOME` environment variable set and pointing to your Android SDK dir. On Linux, this can be accomplished by adding the following line at the bottom of your `.bashrc` file in your home dir: `export ANDROID_HOME="$HOME/bin/android-sdk-linux_86"` (use the actual path to your SDK install here)

You need to enter the command in your current shell as well in order for it to take effect there as well.

On Ubuntu 18.04 or later (or if your default JRE is Java 9 or later), edit `$ANDROID_HOME/tools/bin/sdkmanager`. Change line #31 to read:

.. code-block:: bash

    DEFAULT_JVM_OPTS='"-Dcom.android.sdklib.toolsdir=$APP_HOME" -XX:+IgnoreUnrecognizedVMOptions --add-modules java.se.ee'

Now run `$ANDROID_HOME/tools/bin/sdkmanager "cmake;3.6.4111459"` and accept the license agreement. (If the package is not found, run `$ANDROID_HOME/tools/bin/sdkmanager --list | grep cmake` and install the cmake version reported there.)

Make sure you have NDK version 12 or later (if you don’t, install it with `$ANDROID_HOME/tools/bin/sdkmanager "ndk-bundle"`.)

If you did **not** install NDK through `sdkmanager`, make sure you have the `ANDROID_NDK_HOME` environment variable set and pointing to your Android NDK dir. (If not, add it as described above.)

Change to the Navit source dir and run `./gradlew build`.

So far, two issues have been observed with the build:

 * Building `vehicle/gpsd` and `map/garmin` fails on Android. As a workaround, edit `CMakeLists.txt` `, inserting the following two lines in the `if(ANDROID)` block (around line 710):

.. code-block:: C

    set_with_reason(vehicle/gpsd "Android detected" FALSE)
    set_with_reason(map/garmin "Android detected" FALSE)

 * Bitmap resources are missing from the APK. A workaround is described `here <https://github.com/navit-gps/navit/pull/553#issuecomment-406881461>` — integration of these steps into gradle is being worked on.

Testing an alternative build
============================

If you want to try an alternative build (e.g. Jan's builds with alternative routing) you can do it by :
 * enable unsigned apk installation ( `example build <http://www.tomsguide.com/faq/id-2326514/download-install-android-apps-unidentified-developer.html>` )
 * installing an alternative apk (e.g. `an APK from CircleCI <https://circle-artifacts.com/gh/jandegr/navit/292/artifacts/0/tmp/circle-artifacts.MZk9Slb/navit-96b3160a2e51dffb54e3aa74c17ce3683c52828e-debug.apk>`)
 * you will probably need an alternative map to match the application requirements (such as `this one <https://circle-artifacts.com/gh/jandegr/navit/265/artifacts/0/tmp/circle-artifacts.WJkkT78/BNLFR.bin>`)
