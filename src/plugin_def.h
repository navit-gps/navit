enum projection;
PLUGIN_FUNC1(draw, struct container *, co)
PLUGIN_FUNC3(popup, struct container *, map, struct popup *, p, struct popup_item **, list)
struct navit;
PLUGIN_TYPE(graphics, (struct graphics_methods *meth)) 
PLUGIN_TYPE(gui, (struct navit *nav, struct gui_methods *meth, int w, int h)) 
PLUGIN_TYPE(map, (struct map_methods *meth, char *data, char **charset, enum projection *pro)) 
PLUGIN_TYPE(osd, (struct osd_methods *meth)) 
PLUGIN_TYPE(speech, (struct speech_methods *meth)) 
PLUGIN_TYPE(vehicle, (struct vehicle_methods *meth)) 
