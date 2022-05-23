module_add_library(vehicle_gpsd vehicle_gpsd.c)
if (LIBGPS19_FOUND)
	set_property(
		TARGET vehicle_gpsd
		APPEND PROPERTY COMPILE_DEFINITIONS HAVE_LIBGPS19=1
		)
endif (LIBGPS19_FOUND)

