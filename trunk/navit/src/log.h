#ifndef NAVIT_LOG_H
#define NAVIT_LOG_H
/* prototypes */
struct attr;
struct log;
struct log *log_new(struct attr **attrs);
void log_set_header(struct log *this_, char *data, int len);
void log_set_trailer(struct log *this_, char *data, int len);
void log_write(struct log *this_, char *data, int len);
void log_destroy(struct log *this_);
/* end of prototypes */
#endif
