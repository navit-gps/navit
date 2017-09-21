mkdir wince && cd wince
cmake ../ -DCMAKE_TOOLCHAIN_FILE=Toolchain/arm-mingw32ce.cmake -DXSLTS=windows -DCACHE_SIZE=10485760 -Dsvg2png_scaling:STRING=16,32 -Dsvg2png_scaling_nav:STRING=32 -Dsvg2png_scaling_flag=16 -DSAMPLE_MAP=n 
make
