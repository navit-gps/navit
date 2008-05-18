#ifndef NAVIT_COMPASS_H
#define NAVIT_COMPASS_H

struct compass * compass_new(struct container *co);
void compass_draw(struct compass *comp, struct container *co);

#endif
