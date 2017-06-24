FIND_PROGRAM(GIT_EXECUTABLE NAMES git git.exe DOC "git command line client")

get_filename_component(SOURCE_DIR ${SRC} PATH)

string(TIMESTAMP VERSION "%y%m%d%H%M")

string(REGEX MATCH "^[a-z0-9]+" VERSION ${VERSION} )

if (STRIP_M)
   set(VERSION ${VERSION_NUM})
endif()

set(NAVIT_VARIANT "-")

set(${NAME} ${VERSION})

message (STATUS "Git commit: ${${NAME}}")
CONFIGURE_FILE(${SRC} ${DST} @ONLY)
