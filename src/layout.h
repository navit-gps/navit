#ifndef NAVIT_LAYOUT_H
#define NAVIT_LAYOUT_H

#include "item.h"
#include "color.h"

struct element_line;
struct element_text;

struct element {
	enum { element_point, element_polyline, element_polygon, element_circle, element_label, element_icon, element_image } type;
	struct color color;
	int label_size;
	union {
		struct element_point {
		} point;
		struct element_polyline {
			int width;
			int directed;
		} polyline;
		struct element_polygon {
		} polygon;
		struct element_circle {
			int width;
			int radius;
		} circle;
		struct element_icon {
			char *src;
		} icon;
	} u;
};


struct itemtype { 
	int order_min, order_max;
	GList *type;
	GList *elements;
};

struct color;

struct layer { char *name; int details; GList *itemtypes; };

struct layout { char *name; struct color *color; GList *layers; };

/* prototypes */
enum item_type;
struct element;
struct itemtype;
struct layer;
struct layout;
struct layout *layout_new(const char *name, struct color *color);
struct layer *layer_new(const char *name, int details);
void layout_add_layer(struct layout *layout, struct layer *layer);
struct itemtype *itemtype_new(int order_min, int order_max);
void itemtype_add_type(struct itemtype *this, enum item_type type);
void layer_add_itemtype(struct layer *layer, struct itemtype *itemtype);
void itemtype_add_element(struct itemtype *itemtype, struct element *element);
struct element *polygon_new(struct color *color);
struct element *polyline_new(struct color *color, int width, int directed);
struct element *circle_new(struct color *color, int radius, int width, int label_size);
struct element *label_new(int label_size);
struct element *icon_new(const char *src);
struct element *image_new(void);

#endif

