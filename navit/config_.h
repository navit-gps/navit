extern struct config *config;
/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct config;
void config_destroy(struct config *this_);
int config_get_attr(struct config *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int config_set_attr(struct config *this_, struct attr *attr);
int config_add_attr(struct config *this_, struct attr *attr);
int config_remove_attr(struct config *this_, struct attr *attr);
struct attr_iter *config_attr_iter_new(void);
void config_attr_iter_destroy(struct attr_iter *iter);
struct config *config_new(struct attr *parent, struct attr **attrs);
/* end of prototypes */
