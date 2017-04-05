# $Id$
# Authority: dries
%global navit_version_minor %(grep NAVIT_VERSION_MINOR ../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
%global navit_version_major %(grep NAVIT_VERSION_MAJOR ../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
%global navit_version_patch %(grep NAVIT_VERSION_PATCH ../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
%global navit_version %{navit_version_major}.%{navit_version_minor}.%{navit_version_patch}
%global git_version %(git describe --tags | sed y/-/_/)

Name: navit
Summary: Open Source car navigation system
Version: %{navit_version}_%{git_version}
Release: metalstrolch
License: GPL
Group: Applications/Productivity
URL: http://navit.sourceforge.net/

BuildRequires: gcc
BuildRequires: cmake
BuildRequires: glib2-devel
BuildRequires: gettext-devel
BuildRequires: freetype-devel
BuildRequires: zlib-devel
BuildRequires: qt5-qtcore-devel
BuildRequires: qt5-qtdeclarative-devel
BuildRequires: qt5-qtdbus-devel
BuildRequires: qt5-qtpositioning-devel
BuildRequires: qt5-qtxml-devel
BuildRequires: qt5-qtsvg-devel

Requires: glib2
Requires: gettext-libs
Requires: freetype
Requires: zlib
Requires: qt5-qtcore
Requires: qt5-qtdeclarative
Requires: qt5-qtdbus
Requires: qt5-qtpositioning
Requires: qt5-qtxml
Requires: qt5-qtsvg

%global navit_real_source %{navit_source}

%description
Navit is a car navigation system with routing engine.

It's modular design is capable of using vector maps of various formats for routing and rendering of the displayed map. It's even possible to use multiple maps at a time.

The GTK+ or SDL user interfaces are designed to work well with touch screen displays. Points of Interest of various formats are displayed on the map.

The current vehicle position is either read from gpsd or directly from NMEA GPS sensors.

The routing engine not only calculates an optimal route to your destination, but also generates directions and even speaks to you using speechd.

%prep
#just create empty directory here for build files. We'll use git sources directly.
rm -rf navit-build
mkdir navit-build
#nothing to delete and nothing to extract. Just setup for our empty directory created before.
%setup -D -T -n navit-build

%build
%define debug_package %{nil}
%{__rm} -rf %{buildroot}
#cmake git files directly 
cmake  -DCMAKE_INSTALL_PREFIX:PATH=/usr \
       -DBUILD_MAPTOOL:BOOL=FALSE \
       -Dspeech/dbus:BOOL=FALSE \
       -Dbinding/dbus:BOOL=FALSE \
       -Dvehicle/gpsd_dbus:BOOL=FALSE \
       -DUSE_QWIDGET:BOOL=FALSE \
       -DSAMPLE_MAP=n \
         %{navit_real_source}
%{__make}

%install
%make_install
#copy in sailfish config
cp %{navit_real_source}/sailfish/navit.xml %{buildroot}/usr/share/navit/navit.xml
#copy in espeak script
cp %{navit_real_source}/sailfish/say_de_DE.sh %{buildroot}/usr/bin/say_de_DE.sh

%files
%defattr(-, root, root, -)
%{_datadir}/navit/navit.xml
%{_datadir}/navit/xpm/
#%{_datadir}/navit/maps/osm_bbox_11.3,47.9,11.7,48.2.bin
%{_datadir}/applications/navit.desktop
%{_datadir}/icons/hicolor/128x128/apps/navit.png
%{_datadir}/icons/hicolor/22x22/apps/navit.png
%{_datadir}/locale/
%{_bindir}/navit
%{_bindir}/say_de_DE.sh
%{_libdir}/navit/gui/libgui_internal.so
%{_libdir}/navit/map/libmap_mg.so
%{_libdir}/navit/map/libmap_csv.so
%{_libdir}/navit/map/libmap_shapefile.so
%{_libdir}/navit/map/libmap_filter.so
%{_libdir}/navit/map/libmap_textfile.so
%{_libdir}/navit/map/libmap_binfile.so
%{_libdir}/navit/osd/libosd_core.so
%{_libdir}/navit/font/libfont_freetype.so
%{_libdir}/navit/graphics/libgraphics_qt5.so
%{_libdir}/navit/graphics/libgraphics_null.so
%{_libdir}/navit/speech/libspeech_cmdline.so
%{_libdir}/navit/vehicle/libvehicle_file.so
%{_libdir}/navit/vehicle/libvehicle_socket.so
%{_libdir}/navit/vehicle/libvehicle_pipe.so
%{_libdir}/navit/vehicle/libvehicle_serial.so
%{_libdir}/navit/vehicle/libvehicle_demo.so
%{_libdir}/navit/vehicle/libvehicle_qt5.so
%doc %{_mandir}/man1/navit.1.gz
%doc %{_mandir}/man1/maptool.1.gz


%changelog
*Mon Dec 14 2015 Initial sailfish release
- Initial package.
