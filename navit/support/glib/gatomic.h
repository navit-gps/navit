//
// Created by jan on 18.02.26.
//

#ifndef NAVIT_GATOMIC_H
#define NAVIT_GATOMIC_H

#include <glib.h>

void g_atomic_int_add(volatile gint *atomic, gint val);
gint g_atomic_int_exchange_and_add(volatile gint *atomic, gint val);

#endif //NAVIT_GATOMIC_H
