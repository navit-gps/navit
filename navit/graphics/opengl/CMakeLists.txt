set(graphics_opengl_SOURCES graphics_opengl.c)
if (USE_OPENGLES)
   LIST(APPEND graphics_opengl_SOURCES window.c)
endif (USE_OPENGLES)
module_add_library(graphics_opengl ${graphics_opengl_SOURCES}) 

