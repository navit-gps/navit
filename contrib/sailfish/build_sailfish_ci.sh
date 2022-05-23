#!/bin/sh
#run on the Sailfish OS sdk docker container. Check that rpmbuild directory exists. Remember to export VERSION_ID
# please don't mess around with those lines without testing,
# even if some fancy tool tells you to do so to save some pipes.
# -hoehnp-

if [ -z ${VERSION_ID+x} ]; then echo "VERSION_ID not set. Forgot to export VERSION_ID?"; exit 1; fi

# First we need to cd to the directory containing this script and the spec file.
# Makes calling it directly from docker easier.
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
cd $SCRIPTPATH
ls -lah /navit
mkdir $HOME/rpmbuild

#arm devices
sb2 -t SailfishOS-${VERSION_ID}-armv7hl -m sdk-install -R zypper --non-interactive in $(grep "^BuildRequires: " navit-sailfish.spec | sed -e "s/BuildRequires: //")
sb2 -t SailfishOS-${VERSION_ID}-armv7hl -m sdk-build rpmbuild --define "_topdir /home/nemo/rpmbuild" --define "navit_source ${SCRIPTPATH}/../.." -bb navit-sailfish.spec
#intel devices
#sb2 -t SailfishOS-${VERSION_ID}-i486 -m sdk-install -R zypper --non-interactive in $(grep "^BuildRequires: " navit-sailfish.spec | sed -e "s/BuildRequires: //")
#sb2 -t SailfishOS-${VERSION_ID}-i486 -m sdk-build rpmbuild --define "_topdir /home/nemo/rpmbuild" --define "navit_source ${SCRIPTPATH}/../.." -bb navit-sailfish.spec

