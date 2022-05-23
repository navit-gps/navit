#! /bin/sh
#run on the Sailfish OS sdk virtual machine. Check that rpmbuild directory exists. Remember to export VERSION_ID

# please don't mess around with those lines without testing,
# even if some fancy tool tells you to do so to save some pipes.
# -metalstrolch-

if [ -z ${VERSION_ID+x} ]; then echo "VERSION_ID not set. Forgot to export VERSION_ID?"; exit 1; fi

#arm devices
sb2 -t SailfishOS-${VERSION_ID}-armv7hl -m sdk-install -R zypper in $(grep "^BuildRequires: " navit-sailfish.spec | sed -e "s/BuildRequires: //")
sb2 -t SailfishOS-${VERSION_ID}-armv7hl -m sdk-build rpmbuild --define "_topdir /home/src1/rpmbuild" --define "navit_source $(pwd)/../.." -bb navit-sailfish.spec
#intel devices
sb2 -t SailfishOS-${VERSION_ID}-i486 -m sdk-install -R zypper in $(grep "^BuildRequires: " navit-sailfish.spec | sed -e "s/BuildRequires: //")
sb2 -t SailfishOS-${VERSION_ID}-i486 -m sdk-build rpmbuild --define "_topdir /home/src1/rpmbuild" --define "navit_source $(pwd)/../.." -bb navit-sailfish.spec

