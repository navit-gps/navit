#!/bin/bash
set -e

COVERITY_VERSION="2017.07"
BUILD_PATH="linux"

cmake_opts="-Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE -DSVG2PNG:BOOL=FALSE -DSAMPLE_MAP=n -Dgraphics/gtk_drawing_area:BOOL=TRUE"

[ -d $BUILD_PATH ] || mkdir -p $BUILD_PATH
pushd $BUILD_PATH

# Build everything
    echo "Building..."
cmake ${cmake_opts} ../
make -j $(nproc --all)
make package

if [[ "${CIRCLE_PROJECT_USERNAME}" == "navit-gps" && "${CIRCLE_BRANCH}" == "trunk" ]]; then
	# If we are building the official trunk code, push an update to coverity
    # Temporarily disabled because Coverity is down.
    # TODO on the long run, CI should not fail just because the Coverity test did not run,
    # especially if the test results are not taken into account.
    #echo "Pushing an update to coverity as we are building the official trunk code."
    #echo "Downloading coverity..."
	#curl \
	#    -X POST --data "token=${COVERITY_TOKEN}&project=${CIRCLE_PROJECT_USERNAME}" \
	#    -o /tmp/cov-analysis-linux64-${COVERITY_VERSION}.tar.gz -s \
	#    https://scan.coverity.com/download/linux64

    #echo "Unpacking coverity..."
	#tar xfz /tmp/cov-analysis-linux64-${COVERITY_VERSION}.tar.gz --no-same-owner -C /usr/local/share/
	#export PATH=/usr/local/share/cov-analysis-linux64-${COVERITY_VERSION}/bin:$PATH

    #echo "Re-running build with coverity..."
	#cov-build --dir cov-int make -j $(nproc --all)
	#tar czvf navit.tgz cov-int

#	curl --form token=$COVERITY_TOKEN \
#  --form email=$COVERITY_EMAIL \
#  --form file=@navit.tgz \
#  --form version="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
#  --form description="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
#  https://scan.coverity.com/builds?project=$CIRCLE_PROJECT_USERNAME

	# Then update the translation template on launchpad
	echo "Updating the translation template on launchpad..."
	sed -i '/INTEGER/d' po/navit.pot
	cp po/navit.pot $CIRCLE_ARTIFACTS/
	curl "https://translations.launchpad.net/navit/${CIRCLE_BRANCH}/+translations-upload" -H "$lp_cookie" -H "Referer: https://translations.launchpad.net/navit/${CIRCLE_BRANCH}/+translations-upload" -F file=@po/navit.pot | grep title
fi

if [[ "$CIRCLE_ARTIFACTS" != "" ]]; then
	echo "Copying icons to artifacts..."
	cp -r navit/icons $CIRCLE_ARTIFACTS
fi
