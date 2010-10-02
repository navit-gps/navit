add_definitions( -DMODULE=support_glib )
add_library(support_glib STATIC fake.c galiasdef.c gatomic.c gerror.c ghash.c 
glib_init.c glist.c gmem.c gmessages.c gprimes.c gprintf.c gslice.c 
gstrfuncs.c gstring.c gutf8.c gutils.c) 
