set(SUPPORT_GLIB_SRC fake.c galiasdef.c gatomic.c gerror.c ghash.c 
      glib_init.c glist.c gmem.c gmessages.c gprimes.c gprintf.c gslice.c gslist.c
      gstrfuncs.c gstring.c gutf8.c gutils.c)
if(WIN32 OR WINCE)
	list(APPEND SUPPORT_GLIB_SRC gnulib/asnprintf.c gnulib/printf.c gnulib/printf-args.c gnulib/printf-parse.c gnulib/vasnprintf.c)
	set(GLIB_NEED_PRINTF 1)
endif()
supportlib_add_library(support_glib ${SUPPORT_GLIB_SRC}) 
if(GLIB_USE_OWN_PRINTF)
else(GLIB_USE_OWN_PRINTF)
    GET_TARGET_PROPERTY(GLIB_DEFS support_glib COMPILE_DEFINITIONS)
    SET_TARGET_PROPERTIES(support_glib PROPERTIES COMPILE_DEFINITIONS "HAVE_GOOD_PRINTF;${GLIB_DEFS}")
endif(GLIB_USE_OWN_PRINTF)
