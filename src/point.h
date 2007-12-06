#ifndef NAVIT_POINT_H
#define NAVIT_POINT_H

struct point {
	int x;
	int y;
};

struct point_rect {
	struct point lu;
	struct point rl;
};
#endif
