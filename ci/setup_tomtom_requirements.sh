#!/bin/sh
# this builds navit for tomtom
# in case you want to build a plugin for tomtom use build_tomtom_plugin.sh instead
# in case you want to build a standalone system
# https://github.com/george-hopkins/opentom
# https://github.com/gefin/opentom

set -e

export ARCH=arm-linux
cp Toolchain/$ARCH.cmake /tmp

# toolchain
export TOMTOM_SDK_DIR=/opt/tomtom-sdk
mkdir -p $TOMTOM_SDK_DIR >/dev/null 2>&1 || export TOMTOM_SDK_DIR=$HOME/tomtom-sdk 
export PREFIX=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/$ARCH/sys-root
export PATH=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/bin:$PREFIX/bin/:$PATH
export CFLAGS="-O2 -I$PREFIX/include -I$PREFIX/usr/include"
export CPPFLAGS="-I$PREFIX/include -I$PREFIX/usr/include"
export LDFLAGS="-L$PREFIX/lib -L$PREFIX/usr/lib"
export CC=$ARCH-gcc
export CXX=$ARCH-g++
export LD=$ARCH-ld
export NM="$ARCH-nm -B"
export AR=$ARCH-ar
export RANLIB=$ARCH-ranlib
export STRIP=$ARCH-strip
export OBJCOPY=$ARCH-objcopy
export LN_S="ln -s"
export PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
JOBS=`getconf _NPROCESSORS_ONLN`

echo "Jobs"
echo $JOBS

mkdir -p ~/assets/tomtom/

if ! [ -e "~/assets/tomtom/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz" ]
 then 
  wget -nv -c http://www.tomtom.com/gpl/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -P ~/assets/tomtom
fi

if ! test -f "~/assets/tomtom/libpng-1.6.29.tar.gz"
then 
  wget -nv -c ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.29.tar.gz -P ~/assets/tomtom
fi

# toolchain
cd /tmp
mkdir -p $TOMTOM_SDK_DIR
tar xzf ~/assets/tomtom/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -C $TOMTOM_SDK_DIR

# zlib
cd /tmp
wget -nv -c http://zlib.net/zlib-1.2.9.tar.gz
tar xzf zlib-1.2.9.tar.gz
cd zlib-1.2.9
./configure --prefix=$PREFIX
make -j$JOBS
make install

# libxml
cd /tmp/
wget -nv -c http://xmlsoft.org/sources/libxml2-2.7.8.tar.gz
tar xzf libxml2-2.7.8.tar.gz
cd libxml2-2.7.8/
./configure --prefix=$PREFIX --host=$ARCH --without-python
make -j$JOBS
make install

# libpng
cd /tmp/
tar xzf ~/assets/tomtom/libpng-1.6.29.tar.gz
cd libpng-1.6.29/ 
./configure --prefix=$PREFIX --host=$ARCH
make -j$JOBS
make install


cd /tmp
wget -nv -c http://download.savannah.gnu.org/releases/freetype/freetype-2.5.0.tar.gz
tar xzf freetype-2.5.0.tar.gz
cd freetype-2.5.0
./configure --prefix=$PREFIX --host=$ARCH
make -j$JOBS
make install

freetype-config --cflags

# glib
cd /tmp
wget -nv -c http://ftp.gnome.org/pub/gnome/sources/glib/2.25/glib-2.25.17.tar.gz
tar xzf glib-2.25.17.tar.gz
cd glib-2.25.17
cat > tomtom.cache << EOF
glib_cv_long_long_format=ll
glib_cv_stack_grows=no
glib_cv_uscore=no
ac_cv_func_posix_getgrgid_r=yes
ac_cv_func_posix_getpwuid_r=yes
EOF
chmod a-w tomtom.cache
./configure --prefix=$PREFIX --host=$ARCH --cache-file=tomtom.cache
sed -i "s|cp xgen-gmc gmarshal.c |cp xgen-gmc gmarshal.c \&\& sed -i \"s\|g_value_get_schar\|g_value_get_char\|g\" gmarshal.c |g" gobject/Makefile
make -j$JOBS
make install


# tslib
cd /tmp
rm -rf tslib-svn
git clone https://github.com/playya/tslib-svn.git
cd tslib-svn
sed -i "s|AM_CONFIG_HEADER|AC_CONFIG_HEADERS|g" configure.ac
sed -i "119i\#ifdef EVIOCGRAB" plugins/input-raw.c
sed -i "124i\#endif" plugins/input-raw.c
sed -i "290i\#ifdef EVIOCGRAB" plugins/input-raw.c
sed -i "294i\#endif" plugins/input-raw.c
sed -i "s|# module_raw h3600|module_raw h3600|g" etc/ts.conf # tomtom go 710
./autogen.sh
./configure --prefix=$PREFIX --host=$ARCH
make -j$JOBS
make install


cd /tmp
wget -nv -c http://www.libsdl.org/release/SDL-1.2.15.tar.gz
tar xzf SDL-1.2.15.tar.gz
cd SDL-1.2.15
wget -nv -c http://tracks.yaina.de/source/sdl-fbcon-notty.patch
patch -p0 -i sdl-fbcon-notty.patch
./configure --prefix=$PREFIX --host=$ARCH \
  --disable-esd --disable-cdrom --disable-joystick --disable-video-x11 \
  --disable-x11-vm --disable-dga --disable-video-x11-dgamouse \
  --disable-video-x11-xv --disable-video-x11-xinerama --disable-video-directfb \
  --enable-video-fbcon --disable-audio CFLAGS="$CFLAGS -DFBCON_NOTTY"
make -j$JOBS
make install

# sdl test utilities
cd test
./configure --prefix=$PREFIX --host=$ARCH
make testvidinfo
cp testvidinfo $PREFIX/usr/bin/

# to find sdl-config
export PATH=$PREFIX/bin:$PATH

# sdl image
cd /tmp
wget -nv -c http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12.tar.gz
tar xzf SDL_image-1.2.12.tar.gz
cd SDL_image-1.2.12
./configure --prefix=$PREFIX --host=$ARCH
make -j$JOBS
make install



# espeak
cd /tmp
# this one includes the precompiled voices
wget -nv -c http://freefr.dl.sourceforge.net/project/espeak/espeak/espeak-1.48/espeak-1.48.04-source.zip
unzip espeak-1.48.04-source.zip
cd espeak-1.48.04-source
sed -i "s/PREFIX=\/usr//g" src/Makefile
sed -i "s/DATADIR=\/usr\/share\/espeak-data/DATADIR=~\/share\/espeak-data/g" src/Makefile
sed -i "s/AUDIO = portaudio/#AUDIO = portaudio/g" src/Makefile
sed -i "s/-fvisibility=hidden//g" src/Makefile
cat src/Makefile
make -C src
cd src
sudo make install

# http://forum.navit-project.org/viewtopic.php?f=17&t=568
cd /tmp
arm-linux-gcc -O2 -I$PREFIX/include -I$PREFIX/usr/include ~/navit/contrib/tomtom/espeakdsp.c -o espeakdsp



# in the end we only want Navit locale
rm -r $PREFIX/share/locale

# navit
cd ~/navit
sed -i "s|set ( TOMTOM_SDK_DIR /opt/tomtom-sdk )|set ( TOMTOM_SDK_DIR $TOMTOM_SDK_DIR )|g" /tmp/$ARCH.cmake
mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DFREETYPE_INCLUDE_DIRS=$PREFIX/include/freetype2/ -Dsupport/gettext_intl=TRUE \
-DHAVE_API_TOMTOM=TRUE -DXSLTS=tomtom -DAVOID_FLOAT=TRUE -Dmap/mg=FALSE -DUSE_PLUGINS=0 -DCMAKE_TOOLCHAIN_FILE=/tmp/$ARCH.cmake \
-DDISABLE_QT=ON -DSAMPLE_MAP=n -DBUILD_MAPTOOL=n
make -j$JOBS
make install
cd ..


# creating directories
OUT_PATH="/tmp/tomtom/sdcard"
rm -rf $OUT_PATH
mkdir -p $OUT_PATH
cd $OUT_PATH
mkdir -p navit SDKRegistry
cd navit
mkdir -p bin lib share sdl ts
cd share
mkdir -p fonts
cd ..

