SET(CMAKE_SYSTEM_NAME Windows)

FIND_PROGRAM(CMAKE_C_COMPILER NAMES i686-w64-mingw32-gcc i686-mingw32-gcc i586-mingw32-gcc i386-mingw32-gcc i586-mingw32msvc-gcc mingw32-gcc)
FIND_PROGRAM(CMAKE_CXX_COMPILER NAMES i686-w64-mingw32-g++ i686-mingw32-g++ i586-mingw32-g++ i386-mingw32-g++ i586-mingw32msvc-g++ mingw32-g++)

FIND_PROGRAM(CMAKE_RC_COMPILER_INIT NAMES i686-w64-mingw32-windres i686-mingw32-windres i586-mingw32-windres i386-mingw32-windres mingw32-windres i586-mingw32msvc-windres windres.exe)

FIND_PROGRAM(CMAKE_AR NAMES i686-w64-mingw32-ar i686-mingw32-ar i586-mingw32-ar i386-mingw32-ar i586-mingw32msvc-ar mingw32-ar ar.exe)

IF (NOT CMAKE_FIND_ROOT_PATH)
EXECUTE_PROCESS(
   COMMAND ${CMAKE_C_COMPILER} -print-sysroot
   OUTPUT_VARIABLE CMAKE_FIND_ROOT_PATH
)
ENDIF(NOT CMAKE_FIND_ROOT_PATH)

set(PKG_CONFIG_EXECUTABLE "mingw32-pkg-config")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
