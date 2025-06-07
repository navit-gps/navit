#ifndef NAVIT_ATTR_TYPE_DEFH
#define NAVIT_ATTR_TYPE_DEFH

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attribute type values, created using macro magic.
 */
enum attr_type {
#define ATTR2(x,y) attr_##y=x,
#define ATTR(x) attr_##x,

    /* Special macro for unused attribute types. Creates a placeholder entry
     * in the enum so the following values do not change. */
#define ATTR_UNUSED ATTR_UNUSED_L(__LINE__)
#define ATTR_UNUSED_L(x) ATTR_UNUSED_WITH_LINE_NUMBER(x)
#define ATTR_UNUSED_WITH_LINE_NUMBER(x) ATTR_UNUSED_##x,

#include "attr_def.h"

#undef ATTR_UNUSED_WITH_LINE_NUMBER
#undef ATTR_UNUSED_L
#undef ATTR_UNUSED

#undef ATTR2
#undef ATTR
};

#ifdef __cplusplus
}
/* __cplusplus */
#endif

/* NAVIT_ATTR_TYPE_DEFH */
#endif
