#ifndef NAVIT_LOG_H
#define NAVIT_LOG_H
/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct log;
int log_get_attr(struct log *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct log *log_new(struct attr **attrs);
void log_set_header(struct log *this_, char *data, int len);
void log_set_trailer(struct log *this_, char *data, int len);
void log_write(struct log *this_, char *data, int len);
void log_destroy(struct log *this_);
/* end of prototypes */
#endif
