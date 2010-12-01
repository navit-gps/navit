SET(CMAKE_SYSTEM_NAME WINCE)

# specify the cross compiler
SET(CMAKE_C_COMPILER "arm-mingw32ce-gcc")
SET(CMAKE_CXX_COMPILER "arm-mingw32ce-g++")
SET(WINCE TRUE)

# where is the target environment
#SET(CMAKE_FIND_ROOT_PATH /usr/i686-pc-mingw32/sys-root/mingw)

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

