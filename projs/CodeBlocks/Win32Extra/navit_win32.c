

extern void module_data_textfile_init(void);
extern void module_data_binfile_init(void);
extern void module_data_mg_init(void);
extern void module_win32_gui_init(void);
extern void module_data_garmin_init(void);
extern void module_data_poi_geodownload_init(void);
extern void module_vehicle_demo_init(void);
extern void module_vehicle_file_init(void);
extern void module_speech_speech_dispatcher_init(void);

void builtin_init(void) {
	module_data_textfile_init();
	module_data_binfile_init();
	module_data_mg_init();
	module_win32_gui_init();
    module_data_garmin_init();
	module_data_poi_geodownload_init();
	module_vehicle_demo_init();
	module_vehicle_file_init();
	module_speech_speech_dispatcher_init();
}
