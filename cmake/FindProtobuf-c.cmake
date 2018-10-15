#  PROTOBUF-C_FOUND - System has libyang
#  PROTOBUF-C_INCLUDE_DIRS - The libyang include directories
#  PROTOBUF-C_LIBRARIES - The libraries needed to use libyang
#  PROTOBUF-C_DEFINITIONS - Compiler switches required for using libyang

function(PROTOBUF-C_GENERATE SRCS HDRS)
   if(NOT ARGN)
     message(SEND_ERROR "Error: PROTOBUFC_GENERATE() called without any proto files")
     return()
   endif()

   if(PROTOBUF-C_GENERATE_APPEND_PATH)
     # Create an include path for each file specified
     foreach(FIL ${ARGN})
       get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
       get_filename_component(ABS_PATH ${ABS_FIL} PATH)
       list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
       if(${_contains_already} EQUAL -1)
           list(APPEND _protobuf_include_path -I ${ABS_PATH})
       endif()
     endforeach()
   else()
     set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
   endif()

   if(DEFINED PROTOBUFC_IMPORT_DIRS)
     foreach(DIR ${PROTOBUF_IMPORT_DIRS})
       get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
       list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
       if(${_contains_already} EQUAL -1)
           list(APPEND _protobuf_include_path -I ${ABS_PATH})
       endif()
     endforeach()
   endif()

   set(${SRCS})
   set(${HDRS})
   foreach(FIL ${ARGN})
     get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
     get_filename_component(FIL_WE ${FIL} NAME_WE)

     list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.c")
     list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

     add_custom_command(
       OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.c"
              "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
       COMMAND  ${PROTOBUFC_PROTOC_EXECUTABLE}
       ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FIL}
       DEPENDS ${ABS_FIL}
       COMMENT "Running C protocol buffer compiler on ${FIL}"
       VERBATIM )
   endforeach()

   set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
   set(${SRCS} ${${SRCS}} PARENT_SCOPE)
   set(${HDRS} ${${HDRS}} PARENT_SCOPE)
 endfunction()

# Find the protoc Executable
find_program(PROTOBUFC_PROTOC_EXECUTABLE
    NAMES protoc-c
    DOC "protoc-c compiler"
)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_PROTOBUF-C QUIET protobuf-c)
    set(PROTOBUF-C_DEFINITIONS ${PC_PROTOBUF-C_CFLAGS_OTHER})
endif()

find_path(PROTOBUF-C_INCLUDE_DIR protobuf-c/protobuf-c.h
    HINTS ${PC_PROTOBUF-C_INCLUDEDIR} ${PC_PROTOBUF-C_INCLUDE_DIRS}
          PATH_SUFFIXES protobuf-c)

find_library(PROTOBUF-C_LIBRARY NAMES protobuf-c
    HINTS ${PC_PROTOBUF-C_LIBDIR} ${PC_PROTOBUF-C_LIBRARY_DIRS})

set(PROTOBUF-C_LIBRARIES ${PROTOBUF-C_LIBRARY} )
set(PROTOBUF-C_INCLUDE_DIRS ${PROTOBUF-C_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(protobuf-c  DEFAULT_MSG
    PROTOBUF-C_LIBRARY PROTOBUF-C_INCLUDE_DIR)

mark_as_advanced(PROTOBUF-C_INCLUDE_DIR YANG_LIBRARY)
