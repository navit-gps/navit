sudo apt-get install cmake libpng12-dev librsvg2-bin libfreetype6-dev libdbus-glib-1-dev g++ libgtk2.0-dev libqt5svg5-dev

cmake_opts="-Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE -DSVG2PNG:BOOL=FALSE -DSAMPLE_MAP=n -Dgraphics/gtk_drawing_area:BOOL=TRUE"

if [[ "${CIRCLE_PROJECT_USERNAME}" == "navit-gps" && "${CIRCLE_BRANCH}" == "trunk" ]]; then
	# If we are building the official trunk code, push an update to coverity
	wget -nv -c -O ~/assets/cov-analysis-linux64-7.6.0.tar.gz http://sd-55475.dedibox.fr/cov-analysis-linux64-7.6.0.tar.gz
	tar xfz ~/assets/cov-analysis-linux64-7.6.0.tar.gz
	export PATH=~/navit/cov-analysis-linux64-7.6.0/bin:$PATH
	
	mkdir ~/linux-bin && cd ~/linux-bin
	cov-build --dir cov-int cmake ~/${CIRCLE_PROJECT_REPONAME}/ ${cmake_opts}
	cov-build --dir cov-int make || exit -1
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
	mkdir ~/linux-bin && cd ~/linux-bin
	cmake ~/${CIRCLE_PROJECT_REPONAME}/ ${cmake_opts} || exit -1
	make  || exit -1
fi

if [[ "$CIRCLE_ARTIFACTS" != "" ]]; then
	cp -r navit/xpm $CIRCLE_ARTIFACTS
fi


# Done with the builds tests. Running some app tests 
bash ~/navit/ci/run_linux_tests.sh
