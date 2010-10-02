add_definitions( -DMODULE=support_zlib )
add_library(support_zlib adler32.c crc32.c infback.c inffast.c inflate.c inftrees.c zutil.c zlib_init.c) 
