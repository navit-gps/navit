add_library(fib fib.c)
if (USE_PLUGINS)
   SET_TARGET_PROPERTIES( fib PROPERTIES COMPILE_FLAGS -fPIC)
endif()

