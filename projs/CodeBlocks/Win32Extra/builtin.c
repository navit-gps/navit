extern void module_data_binfile_init(void);
extern void module_data_poi_geodownload_init(void);
extern void module_gtk_graphics_init(void);
extern void module_gtk_gui_init(void);
extern void module_vehicle_demo_init(void);
extern void module_vehicle_file_init(void);
extern void builtin_init(void);
void builtin_init(void) {
	module_data_binfile_init();
	module_data_poi_geodownload_init();
	module_gtk_graphics_init();
	module_gtk_gui_init();
	module_vehicle_demo_init();
	module_vehicle_file_init();
}
