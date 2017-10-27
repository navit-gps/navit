
#!/bin/bash
set -x

find .

export MING_PATH=$(pwd)/opt/mingw32ce
export SOURCE_PATH=$START_PATH"/"${CIRCLE_PROJECT_REPONAME}"/"
export CMAKE_FILE=$SOURCE_PATH"/Toolchain/arm-mingw32ce.cmake"
export PATH=$PATH:$MING_PATH/bin

echo "#####PATH IS:" $PATH

wget -c https://sourceforge.net/projects/cegcc/files/cegcc/0.59.1/mingw32ce-0.59.1.tar.bz2/download 
echo "##########PWD IS:" pwd
mv download ming232ce.tar.bz
tar xjf ming232ce.tar.bz

find .

cmake -DCMAKE_TOOLCHAIN_FILE=Toolchain/arm-mingw32ce.cmake -DXSLTS=windows -DCACHE_SIZE=10485760 -Dsvg2png_scaling:STRING=16,32 -Dsvg2png_scaling_nav:STRING=32 -Dsvg2png_scaling_flag=16 -DSAMPLE_MAP=n .
make
