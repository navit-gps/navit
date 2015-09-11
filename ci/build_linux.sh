if [[ "${CIRCLE_BRANCH}" == "trunk" ]]; then
	tar xfz ~/assets/cov-analysis-linux64-7.6.0.tar.gz
	export PATH=~/navit/cov-analysis-linux64-7.6.0/bin:$PATH
	
	mkdir bin && cd bin
	cov-build --dir cov-int cmake ../ -Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE
	cov-build --dir cov-int make || exit -1
	tar czvf navit.tgz cov-int
	
	curl --form token=$COVERITY_TOKEN \
  --form email=$COVERITY_EMAIL \
  --form file=@navit.tgz \
  --form version="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
  --form description="${CIRCLE_BRANCH}-$CIRCLE_SHA1" \
  https://scan.coverity.com/builds?project=$CIRCLE_PROJECT_USERNAME
else
	mkdir bin && cd bin
	cmake ../ -Dgraphics/qt_qpainter:BOOL=FALSE -Dgui/qml:BOOL=FALSE || exit -1
	make  || exit -1
fi
