FIND_PROGRAM(GIT_EXECUTABLE NAMES git git.exe DOC "git command line client")

get_filename_component(SOURCE_DIR ${SRC} PATH)

string(TIMESTAMP VERSION "%y%m%d%H%M")

string(REGEX MATCH "[0-9]+" VERSION_NUM ${VERSION} ) 
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
