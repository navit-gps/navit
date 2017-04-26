# $Id$
# Authority: dries
#%global navit_version_minor %(grep NAVIT_VERSION_MINOR ../../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
#%global navit_version_major %(grep NAVIT_VERSION_MAJOR ../../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
#%global navit_version_patch %(grep NAVIT_VERSION_PATCH ../../CMakeLists.txt |head -1| sed -e s/[^0-9]//g)
#%global navit_version %{navit_version_major}.%{navit_version_minor}.%{navit_version_patch}
#%global git_version %(git describe --tags | sed y/-/_/)

Name: harbour-navit
Summary: Open Source car navigation system
#Version: %{navit_version}_%{git_version}
Version: 0.5.1
Release: 1
License: GPL
Group: Applications/Productivity
URL: http://navit-projet.org/

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

#Requires: glib2
#Requires: gettext-libs
Requires: freetype
#Requires: zlib
#Requires: qt5-qtcore
#Requires: qt5-qtdeclarative
#Requires: qt5-qtdbus
Requires: qt5-qtpositioning
#Requires: qt5-qtxml
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
       -DPACKAGE:STRING=harbour-navit \
       -DNAVIT_BINARY:STRING=harbour-navit \
       -DSHARE_DIR:PATH=share/harbour-navit \
       -DLOCALE_DIR:PATH=share/harbour-navit/locale \
       -DIMAGE_DIR:PATH=share/harbour-navit/xpm \
       -DLIB_DIR:PATH=share/harbour-navit/lib \
       -DBUILD_MAPTOOL:BOOL=FALSE \
       -Dbinding/dbus:BOOL=FALSE \
       -Dgraphics/gtk_drawing_area:BOOL=FALSE \
       -Dgraphics/null:BOOL=FALSE \
       -Dgraphics/opengl:BOOL=FALSE \
       -Dgraphics/sdl:BOOL=FALSE \
       -Dspeech/dbus:BOOL=FALSE \
       -Dvehicle/gpsd:BOOL=FALSE \
       -Dvehicle/gpsd_dbus:BOOL=FALSE \
       -DUSE_PLUGINS=n \
       -DUSE_QWIDGET:BOOL=FALSE \
         %{navit_real_source}
%{__make}

#       -DMAN_DIR:PATH=share/harbour-navit/man1 

%install
%make_install
#copy in sailfish config
cp %{navit_real_source}/contrib/sailfish/navit.xml %{buildroot}/usr/share/harbour-navit/navit.xml
#copy in espeak script
cp %{navit_real_source}/contrib/sailfish/say_de_DE.sh %{buildroot}/usr/bin/say_de_DE.sh

%files
%defattr(644, root, root, 755)
%{_datadir}/harbour-navit/navit.xml
%{_datadir}/harbour-navit/xpm/
%{_datadir}/harbour-navit/maps/osm_bbox_11.3,47.9,11.7,48.2.bin
%{_datadir}/applications/harbour-navit.desktop
%{_datadir}/icons/hicolor/256x256/apps/harbour-navit.png
%{_datadir}/icons/hicolor/128x128/apps/harbour-navit.png
%{_datadir}/icons/hicolor/108x108/apps/harbour-navit.png
%{_datadir}/icons/hicolor/86x86/apps/harbour-navit.png
%{_datadir}/icons/hicolor/22x22/apps/harbour-navit.png
%{_datadir}/harbour-navit/locale/
%attr(755, root, root) %{_bindir}/harbour-navit
%attr(755, root, root) %{_bindir}/say_de_DE.sh
%doc %{_mandir}/man1/harbour-navit.1.gz
%doc %{_mandir}/man1/maptool.1.gz


%changelog
*Mon Dec 14 2015 Initial sailfish release
*Mon Apr 10 2017 Almost harbour valid
- Initial package.
