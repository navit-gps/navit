if(HAVE_GTK2 AND NOT MSVC)
	#binfile for WIN32 needs -mno-ms-bitfields
	remove_definitions("-mms-bitfields")
	add_definitions("-mno-ms-bitfields")
endif(HAVE_GTK2 AND NOT MSVC)
module_add_library(map_binfile binfile.c)
