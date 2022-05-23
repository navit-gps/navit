# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.

add_subdirectory (ezxml)

if (HAVE_API_WIN32_BASE)
	add_subdirectory (glib)
	add_subdirectory (win32)
endif (HAVE_API_WIN32_BASE)

add_subdirectory (zlib)
