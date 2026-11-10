/* Compile with navit/geom.c, the source and build include directories,
 * pkg-config --cflags --libs glib-2.0, and -lm. No copied geometry code. */
#include <assert.h>

#include "geom.h"

int main(void) {
  struct coord diamond[] = {{0, 10}, {10, 0}, {20, 10}, {10, 20}, {0, 10}};
  struct coord triangle[] = {{-2, -1}, {3, 1}, {-2, 2}, {-2, -1}};
  struct coord centre = {10, 10}, outside = {-1, 10}, fractional = {0, 0};
  int winding, closed;
  for (winding = 0; winding < 2; winding++) {
    for (closed = 0; closed < 2; closed++) {
      assert(geom_poly_point_inside(diamond, 4 + closed, &centre));
      assert(!geom_poly_point_inside(diamond, 4 + closed, &outside));
      assert(geom_poly_point_inside(triangle, 3 + closed, &fractional));
    }
    geom_coord_revert(diamond, 5);
    geom_coord_revert(triangle, 4);
  }
  assert(!geom_poly_point_inside(diamond, 0, &centre));
  return 0;
}
