module_add_library(vehicle_geoclue vehicle_geoclue.c)
target_link_libraries(vehicle_geoclue PRIVATE ${GeoClue_LIBRARIES})
