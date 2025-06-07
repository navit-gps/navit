#ifndef NAVIT_ITEM_TYPE_DEFH
#define NAVIT_ITEM_TYPE_DEFH

#ifdef __cplusplus
extern "C" {
#endif

enum item_type {
#define ITEM2(x,y) type_##y=x,
#define ITEM(x) type_##x,
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

#ifdef __cplusplus
}
/* __cplusplus */
#endif

/* NAVIT_ITEM_TYPE_DEFH */
#endif
