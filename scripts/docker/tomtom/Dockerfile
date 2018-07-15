FROM ubuntu:14.04

RUN apt-get update  && apt-get install -y libglib2.0-dev git autogen autoconf libtool imagemagick zip \
	wget build-essential gettext cmake \
	&& apt-get clean && apt-get autoremove && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
RUN dpkg --add-architecture i386 && apt-get update \
	&& apt-get install -y libc6:i386 libncurses5:i386 libstdc++6:i386 \
	&& apt-get clean && apt-get autoremove && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV ARCH=arm-linux
ENV TOMTOM_SDK_DIR=/opt/tomtom-sdk
ENV PREFIX=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/$ARCH/sys-root
ENV PATH=$TOMTOM_SDK_DIR/gcc-3.3.4_glibc-2.3.2/bin:$PREFIX/bin/:$PATH
ENV CFLAGS="-O2 -I$PREFIX/include -I$PREFIX/usr/include"
ENV CPPFLAGS="-I$PREFIX/include -I$PREFIX/usr/include"
ENV LDFLAGS="-L$PREFIX/lib -L$PREFIX/usr/lib"
ENV CC=$ARCH-gcc
ENV CXX=$ARCH-g++
ENV LD=$ARCH-ld
ENV NM="$ARCH-nm -B"
ENV AR=$ARCH-ar
ENV RANLIB=$ARCH-ranlib
ENV STRIP=$ARCH-strip
ENV OBJCOPY=$ARCH-objcopy
ENV LN_S="ln -s"
ENV PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
RUN mkdir -p $TOMTOM_SDK_DIR

RUN wget -nv -c https://github.com/navit-gps/dependencies/raw/master/tomtom/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -P /tmp \
	&& tar xzf /tmp/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz -C $TOMTOM_SDK_DIR \
	&& rm /tmp/toolchain_redhat_gcc-3.3.4_glibc-2.3.2-20060131a.tar.gz

# zlib
RUN wget -nv -c http://zlib.net/zlib-1.2.11.tar.gz -P /tmp && cd /tmp/ \
	&& tar xzf zlib-1.2.11.tar.gz && cd zlib-1.2.11 \
	&& ./configure --prefix=$PREFIX && make && make install \
	&& rm -rf /tmp/zlib-1.2.11 /tmp/zlib-1.2.11.tar.gz

# libxml
RUN  wget -nv -c http://xmlsoft.org/sources/libxml2-2.7.8.tar.gz -P /tmp && cd /tmp/ \
	&& tar xzf libxml2-2.7.8.tar.gz && cd libxml2-2.7.8/ \
	&& ./configure --prefix=$PREFIX --host=$ARCH --without-python && make && make install \
	&& rm -rf /tmp/libxml2-2.7.8/ /tmp/libxml2-2.7.8.tar.gz

# libpng
RUN wget -nv -c https://github.com/navit-gps/dependencies/raw/master/tomtom/libpng-1.6.29.tar.gz -P /tmp && cd /tmp/ \
	&& tar xzf libpng-1.6.29.tar.gz && cd libpng-1.6.29/ \
	&& ./configure --prefix=$PREFIX --host=$ARCH  && make && make install \
	&& rm -rf /tmp/libpng-1.6.29/ /tmp/libpng-1.6.29.tar.gz

# Freetype
RUN wget -nv -c http://download.savannah.gnu.org/releases/freetype/freetype-2.5.0.tar.gz -P /tmp && cd /tmp/ \
	&& tar xzf freetype-2.5.0.tar.gz && cd freetype-2.5.0 \
	&& ./configure --prefix=$PREFIX --host=$ARCH &&  make && make install \
	&& rm -rf /tmp/freetype-2.5.0 /tmp/freetype-2.5.0.tar.gz

# freetype-config --cflags

# glib

# chmod a-w tomtom.cache
COPY tomtom.cache /tmp/

RUN wget -nv -c http://ftp.gnome.org/pub/gnome/sources/glib/2.25/glib-2.25.17.tar.gz -P /tmp && cd /tmp / \
	&& tar xzf glib-2.25.17.tar.gz && cd glib-2.25.17 \
	&& mv /tmp/tomtom.cache . \
	&& ./configure --prefix=$PREFIX --host=$ARCH --cache-file=tomtom.cache \
	&& sed -i "s|cp xgen-gmc gmarshal.c |cp xgen-gmc gmarshal.c \&\& sed -i \"s\|g_value_get_schar\|g_value_get_char\|g\" gmarshal.c |g" gobject/Makefile \
	&& make && make install \
	&& rm -rf /tmp/glib-2.25.17 /tmp/glib-2.25.17.tar.gz

# tslib
RUN git clone https://github.com/playya/tslib-svn.git /tmp/tslib-svn  && cd /tmp/tslib-svn/ \
	&& sed -i "s|AM_CONFIG_HEADER|AC_CONFIG_HEADERS|g" configure.ac \
	&& sed -i "119i\#ifdef EVIOCGRAB" plugins/input-raw.c \
	&& sed -i "124i\#endif" plugins/input-raw.c \
	&& sed -i "290i\#ifdef EVIOCGRAB" plugins/input-raw.c \
	&& sed -i "294i\#endif" plugins/input-raw.c \
	&& sed -i "s|# module_raw h3600|module_raw h3600|g" etc/ts.conf \
	&& ./autogen.sh && ./configure --prefix=$PREFIX --host=$ARCH \
	&& make && make install \
	&& rm -rf /tmp/tslib-svn/

# SDL
RUN wget -nv -c http://www.libsdl.org/release/SDL-1.2.15.tar.gz -P /tmp && cd /tmp/ \
	&& tar xzf SDL-1.2.15.tar.gz && cd SDL-1.2.15 \
	&& wget -nv -c http://tracks.yaina.de/source/sdl-fbcon-notty.patch \
	&& patch -p0 -i sdl-fbcon-notty.patch \
	&& ./configure --prefix=$PREFIX --host=$ARCH \
	   --disable-esd --disable-cdrom --disable-joystick --disable-video-x11 \
	   --disable-x11-vm --disable-dga --disable-video-x11-dgamouse \
	   --disable-video-x11-xv --disable-video-x11-xinerama --disable-video-directfb \
	   --enable-video-fbcon --disable-audio CFLAGS="$CFLAGS -DFBCON_NOTTY" \
	&& make && make install \
	&& cd test && ./configure --prefix=$PREFIX --host=$ARCH \
	&& make testvidinfo && cp testvidinfo $PREFIX/usr/bin/ \
	&& rm -rf /tmp/SDL-1.2.15 /tmp/SDL-1.2.15.tar.gz

# sdl image
RUN wget -nv -c http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12.tar.gz -P /tmp/ && cd /tmp/ \
	&& tar xzf SDL_image-1.2.12.tar.gz && cd SDL_image-1.2.12 \
	&& ./configure --prefix=$PREFIX --host=$ARCH && make && make install \
	&& rm -rf /tmp/SDL_image-1.2.12 /tmp/SDL_image-1.2.12.tar.gz

COPY entrypoint.sh /
CMD bash /entrypoint.sh
