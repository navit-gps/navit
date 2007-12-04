#!/bin/sh
if ! pkg-config --version >/dev/null
then
	echo "You need to install pkg-config"
	exit 1
fi

if ! libtool --version >/dev/null
then
	echo "You need to install libtool"
	exit 1
fi

autoreconf --force --install -I m4
