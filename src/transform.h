#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "point.h"

struct transformation {
        int width;		/* Height of destination rectangle */
        int height;		/* Width of destination rectangle */
        long scale;		/* Scale factor */
	int angle;		/* Rotation angle */
	double cos_val,sin_val;	/* cos and sin of rotation angle */
	struct coord rect[2];	/* Source rectangle */
	struct coord center;	/* Center of source rectangle */
};

int transform(struct transformation *t, struct coord *c, struct point *p);
int is_visible(struct transformation *t, struct coord *c);
int is_line_visible(struct transformation *t, struct coord *c);
int is_too_small(struct transformation *t, struct coord *c, int limit);
void transform_lng_lat(struct coord *c, struct coord_geo *g);
void transform_reverse(struct transformation *t, struct point *p, struct coord *c);
void transform_print_deg(double deg);
double transform_scale(int y);
double transform_distance(struct coord *c1, struct coord *c2);
int transform_distance_sq(struct coord *c1, struct coord *c2);
int transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);

void transform_mercator(double *lng, double *lat, struct coord *c);
int is_point_visible(struct transformation *t, struct coord *c);
int transform_get_scale(struct transformation *t);
void transform_setup_source_rect(struct transformation *t);
void transform_set_angle(struct transformation *t,int angle);
void transform_setup(struct transformation *t, int x, int y, int scale, int angle);
void transform_setup_source_rect_limit(struct transformation *t, struct coord *center, int limit);
void transform_geo_text(struct coord_geo *g, char *buffer);
void transform_limit_extend(struct coord *rect, struct coord *c);
int transform_get_angle(struct coord *c, int dir);

#endif
