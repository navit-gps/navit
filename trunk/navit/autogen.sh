#!/bin/sh
if [ `uname` = Darwin ]; then
	LIBTOOL=glibtool
else
	LIBTOOL=libtool
fi

for pkg in pkg-config $LIBTOOL automake aclocal autoreconf:autoconf autopoint:gettext
do
	if ! ${pkg%%:*} --version >/dev/null 
	then
		echo "You need to install ${pkg##*:}"
		exit 1
	fi
done

autoreconf --install -I m4 "$@"
