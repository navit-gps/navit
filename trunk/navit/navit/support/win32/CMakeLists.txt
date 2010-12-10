add_definitions( -DMODULE=support_win32 )
add_library(support_win32 mmap.c win32_init.c ConvertUTF.c serial_io.c addwinsock.c) 

