#!/bin/sh
# this builds a plugin for tomtom
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

mkdir -p ~/tomtom_assets

if ! [ -e "~/tomtom_assets/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz" ]
 then 
  wget -nv -c https://github.com/navit-gps/dependencies/raw/master/tomtom/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -P ~/tomtom_assets
fi

if ! test -f "~/tomtom_assets/libpng-1.6.28.tar.gz"
then 
  wget -nv -c ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.28.tar.gz -P ~/tomtom_assets
fi

# toolchain
cd /tmp
mkdir -p $TOMTOM_SDK_DIR
tar xzf ~/tomtom_assets/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -C $TOMTOM_SDK_DIR

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
make install

# http://forum.navit-project.org/viewtopic.php?f=17&t=568
cd /tmp
cat > /tmp/espeakdsp.c << EOF
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/soundcard.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define IBUFFERLEN 1024
#define MAXARGC 30


int  main(int argc, char *argv[],char *envp[])
{
	int pipefd[2];
	pid_t cpid;
	char buf;
	int co,wp,l,fh;
	short bufi[IBUFFERLEN],bufo[IBUFFERLEN*2];	 
	int rate=22050;

	char *newargv[MAXARGC+2];

	for(co=0;co<argc;co++)
	{
		if(co>=MAXARGC)break;
		newargv[co]=argv[co];
	}
	newargv[co++]="--stdout";
	newargv[co++]=NULL;

	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	if(setpriority(PRIO_PROCESS,0,-10))
	perror ("setpriority");

	cpid = fork();
	if (cpid == -1)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (cpid == 0)
	{    /* Child writes to pipe */

		close(pipefd[0]);          /* Close unused read end */
		dup2(pipefd[1],1);	
		execve("/mnt/sdcard/navit/bin/espeak",newargv,envp);	      
		perror("exec /mnt/sdcard/navit/bin/espeak");	
		close(pipefd[1]);          /* Reader will see EOF */
		wait(NULL);                /* Wait for child */
		exit(EXIT_SUCCESS);

	} else { /* Parent read from  pipe */

		close(pipefd[1]);          /* Close unused write end */

		l=read(pipefd[0],bufi,64);
		if(memcmp(bufi,"RIFF",4))
		{
			while(l>0)
			{
				write(1,bufi,l);
				l=read(pipefd[0],bufi,IBUFFERLEN);
			}
			exit(EXIT_SUCCESS);
		}
		l=read(pipefd[0],bufi,IBUFFERLEN);
		if(l<500)
		{
			printf("espeakdsp: avoid noise speaking a empty string\n");
			exit(EXIT_SUCCESS); 	
		} 			
		usleep (50000);

		fh=open("/dev/dsp",O_WRONLY);
		if(fh<0)
		{
			perror("open /dev/dsp");	
			exit(EXIT_FAILURE);
		}
		ioctl(fh, SNDCTL_DSP_SPEED , &rate);
		ioctl(fh, SNDCTL_DSP_SYNC, 0);
		while(l)
		{
			for(co=0,wp=0;co<IBUFFERLEN;co++)
			{
				bufo[wp++]=bufi[co]; /* mono->stereo */
				bufo[wp++]=bufi[co];
			}
			write (fh,bufo,wp);
			l=read(pipefd[0],bufi,IBUFFERLEN);
		}
		ioctl(fh, SNDCTL_DSP_SYNC, 0);
		close(pipefd[0]);
		exit(EXIT_SUCCESS);
	}
}
EOF
arm-linux-gcc -O2 -I$PREFIX/include -I$PREFIX/usr/include espeakdsp.c -o espeakdsp

# zlib
cd /tmp
wget -nv -c http://zlib.net/zlib-1.2.11.tar.gz
tar xzf zlib-1.2.11.tar.gz
cd zlib-1.2.11
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
tar xzf ~/tomtom_assets/libpng-1.6.28.tar.gz
cd libpng-1.6.28/ 
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


# in the end we only want Navit locale
rm -r $PREFIX/share/locale

cat > ~/navit/navit/xpm/tomtom_minus.svg << EOF
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="64px" height="64px" viewBox="-100 -100 200 200" xmlns="http://www.w3.org/2000/svg" version="1.1">
<rect x="-110" y="-110" width="220" height="220" fill="#000080" stroke="none" opacity="0.5"/>
<path fill="none" stroke="#ffffff" stroke-width="20" stroke-linecap="round" d="M 60 0 L -60 0"/>
</svg>
EOF

cat > ~/navit/navit/xpm/tomtom_plus.svg << EOF
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="64px" height="64px" viewBox="-100 -100 200 200" xmlns="http://www.w3.org/2000/svg" version="1.1">
<rect x="-110" y="-110" width="220" height="220" fill="#000080" stroke="none" opacity="0.5"/>
<path fill="none" stroke="#ffffff" stroke-width="20" stroke-linecap="round" d="M 0 60 L 0 -60 M 60 0 L -60 0"/>
</svg>
EOF


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


cp $PREFIX/lib/libfreetype.so.6 lib
cp $PREFIX/lib/libSDL-1.2.so.0 lib
cp $PREFIX/lib/libSDL_image-1.2.so.0 lib
cp $PREFIX/lib/libfreetype.so.6 lib
cp $PREFIX/lib/libgio-2.0.so lib
cp $PREFIX/lib/libglib-2.0.so lib/libglib-2.0.so.0
cp $PREFIX/lib/libgmodule-2.0.so lib/libgmodule-2.0.so.0
cp $PREFIX/lib/libgobject-2.0.so lib/libgobject-2.0.so.0
cp $PREFIX/lib/libgthread-2.0.so lib/libgthread-2.0.so.0
cp $PREFIX/lib/libpng16.so.16 lib
cp $PREFIX/lib/libts-1.0.so.0 lib
cp $PREFIX/lib/libxml2.so.2 lib
cp $PREFIX/lib/libz.so.1 lib
cp $PREFIX/etc/ts.conf ts
cp $TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/$ARCH/lib/libstdc++.so.5 lib

# flite
# cp $PREFIX/bin/flite* bin/

# SDL testvidinfo
cp $PREFIX/usr/bin/testvidinfo sdl/

# navit executable and wrapper
cp $PREFIX/bin/navit bin/
cat > bin/navit-wrapper << 'EOF'
#!/bin/sh

cd /mnt/sdcard/navit/bin

# Set some paths.
export PATH=$PATH:/mnt/sdcard/navit/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/sdcard/navit/lib:/mnt/sdcard/LoquendoTTS/lib
export HOME=/mnt/sdcard/navit
export NAVIT_LIBDIR=/mnt/sdcard/navit/lib/navit
export NAVIT_SHAREDIR=/mnt/sdcard/navit/share
export NAVIT_LOCALEDIR=/mnt/sdcard/navit/share/locale

# tslib requirements.
export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb
export TSLIB_TSDEVICE=/dev/ts
export TSLIB_CALIBFILE=/mnt/sdcard/navit/ts/pointercal
export TSLIB_CONFFILE=/mnt/sdcard/navit/ts/ts.conf
export TSLIB_PLUGINDIR=/mnt/sdcard/navit/lib/ts

# SDL requirements.
export SDL_MOUSEDRV=TSLIB
export SDL_MOUSEDEV=$TSLIB_TSDEVICE
export SDL_NOMOUSE=1
export SDL_FBDEV=/dev/fb
export SDL_VIDEODRIVER=fbcon
export SDL_AUDIODRIVER=dsp

# Set time zone.
export TZ="CEDT-01:00:00CEST-02:00:00,M3.4.0,M10.4.0"

# Set language.
export LANG=en_US

export ESPEAK_DATA_PATH=/mnt/sdcard/navit/share

# Run Navit.
if /mnt/sdcard/navit/sdl/testvidinfo | grep 480x272
then
	/mnt/sdcard/navit/bin/navit /mnt/sdcard/navit/share/tomtom480.xml 2>/mnt/sdcard/navit/navit.log&
# tomtom320xml is not provided yet
# elif  /mnt/sdcard/navit/sdl/testvidinfo | grep 320x240
# then
# 	/mnt/sdcard/navit/bin/navit /mnt/sdcard/navit/share/config/tomtom320.xml 2>/mnt/sdcard/navit/navit.log&
else
	exit 1
fi

# Kill TTN while Navit is running.
killall ttn

while [ $? -eq 0 ]
do
echo "\0" > /dev/watchdog
sleep 10
ps | grep -v grep | grep -v wrapper | grep navit
done

/sbin/reboot

EOF
chmod a+rx bin/navit-wrapper

# fonts
cp -r ~/navit/navit/fonts/*.ttf $OUT_PATH/navit/share/fonts

# ts
cp -r $PREFIX/lib/ts $OUT_PATH/navit/lib/
cp $PREFIX/bin/ts_* $OUT_PATH/navit/ts/

# images and xml
cd share
mkdir xpm
cd xpm
cp $PREFIX/share/navit/xpm/*16.png ./
cp $PREFIX/share/navit/xpm/*32.png ./
cp $PREFIX/share/navit/xpm/*48.png ./
cp $PREFIX/share/navit/xpm/*64.png ./
cp $PREFIX/share/navit/xpm/nav*.* ./
cp $PREFIX/share/navit/xpm/country*.png ./
cd ..
cp $PREFIX/share/navit/navit.xml ./tomtom480.xml
mkdir -p maps


# locale
cp -r $PREFIX/share/locale ./

# espeak
cp -r ~/share/espeak-data ./
cp $PREFIX/bin/espeak $OUT_PATH/navit/bin/
cp $PREFIX/lib/libespeak.so.1 $OUT_PATH/navit/lib

mv /tmp/espeakdsp $OUT_PATH/navit/bin/

# add a menu button
cat > $OUT_PATH/SDKRegistry/navit.cap << EOF
Version|100|
AppName|navit-wrapper|
AppPath|/mnt/sdcard/navit/bin/|
AppIconFile|navit.bmp|
AppMainTitle|Navit|
AppPort|2001|
COMMAND|CMD|hallo|navit.bmp|Navit|
EOF


convert $PREFIX/share/icons/hicolor/128x128/apps/navit.png  -type truecolor -crop 100x100+12+28 -resize 48x48 $OUT_PATH/SDKRegistry/navit.bmp
convert -background none ~/navit/navit/xpm/tomtom_plus.svg -resize 80x80 $OUT_PATH/navit/share/xpm/tomtom_plus_80_80.png
convert -background none ~/navit/navit/xpm/tomtom_minus.svg -resize 80x80 $OUT_PATH/navit/share/xpm/tomtom_minus_80_80.png

cat > $OUT_PATH/SDKRegistry/ts.cap << EOF
Version|100|
AppName|ts-wrapper|
AppPath|/mnt/sdcard/navit/ts/|
AppIconFile||
AppMainTitle|Touchscreen|
AppPort||
COMMAND|CMD|hallo||Touchscreen|
EOF


cat > $OUT_PATH/navit/ts/ts-wrapper << EOF
#!/bin/sh

cd /mnt/sdcard

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/sdcard/navit/lib

export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb
export TSLIB_TSDEVICE=/dev/ts
export TSLIB_CALIBFILE=/mnt/sdcard/navit/ts/pointercal
export TSLIB_CONFFILE=/mnt/sdcard/navit/ts/ts.conf
export TSLIB_PLUGINDIR=/mnt/sdcard/navit/lib/ts

/mnt/sdcard/navit/ts/ts_calibrate
/mnt/sdcard/navit/ts/ts_test
EOF

cd $OUT_PATH
zip -r $CIRCLE_ARTIFACTS/navitom.zip navit SDKRegistry
