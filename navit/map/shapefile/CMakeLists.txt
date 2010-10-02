set (map_shapefile_SRC shapefile.c)
if (NOT HAVE_SYSTEM_SHAPEFILELIB)
	set (map_shapefile_SRC ${map_shapefile_SRC} dbfopen.c shpopen.c shptree.c)
endif()

module_add_library(map_shapefile ${map_shapefile_SRC})
