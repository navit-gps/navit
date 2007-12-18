#ifndef NAVIT_VEHICLE_H
#define NAVIT_VEHICLE_H

#ifdef __cplusplus
extern "C" {
#endif
struct vehicle_priv;

struct vehicle_methods {
	void (*destroy)(struct vehicle_priv *priv);
	int (*position_attr_get)(struct vehicle_priv *priv, enum attr_type type, struct attr *attr);
	int (*set_attr)(struct vehicle_priv *priv, struct attr *attr, struct attr **attrs);

};

/* prototypes */
enum attr_type;
struct attr;
struct vehicle;
struct vehicle *vehicle_new(struct attr **attrs);
int vehicle_position_attr_get(struct vehicle *this_, enum attr_type type, struct attr *attr);
int vehicle_set_attr(struct vehicle *this_, struct attr *attr, struct attr **attrs);
int vehicle_add_attr(struct vehicle *this_, struct attr *attr, struct attr **attrs);
int vehicle_remove_attr(struct vehicle *this_, struct attr *attr);
void vehicle_destroy(struct vehicle *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

