FIND_PROGRAM(GIT_EXECUTABLE NAMES git git.exe DOC "git command line client")

get_filename_component(SOURCE_DIR ${SRC} PATH)

if (GIT_EXECUTABLE)
   EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -n 1 --format=%h
      WORKING_DIRECTORY "${SOURCE_DIR}"
      OUTPUT_VARIABLE VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
   )
else()
   message(STATUS "git not found, cannot record git commit")
endif(GIT_EXECUTABLE)

set(NAVIT_VARIANT "-")
if (DEFINED VERSION)
   set(${NAME} ${VERSION})
else()
   set(${NAME} "---")
endif()

message (STATUS "Git commit: ${${NAME}}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
