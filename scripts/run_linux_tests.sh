#!/bin/bash

linux_test () {
	# create logs dir
	mkdir $CIRCLE_ARTIFACTS/logs_${1}

	#run instance of navit and remember pid
	./navit >$CIRCLE_ARTIFACTS/logs_${1}/stdout.txt 2>$CIRCLE_ARTIFACTS/logs_${1}/stderr.txt &
	pid=$!

	# give navit time to come up
	sleep 5

	# screen shot root window
	import -window root $CIRCLE_ARTIFACTS/logs_${1}/default.png

	# run tests on X11
	bash ~/navit/scripts/xdotools.sh ${1}

	# python ~/navit/scripts/dbus_tests.py $CIRCLE_TEST_REPORTS/
	# dbus-send  --print-reply --session --dest=org.navit_project.navit /org/navit_project/navit/default_navit org.navit_project.navit.navit.quit

	# kill navit instance
	kill $pid
}

set -e

pushd ~/linux-bin/navit/

# prepare environment by getting a map of berkley
cat > maps/berkeley.xml << EOF
<map type="binfile" data="\$NAVIT_SHAREDIR/maps/berkeley.bin" />
EOF
wget http://sd-55475.dedibox.fr/berkeley.bin -O maps/berkeley.bin

# back up config
cp navit.xml navit.xml.bak

# run gtk test
sed -i -e 's@name="Local GPS" profilename="car" enabled="yes" active="1"@name="Local GPS" profilename="car" enabled="no" active="0"@' navit.xml
sed -i -e 's@name="Demo" profilename="car" enabled="no" @name="Demo" profilename="car" enabled="yes" follow="1" refresh="1"@' navit.xml
sed -i -e 's@type="internal" enabled@type="internal" fullscreen="1" font_size="350" enabled@' navit.xml
sed -i -e 's@libbinding_dbus.so" active="no"@libbinding_dbus.so" active="yes"@' navit.xml
linux_test gtk_drawing_area

# restore config
cp navit.xml.bak navit.xml
# run qt5 qwidget test
sed -i -e 's@graphics type="gtk_drawing_area"@graphics type="qt5" w="792" h="547" qt5_widget="qwidget"@' navit.xml
sed -i -e 's@name="Local GPS" profilename="car" enabled="yes" active="1"@name="Local GPS" profilename="car" enabled="no" active="0"@' navit.xml
sed -i -e 's@name="Demo" profilename="car" enabled="no" @name="Demo" profilename="car" enabled="yes" follow="1" refresh="1"@' navit.xml
sed -i -e 's@type="internal" enabled@type="internal" fullscreen="1" font_size="350" enabled@' navit.xml
sed -i -e 's@libbinding_dbus.so" active="no"@libbinding_dbus.so" active="yes"@' navit.xml
linux_test qt5_widget

# restore config
cp navit.xml.bak navit.xml
# run qt5 qml test
sed -i -e 's@graphics type="gtk_drawing_area"@graphics type="qt5" w="792" h="547" qt5_widget="qml"@' navit.xml
sed -i -e 's@name="Local GPS" profilename="car" enabled="yes" active="1"@name="Local GPS" profilename="car" enabled="no" active="0"@' navit.xml
sed -i -e 's@name="Demo" profilename="car" enabled="no" @name="Demo" profilename="car" enabled="yes" follow="1" refresh="1"@' navit.xml
sed -i -e 's@type="internal" enabled@type="internal" fullscreen="1" font_size="350" enabled@' navit.xml
sed -i -e 's@libbinding_dbus.so" active="no"@libbinding_dbus.so" active="yes"@' navit.xml
linux_test qt5_qml

