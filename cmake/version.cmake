FIND_PROGRAM(GIT_EXECUTABLE NAMES git git.exe DOC "git command line client")
FIND_PROGRAM(DATE_EXECUTABLE NAMES date DOC "unix date command")

get_filename_component(SOURCE_DIR ${SRC} PATH)

if (GIT_EXECUTABLE)
   EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log "--format=%ct" # output as unix timestamp
      WORKING_DIRECTORY "${SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_OUTPUT_DATE
      ERROR_VARIABLE GIT_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
   )
   if(GIT_ERROR)
      message(STATUS "Cannot determine current git commit - git error: '${GIT_ERROR}'")
      set(GIT_OUTPUT_DATE "0000000000")
   endif(GIT_ERROR)
   EXECUTE_PROCESS(
    COMMAND ${GIT_EXECUTABLE} log "--format='%h'"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_OUTPUT_HASH
    ERROR_VARIABLE GIT_ERROR_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
   )
   if(GIT_ERROR_HASH)
      message(STATUS "Cannot determine current git hash - git error: '${GIT_ERROR}'")
      set(GIT_OUTPUT_HASH "xdevxgitxnotxfound")
   endif(GIT_ERROR_HASH)
else()
   message(STATUS "git not found, cannot record git commit")
   set(GIT_OUTPUT_DATE "0000000000") # To match length of android versionCode
   set(GIT_OUTPUT_HASH "xdevxgitxnotxfound")
endif(GIT_EXECUTABLE)

string(REGEX MATCH "^[0-9]+" VERSION_NUM ${GIT_OUTPUT_DATE} )

set(VERSION ${GIT_OUTPUT_HASH})

EXECUTE_PROCESS(
   COMMAND ${DATE_EXECUTABLE} "+%y%m%d%H%M" "-d \@${VERSION_NUM}" # output as unix timestamp
   WORKING_DIRECTORY "${SOURCE_DIR}"
   OUTPUT_VARIABLE DATE_CONVERT_OUTPUT
   ERROR_VARIABLE DATE_CONVERT_ERROR
   OUTPUT_STRIP_TRAILING_WHITESPACE
   ERROR_STRIP_TRAILING_WHITESPACE
)
if(NOT DATE_CONVERT_ERROR)
   string(REGEX MATCH "[0-9]+" VERSION_CODE ${DATE_CONVERT_OUTPUT})
else(NOT DATE_CONVERT_ERROR)
   message(FATAL_ERROR "Date convert not working\nError message:\n${DATE_CONVERT_ERROR}")
endif(NOT DATE_CONVERT_ERROR)

string(REGEX MATCH "[a-z0-9]+" VERSION ${GIT_OUTPUT_HASH} )

if (STRIP_M)
   set(VERSION ${VERSION_CODE})
endif()

set(NAVIT_VARIANT "-")
set(GIT_VERSION VERSION)

set(${NAME} ${VERSION})

message (STATUS "Git commit: ${VERSION}")
message (STATUS "Git   date: ${VERSION_CODE}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
