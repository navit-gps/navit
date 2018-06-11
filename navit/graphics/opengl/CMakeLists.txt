set(graphics_opengl_SOURCES graphics_opengl.c)
if (USE_OPENGLES)
	LIST(APPEND graphics_opengl_SOURCES graphics_opengl_x11.c graphics_opengl_egl.c)
endif (USE_OPENGLES)
module_add_library(graphics_opengl ${graphics_opengl_SOURCES})

