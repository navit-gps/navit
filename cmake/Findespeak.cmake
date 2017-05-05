# - Try to find espeak (with libespeak)
# Once done, this will define
#
#  espeak_FOUND - system has Glib
#  espeak_INCLUDE_DIRS - the Glib include directories
#  espeak_LIBRARIES - link these to use Glib

include(LibFindMacros)

# espeak-related libraries
find_path(espeak_INCLUDE_DIR
  NAMES speak_lib.h
  PATHS /usr /sw/include
  PATH_SUFFIXES espeak
)

# Finally the library itself
find_library(espeak_LIBRARY
  NAMES libespeak.so libespeak.a
  PATHS /sw/lib
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(espeak_PROCESS_INCLUDES espeak_INCLUDE_DIR)
set(espeak_PROCESS_LIBS espeak_LIBRARY)
libfind_process(espeak)


