=================
WinCE Development
=================

This is a tutorial for Navit on WinCE/WinMobile. If want just want to download a cab file see [[WinCE]].

This page explains how to build Navit for WinCE/WinMobile with `cegcc <http://cegcc.sourceforge.net>`.

In November 2009 versions compiled using arm-cegcc-gcc (both revision 1214 and release 0.59.1) had problems (threw exception_datatype_misalignment and caused access violations).

Using the variant arm-mingw32ce of CeGCC 0.59.1 it was possible to build a working executable which can be debugged.

The automatic builds from the subversion repository seem to use an adjusted? version arm-wince-mingw32ce (see `build logs <http://download.navit-project.org/logs/navit/wince/svn>`).

Building using arm-mingw32ce
============================

Install arm-mingw32ce

.. code-block:: bash

    mkdir -p navit
    cd navit
    export NAVIT_PATH=`pwd`
    wget -c https://sourceforge.net/projects/cegcc/files/cegcc/0.59.1/mingw32ce-0.59.1.tar.bz2/download
    tar xjf mingw32ce-0.59.1.tar.bz2

Building Navit using Cmake

.. code-block:: bash

    #!/bin/bash
    export NAVIT_PATH="/usr/src/navit"
    export MINGW32CE_PATH="/opt/mingw32ce"
    export PATH=$PATH:$MINGW32CE_PATH/bin

    cd $NAVIT_PATH
    if [ ! -e build ]; then
      mkdir build;
    fi;
    cd build

    cmake \
    -DCMAKE_TOOLCHAIN_FILE=$NAVIT_PATH/Toolchain/arm-mingw32ce.cmake \
    -Dsvg2png_scaling:STRING=0,16 \
    -Dsvg2png_scaling_nav:STRING=32 \
    -Dsvg2png_scaling_country:STRING=16 \
    $NAVIT_PATH

    make

For subsequential builds it is sufficient to issue "make" in the build directory.
A rerun of cmake is only neccessary if parameters are changed.

Remote Debugging
================

Download the debugger provided by the CeGCC project:

.. code-block:: bash

    cd $NAVIT_PATH
    wget -c https://sourceforge.net/projects/cegcc/files/cegcc/0.59.1/gdb-arm-0.59.1.tar.bz2/download
    tar xjf gdb-arm-0.59.1.tar.bz2

Start navit (from SD card) in debug server at target (using TCP port 9999):

.. code-block:: bash

    gdbserver :9999 "\Mounted Volume\navit\navit.exe"

Execute remote debugger at host, if target's IP address was 192.168.1.112:

.. code-block:: bash

    $NAVIT_PATH/opt/mingw32ce/bin/arm-mingw32ce-gdbtui $NAVIT_PATH/navit/navit.exe -eval-command="target remote 192.168.1.112:9999"

Building using arm-cegcc
========================

Building cegcc
--------------

Set the install path to where you want to install `cegcc <http://cegcc.sourceforge.net cegcc>`:

.. code-block:: bash

    export CEGCC_PATH=/usr/local/cegcc
    svn co -r 1214 https://cegcc.svn.sourceforge.net/svnroot/cegcc/trunk/cegcc
    mkdir -p cegcc-builds
    cd cegcc-builds
    ../cegcc/src/build-cegcc.sh --prefix=$CEGCC_PATH --components="binutils bootstrap_gcc w32api newlib dummy_cegccdll gcc cegccdll cegccthrddll libstdcppdll profile"

If you get an error like "'makekinfo' is missing on your system" although makeinfo is available (happened with openSUSE 11.2 and Debian Lenny, both 32 bit), add a workaround to the script src/newlib/missing. Insert a new line after the line "  makeinfo)":

.. code-block:: bash

     "$@" && exit 0

If you get an error like `arm-cegcc-windres: Can't detect architecture`, apply the patch file you find on http://sourceforge.net/tracker/?func=detail&atid=865516&aid=2574606&group_id=173455

Building libraries
------------------

November 2009: The libraries below are *not needed* anymore since navit brings its own version of glib.

The libraries require additional (not published or not existing) patches to build. Just skip to section Building Navit.

These are the libraries needed and versions which should work:
 * zlib-1.2.3
 * libiconv-1.9.1
 * gettext-0.17
 * libpng-1.2.34
 * tiff-3.8.2
 * glib-2.18.4

The current versions of these libs don't need many changes, but they all don't know anything about cegcc. Until I found a way to upload the patches, you have to edit the code yourself. Just add `| -cegcc*` to the line containing `-cygwin*` of all files named config.sub. Here is the example for libiconv-1.9.1_cegcc.patch:

.. code-block:: bash

    diff -ur libiconv-1.9.1/autoconf/config.sub libiconv-1.9.1_cegcc/autoconf/config.sub
    --- libiconv-1.9.1/autoconf/config.sub 2003-05-06 11:36:42.000000000 +0200
    +++ libiconv-1.9.1_cegcc/autoconf/config.sub   2009-02-06 20:22:14.000000000 +0100
    @@ -1121,7 +1121,7 @@
             | -ptx* | -coff* | -ecoff* | -winnt* | -domain* | -vsta* \
             | -udi* | -eabi* | -lites* | -ieee* | -go32* | -aux* \
             | -chorusos* | -chorusrdb* \
    -        | -cygwin* | -pe* | -psos* | -moss* | -proelf* | -rtems* \
    +        | -cygwin* | -pe* | -psos* | -moss* | -proelf* | -rtems* | -cegcc* \
             | -mingw32* | -linux-gnu* | -uxpv* | -beos* | -mpeix* | -udk* \
             | -interix* | -uwin* | -mks* | -rhapsody* | -darwin* | -opened* \
             | -openstep* | -oskit* | -conix* | -pw32* | -nonstopux* \
    diff -ur libiconv-1.9.1/libcharset/autoconf/config.sub libiconv-1.9.1_cegcc/libcharset/autoconf/config.sub
    --- libiconv-1.9.1/libcharset/autoconf/config.sub  2003-05-06 11:36:42.000000000 +0200
    +++ libiconv-1.9.1_cegcc/libcharset/autoconf/config.sub    2009-02-06 20:23:39.000000000 +0100
    @@ -1121,7 +1121,7 @@
             | -ptx* | -coff* | -ecoff* | -winnt* | -domain* | -vsta* \
             | -udi* | -eabi* | -lites* | -ieee* | -go32* | -aux* \
             | -chorusos* | -chorusrdb* \
    -        | -cygwin* | -pe* | -psos* | -moss* | -proelf* | -rtems* \
    +        | -cygwin* | -pe* | -psos* | -moss* | -proelf* | -rtems* | -cegcc* \
             | -mingw32* | -linux-gnu* | -uxpv* | -beos* | -mpeix* | -udk* \
             | -interix* | -uwin* | -mks* | -rhapsody* | -darwin* | -opened* \
             | -openstep* | -oskit* | -conix* | -pw32* | -nonstopux* \

zlib
^^^^

.. code-block:: bash

    wget http://www.zlib.net/zlib-1.2.3.tar.gz
    tar xzf zlib-1.2.3.tar.gz
    cd zlib-1.2.3
    export PATH=$CEGCC_PATH/bin:$PATH
    CC=arm-cegcc-gcc AR="arm-cegcc-ar r" RANLIB=arm--cegcc-ranlib ./configure --prefix=$CEGCC_PATH
    make
    make install

libiconv
^^^^^^^^


.. code-block:: bash

    wget http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.9.1.tar.gz
    tar xzf libiconv-1.9.1.tar.gz
    patch -d libiconv-1.9.1 -p1 < libiconv-1.9.1_cegcc.patch
    cd libiconv-1.9.1
    ./configure --host=arm-cegcc --prefix=$CEGCC_PATH
    make
    make install

gettext
^^^^^^^

workaround for `plural-eval.h:50: error: expected '=', ',', ';', 'asm' or '__attribute__' before 'sigfpe_exit'`
extend gettext-tools/src/plural-eval.h line 32 to `#if defined _MSC_VER || defined __MINGW32__ || defined __CEGCC__`
dito for gettext-tools/gnulib-lib/wait-process.c line 31


.. code-block:: bash

    wget http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-0.17.tar.gz
    tar xzf gettext-0.17.tar.gz
    cd gettext-0.17
    patch -p1 < ../gettext-0.17_cegcc.patch
    ./configure --host=arm-cegcc --prefix=$CEGCC_PATH
    make
    make install

libpng
^^^^^^

.. code-block:: bash

    wget http://prdownloads.sourceforge.net/libpng/libpng-1.2.34.tar.gz?download
    tar xzf libpng-1.2.34.tar.gz
    cd libpng-1.2.34
    patch -p1 < ../libpng-1.2.34_cegcc.patch
    ./configure --host=arm-cegcc --prefix=$CEGCC_PATH
    CFLAG="-I $C_INCLUDE_PATH" make
    make install

libtiff
^^^^^^^

.. code-block::

 libtool: link: CURRENT `' must be a nonnegative integer


.. code-block:: bash

    wget http://libtiff.maptools.org/dl/tiff-3.8.2.tar.gz
    tar xzf tiff-3.8.2.tar.gz
    cd tiff-3.8.2
    patch -p1 < ../tiff-3.8.2_cegcc.patch
    ./configure --host=arm-cegcc --prefix=$CEGCC_PATH
    make
    make install

glib
^^^^

.. code-block::

 gatomic.c:570: Error: no such instruction: `swp %eax,%eax,[%esi]'


.. code-block:: bash

    wget http://ftp.gnome.org/pub/gnome/sources/glib/2.18/glib-2.18.4.tar.bz2
    tar xjf glib-2.18.4.tar.bz2
    cd glib-2.18.4
    patch -p1 < ../glib-2.18.4_cegcc.patch
    ./configure --host=arm-cegcc --prefix=$CEGCC_PATH
    make
    make install

Building Navit
==============

.. code-block:: bash

    git clone https://github.com/navit-gps/navit.git
    cd navit/navit

Add `| -cegcc*` to all files named `config.sub` as for the libraries.

.. code-block:: bash

 WINDRES=arm-cegcc-windres ./configure --disable-vehicle-file --host=arm-cegcc --prefix=$CEGCC_PATH 2>&1 | tee configure-cegcc.log

Add to `navit\support\wordexp\glob.h`: `|| defined __CEGCC__`

Change include in `navit\vehicle\wince\vehicle_wince.c`: `#include <sys/io.h>`

Add to `navit\file.c`: `&& !defined __CEGCC__`

.. code-block:: bash

    make -j
