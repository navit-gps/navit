#ifdef __cplusplus
extern "C" {
#endif
extern int main_argc;
extern char * const* main_argv;
extern struct config *config;
extern int configured;
extern int config_empty_ok;
/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct config;
struct config * config_get(void);
void config_destroy(struct config *this_);
int config_get_attr(struct config *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int config_set_attr(struct config *this_, struct attr *attr);
int config_add_attr(struct config *this_, struct attr *attr);
int config_remove_attr(struct config *this_, struct attr *attr);
struct attr_iter *config_attr_iter_new(void * unused);
void config_attr_iter_destroy(struct attr_iter *iter);
struct config *config_new(struct attr *parent, struct attr **attrs);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
