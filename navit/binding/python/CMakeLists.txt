include_directories(${PYTHON_INCLUDE_DIRS})
module_add_library(binding_python attr.c binding_python.c config.c navigation.c navit.c pcoord.c route.c)
