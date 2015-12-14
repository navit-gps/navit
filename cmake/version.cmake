FIND_PROGRAM(GIT_EXECUTABLE NAMES git git.exe DOC "git command line client")

get_filename_component(SOURCE_DIR ${SRC} PATH)

if (GIT_EXECUTABLE)
   EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log "--format=%h %d"
      WORKING_DIRECTORY "${SOURCE_DIR}"
      OUTPUT_VARIABLE VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
   )
   if(NOT VERSION)
      message(STATUS "git found, but no git tree")
      set(VERSION "0000")
   endif()
else()
   message(STATUS "git not found, cannot record git commit")
   set(VERSION "0000")
endif(GIT_EXECUTABLE)

string(REGEX MATCH "R[0-9]+" VERSION_NUM ${VERSION} )
if(NOT VERSION_NUM)
   message(STATUS "I can't find a release tag. This is probably not Navit's official tree")
   message(STATUS "It's OK, I will default to 0000")
   set(VERSION_NUM "0000")
endif()
string(REPLACE "R" "" VERSION_NUM ${VERSION_NUM} )
if(NOT VERSION_NUM)
   set(VERSION_NUM "0000")
endif()

string(REGEX MATCH "^[a-z0-9]+" VERSION ${VERSION} )

if (STRIP_M)
   set(VERSION ${VERSION_NUM})
endif()

set(NAVIT_VARIANT "-")

set(${NAME} ${VERSION})

message (STATUS "Git commit: ${${NAME}}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
