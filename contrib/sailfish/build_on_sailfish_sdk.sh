#! /bin/sh
#run on the Sailfish OS sdk virtual machine. Check that rpmbuild directory exists.
#arm devices
sb2 -t SailfishOS-armv7hl -m sdk-build rpmbuild --define "_topdir /home/src1/rpmbuild" --define "navit_source `pwd`/../.." -bb navit-sailfish.spec
#intel devices
sb2 -t SailfishOS-i486 -m sdk-build rpmbuild --define "_topdir /home/src1/rpmbuild" --define "navit_source `pwd`/../.." -bb navit-sailfish.spec

