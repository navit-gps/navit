#FIND_PACKAGE(Subversion)

#if (Subversion_FOUND)
#    Subversion_WC_INFO("${PROJECT_SOURCE_DIR}" Project)
#    MESSAGE("Current revision is ${Project_WC_REVISION}")
#    set (VERSION ${Project_WC_REVISION})
#    Subversion_WC_LOG(${PROJECT_SOURCE_DIR} Project)
#    MESSAGE("Last changed log is ${Project_LAST_CHANGED_LOG}")
#endif(Subversion_FOUND)


#FIND_PROGRAM(Subversion_SVN_EXECUTABLE svn DOC "subversion command line client")
#EXECUTE_PROCESS(
#     COMMAND ${Subversion_SVN_EXECUTABLE} --version
#     OUTPUT_VARIABLE VERSION
#     OUTPUT_STRIP_TRAILING_WHITESPACE
#)
FIND_PROGRAM(GIT_EXECUTABLE git DOC "git command line client")

#set (VERSION "")
EXECUTE_PROCESS(
     COMMAND ${GIT_EXECUTABLE} svn info
     COMMAND grep "Revision"
     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
     OUTPUT_VARIABLE VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE
)

set( SVN_VERSION "unknown" )

if (NOT VERSION)
   FIND_PACKAGE(Subversion)

   if (Subversion_FOUND)

   EXECUTE_PROCESS(
      COMMAND svnversion
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
   )
   endif(Subversion_FOUND)

   if (VERSION)
      set( SVN_VERSION ${VERSION} )
   endif(VERSION)
else()
   string(REGEX REPLACE "Revision: " "" SVN_VERSION ${VERSION})
endif(NOT VERSION)

set(NAVIT_VARIANT "-")

message (STATUS "SVN-version ${SVN_VERSION}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
