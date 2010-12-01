SET(CMAKE_SYSTEM_NAME WINCE)

# specify the cross compiler
SET(CMAKE_C_COMPILER "arm-mingw32ce-gcc")
SET(CMAKE_CXX_COMPILER "arm-mingw32ce-g++")
SET(PKG_CONFIG_EXECUTABLE "arm-mingw32ce-pkg-config")
SET(WINCE TRUE)
