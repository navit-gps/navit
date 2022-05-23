/* raster.h -- line/rect/circle/poly rasterization

   copyright (c) 2008 bryan rittmeyer <bryanr@bryanr.org>

   license: LGPLv2
*/

#ifndef __RASTER_H
#define __RASTER_H

#include <stdint.h>
#include "SDL.h"
#include "point.h"

void raster_rect(SDL_Surface *s, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t col);

void raster_line(SDL_Surface *s, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t col);
void raster_circle(SDL_Surface *s, int16_t x, int16_t y, int16_t r, uint32_t col);
void raster_polygon(SDL_Surface *s, int16_t n, int16_t *vx, int16_t *vy, uint32_t col);
void raster_polygon_with_holes (SDL_Surface *s, struct point *p, int count, int hole_count, int* ccount,
                                struct point **holes, uint32_t col);

void raster_aaline(SDL_Surface *s, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t col);
void raster_aacircle(SDL_Surface *s, int16_t x, int16_t y, int16_t r, uint32_t col);
void raster_aapolygon(SDL_Surface *s, int16_t n, int16_t *vx, int16_t *vy, uint32_t col);
void raster_aapolygon_with_holes (SDL_Surface *s, struct point *p, int count, int hole_count, int* ccount,
                                  struct point **holes, uint32_t col);


#endif /* __RASTER_H */
