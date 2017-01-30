#!/bin/bash

set -e

pushd ~/linux-bin/navit/
sed -i -e 's@name="Local GPS" profilename="car" enabled="yes" active="1"@name="Local GPS" profilename="car" enabled="no" active="0"@' navit.xml
sed -i -e 's@name="Demo" profilename="car" enabled="no" active="yes"@name="Demo" profilename="car" enabled="yes" active="yes" follow="1" refresh="1"@' navit.xml
sed -i -e 's@type="internal" enabled@type="internal" fullscreen="1" font_size="350" enabled@' navit.xml
sed -i -e 's@libbinding_dbus.so" active="no"@libbinding_dbus.so" active="yes"@' navit.xml

./navit &
pid=$!

import -window root $CIRCLE_ARTIFACTS/default.png

# python ~/navit/ci/dbus_tests.py $CIRCLE_TEST_REPORTS/
# 
# dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.quit
