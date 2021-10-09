set(graphics_gtk_drawing_area_LIBS ${IMLIB2_LDFLAGS} ${GTK2_LIBRARIES})
if(BUILD_BUNDLE)
    add_compile_options(-x objective-c)
endif(BUILD_BUNDLE)
module_add_library(graphics_gtk_drawing_area graphics_gtk_drawing_area.c)

