enum projection;
struct attr;
PLUGIN_FUNC1(draw, struct container *, co)
PLUGIN_FUNC3(popup, struct container *, map, struct popup *, p, struct popup_item **, list)
struct navit;
PLUGIN_TYPE(graphics, (struct graphics_methods *meth, struct attr **attrs)) 
PLUGIN_TYPE(gui, (struct navit *nav, struct gui_methods *meth, struct attr **attrs)) 
PLUGIN_TYPE(map, (struct map_methods *meth, struct attr **attrs)) 
PLUGIN_TYPE(osd, (struct navit *nav, struct osd_methods *meth, struct attr **attrs))
PLUGIN_TYPE(speech, (char *data, struct speech_methods *meth)) 
PLUGIN_TYPE(vehicle, (struct vehicle_methods *meth)) 
