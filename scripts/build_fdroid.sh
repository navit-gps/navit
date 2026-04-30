#!/bin/bash -e

# setup fdroidserver
test -n "$fdroidserver" || source /etc/profile.d/bsenv.sh
test -d $fdroidserver || mkdir -p $fdroidserver
git ls-remote https://gitlab.com/fdroid/fdroidserver.git master
curl --silent https://gitlab.com/fdroid/fdroidserver/-/archive/master/fdroidserver-master.tar.gz | tar -xz --directory=$fdroidserver --strip-components=1
chown -R vagrant $fdroidserver

chgrp -R vagrant .
chmod -R g+r .
chmod g+w .
chgrp vagrant ..
chmod g+rx-w ..
test -d $home_vagrant/.android || mkdir $home_vagrant/.android
chown -R vagrant $home_vagrant/.android
export GRADLE_USER_HOME=$home_vagrant/.gradle
chown -R vagrant $home_vagrant
mkdir -p build/org.navitproject.navit
chown -R vagrant build
chgrp -R vagrant build
cd build/org.navitproject.navit
ls
git config --global --add safe.directory '*'
cd ..
cd ..

tar cf -  --exclude build . | ( cd build/org.navitproject.navit; tar xf -)
ls build
ls build/org.navitproject.navit
/home/vagrant/fdroidserver/fdroid build --verbose --on-server --no-tarball
