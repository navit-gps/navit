
#ifndef OBJ_FILTER
#define OBJ_FILTER

//forward declaration
struct attr;

struct obj_filter_t {
  char iterator_type[64];
  char filter_expr[256];
  int idx;
};

struct obj_list {
  struct navit_object* objs;
  int size;
};

int parse_obj_filter(const char*input, struct obj_filter_t* out);
struct attr* filter_object(struct attr* root,char*iter_type,char*expr, int idx);

#endif

