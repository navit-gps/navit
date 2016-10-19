set ( CMAKE_SYSTEM_NAME GNU )

# specify the cross compiler
find_program ( CMAKE_C_COMPILER NAMES arm-linux-gcc )
find_program ( CMAKE_CXX_COMPILER NAMES arm-linux-g++ )

set ( TOMTOM_SDK_DIR /opt/tomtom-sdk )
set ( CMAKE_FIND_ROOT_PATH ${TOMTOM_SDK_DIR} )

set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )
add_definitions(-D_GNU_SOURCE)
