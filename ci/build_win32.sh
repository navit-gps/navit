sudo apt-get install mingw32 mingw32-binutils mingw32-runtime default-jdk nsis

mkdir win32
pushd win32
cmake -Dbinding/python:BOOL=FALSE -DSAMPLE_MAP=n -DCMAKE_TOOLCHAIN_FILE=../Toolchain/mingw32.cmake ../ && make && make package
popd

cp win32/*.exe $CIRCLE_ARTIFACTS/
