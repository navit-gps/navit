#ifndef NAVIT_MAIN_H
#define NAVIT_MAIN_H

/* prototypes */
struct navit;
struct iter;
struct iter * main_iter_new(void);
void main_iter_destroy(struct iter *iter);
struct navit * main_get_navit(struct iter *iter);
void main_add_navit(struct navit *nav);
void main_remove_navit(struct navit *nav);
void print_usage(void);
int main(int argc, char **argv);
/* end of prototypes */

#endif

