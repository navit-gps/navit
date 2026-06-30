/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_OSD_LIFECYCLE_H
#define NAVIT_PLUGIN_DRIVER_BREAK_OSD_LIFECYCLE_H

#include "attr.h"
#include "navit.h"
#include "osd.h"

struct driver_break_priv;

struct osd_priv *driver_break_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs);
void driver_break_ecu_sync_to_priv(struct driver_break_priv *priv);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_OSD_LIFECYCLE_H */
