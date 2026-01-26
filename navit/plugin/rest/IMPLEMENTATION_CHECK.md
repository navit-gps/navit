# Rest Plugin Implementation Check

## Summary
All declared functions in plugin headers have been verified to have implementations.

## Functions Checked

### rest_poi_hiking.h
- `poi_search_water_points` - IMPLEMENTED
- `poi_search_water_points_map` - IMPLEMENTED (added)
- `poi_search_cabins` - IMPLEMENTED
- `poi_search_cabins_map` - IMPLEMENTED
- `poi_free_water_points` - IMPLEMENTED
- `poi_free_cabins` - IMPLEMENTED
- `poi_free_water_point` - IMPLEMENTED
- `poi_free_cabin` - IMPLEMENTED

### rest_poi_map.h
- `rest_poi_map_search` - IMPLEMENTED
- `rest_poi_map_search_water` - IMPLEMENTED
- `rest_poi_map_search_cabins` - IMPLEMENTED
- `rest_poi_map_search_car_pois` - IMPLEMENTED
- `rest_poi_is_network_cabin` - IMPLEMENTED
- `rest_route_is_pilgrimage_hiking` - IMPLEMENTED

### rest_poi.h
- `rest_poi_discover` - IMPLEMENTED
- `rest_poi_rank` - IMPLEMENTED
- `rest_poi_free_list` - IMPLEMENTED
- `rest_poi_free` - IMPLEMENTED

### rest_srtm.h
- `srtm_init` - IMPLEMENTED
- `srtm_get_data_directory` - IMPLEMENTED
- `srtm_get_elevation` - IMPLEMENTED
- `srtm_calculate_tiles` - IMPLEMENTED
- `srtm_get_regions` - IMPLEMENTED
- `srtm_get_region` - IMPLEMENTED
- `srtm_download_region` - IMPLEMENTED
- `srtm_download_pause` - IMPLEMENTED
- `srtm_download_resume` - IMPLEMENTED
- `srtm_download_cancel` - IMPLEMENTED
- `srtm_tile_exists` - IMPLEMENTED
- `srtm_get_tile_filename` - IMPLEMENTED
- `srtm_free_regions` - IMPLEMENTED
- `srtm_free_tiles` - IMPLEMENTED
- `srtm_free_download_context` - IMPLEMENTED

### rest_srtm_osd.h
- `rest_srtm_register_commands` - IMPLEMENTED

### rest_energy.h
- `energy_model_init` - IMPLEMENTED
- `energy_calculate_segment` - IMPLEMENTED
- `energy_calculate_gradient` - IMPLEMENTED
- `energy_get_effective_speed` - IMPLEMENTED

### rest_hiking.h
- `hiking_calculate_rest_stops` - IMPLEMENTED
- `hiking_calculate_rest_stops_with_max` - IMPLEMENTED
- `hiking_calculate_daily_segments` - IMPLEMENTED
- `hiking_free_rest_stops` - IMPLEMENTED
- `hiking_free_daily_segments` - IMPLEMENTED

### rest_cycling.h
- `cycling_calculate_rest_stops` - IMPLEMENTED
- `cycling_calculate_daily_segments` - IMPLEMENTED
- `cycling_free_rest_stops` - IMPLEMENTED
- `cycling_free_daily_segments` - IMPLEMENTED

### rest_finder.h
- `rest_finder_find_along_route` - IMPLEMENTED
- `rest_finder_find_near` - IMPLEMENTED
- `rest_finder_is_valid_location` - IMPLEMENTED
- `rest_finder_is_valid_nightly_location` - IMPLEMENTED
- `rest_finder_free_list` - IMPLEMENTED
- `rest_finder_free` - IMPLEMENTED

### rest_glacier.h
- `glacier_is_too_close_for_camping` - IMPLEMENTED
- `glacier_find_nearby` - IMPLEMENTED
- `glacier_free_list` - IMPLEMENTED
- `glacier_get_min_camping_distance` - IMPLEMENTED

### rest_route_validator.h
- `route_validator_validate_hiking` - IMPLEMENTED
- `route_validator_validate_hiking_with_priority` - IMPLEMENTED
- `route_validator_is_forbidden_highway` - IMPLEMENTED
- `route_validator_is_priority_path` - IMPLEMENTED
- `route_validator_free_result` - IMPLEMENTED

### rest_db.h
- `rest_db_new` - IMPLEMENTED
- `rest_db_destroy` - IMPLEMENTED
- `rest_db_add_history` - IMPLEMENTED
- `rest_db_get_history` - IMPLEMENTED
- `rest_db_clear_history` - IMPLEMENTED
- `rest_db_save_config` - IMPLEMENTED
- `rest_db_load_config` - IMPLEMENTED

### rest_osd.h
- `rest_cmd_suggest_stop` - IMPLEMENTED
- `rest_cmd_show_history` - IMPLEMENTED
- `rest_cmd_configure` - IMPLEMENTED
- `rest_cmd_start_break` - IMPLEMENTED
- `rest_cmd_end_break` - IMPLEMENTED
- `rest_cmd_configure_intervals` - IMPLEMENTED
- `rest_cmd_configure_overnight` - IMPLEMENTED
- `rest_register_commands` - IMPLEMENTED

## Fixes Applied

1. **Added missing `poi_search_water_points_map` implementation** - This function was declared in `rest_poi_hiking.h` but was not implemented. Added implementation that wraps `rest_poi_map_search_water`.

2. **Fixed `srtm_download_tile` forward declaration** - Moved forward declarations inside `#ifdef HAVE_CURL` block to match where the functions are defined.

3. **Fixed `srtm_download_progress_callback` placement** - Moved function definition inside `#ifdef HAVE_CURL` block since it uses `srtm_download_tile`.

## Build Status
- Plugin builds successfully with no errors
- No undefined symbol warnings for plugin functions
- All declared functions have implementations
