set -e

COVERITY_VERSION="2017.07"

apt-get install -y libpng12-dev librsvg2-bin libfreetype6-dev libdbus-glib-1-dev libgtk2.0-dev curl

cmake_opts="-Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE -DSVG2PNG:BOOL=FALSE -DSAMPLE_MAP=n -Dgraphics/gtk_drawing_area:BOOL=TRUE"

mkdir ${CIRCLE_WORKING_DIRECTORY}/linux-bin && cd ${CIRCLE_WORKING_DIRECTORY}/linux-bin

if [[ "${CIRCLE_PROJECT_USERNAME}" == "navit-gps" && "${CIRCLE_BRANCH}" == "trunk" ]]; then
	# If we are building the official trunk code, push an update to coverity
	wget -nv -c -O /tmp/cov-analysis-linux64-${COVERITY_VERSION}.tar.gz http://sd-55475.dedibox.fr/cov-analysis-linux64-${COVERITY_VERSION}.tar.gz
	tar xfz /tmp/cov-analysis-linux64-${COVERITY_VERSION}.tar.gz --no-same-owner -C /usr/local/share/
	export PATH=/usr/local/share/cov-analysis-linux64-${COVERITY_VERSION}/bin:$PATH
	
	cov-build --dir cov-int cmake ${CIRCLE_WORKING_DIRECTORY}/ ${cmake_opts}
	cov-build --dir cov-int make -j $(nproc --all) || exit -1
	tar czvf navit.tgz cov-int
	
	curl --form token=$COVERITY_TOKEN \
  --form email=$COVERITY_EMAIL \
  --form file=@navit.tgz \
  --form version="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
  --form description="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
  https://scan.coverity.com/builds?project=$CIRCLE_PROJECT_USERNAME

	# Then update the translation template on launchpad
	sed -i '/INTEGER/d' po/navit.pot
	cp po/navit.pot $CIRCLE_ARTIFACTS/
	curl "https://translations.launchpad.net/navit/${CIRCLE_BRANCH}/+translations-upload" -H "$lp_cookie" -H "Referer: https://translations.launchpad.net/navit/${CIRCLE_BRANCH}/+translations-upload" -F file=@po/navit.pot | grep title

else
	cmake ${CIRCLE_WORKING_DIRECTORY}/ ${cmake_opts} || exit -1
	make -j $(nproc --all) || exit -1
fi

if [[ "$CIRCLE_ARTIFACTS" != "" ]]; then
	cp -r navit/icons $CIRCLE_ARTIFACTS
fi
