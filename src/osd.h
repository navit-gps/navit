struct osd_methods {
       void (*osd_destroy)(struct osd_priv *osd);
};

/* prototypes */
struct attr;
struct navit;
struct osd;
struct osd *osd_new(struct navit *nav, const char *type, struct attr **attrs);
/* end of prototypes */
