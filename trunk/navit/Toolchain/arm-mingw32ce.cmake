SET(CMAKE_SYSTEM_NAME WINCE)

# specify the cross compiler
FIND_PROGRAM(CMAKE_C_COMPILER NAMES arm-mingw32ce-gcc arm-wince-mingw32ce-gcc)
FIND_PROGRAM(CMAKE_CXX_COMPILER NAMES arm-mingw32ce-g++ arm-wince-mingw32ce-g++)
FIND_PROGRAM(CMAKE_RC_COMPILER_INIT NAMES arm-mingw32ce-windres arm-wince-mingw32ce-windres)
SET(PKG_CONFIG_EXECUTABLE "arm-mingw32ce-pkg-config")
SET(WINCE TRUE)
