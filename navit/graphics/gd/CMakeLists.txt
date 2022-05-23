include_directories(${GD_INCLUDE_DIR})
module_add_library(graphics_gd graphics_gd.c)
target_link_libraries(graphics_gd ${GD_LIBS})
