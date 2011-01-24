FIND_PROGRAM(GIT_EXECUTABLE git DOC "git command line client")

get_filename_component(SOURCE_DIR ${SRC} PATH)

EXECUTE_PROCESS(
     COMMAND ${GIT_EXECUTABLE} svn info
     COMMAND grep "Revision"
     WORKING_DIRECTORY "${SOURCE_DIR}"
     OUTPUT_VARIABLE VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE
)

set( ${NAME} "unknown" )

if (NOT VERSION)
   FIND_PACKAGE(Subversion)

   if (Subversion_FOUND)

   EXECUTE_PROCESS(
      COMMAND svnversion
      WORKING_DIRECTORY "${SOURCE_DIR}"
      OUTPUT_VARIABLE VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
   )
   endif(Subversion_FOUND)

   if (VERSION)
      set( ${NAME} ${VERSION} )
   endif(VERSION)
else()
   string(REGEX REPLACE "Revision: " "" ${NAME} ${VERSION})
endif(NOT VERSION)

set(NAVIT_VARIANT "-")

message (STATUS "SVN-version ${${NAME}}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
