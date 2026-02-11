/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_OSD_H
#define NAVIT_PLUGIN_DRIVER_BREAK_OSD_H

#include "config.h"
#include "navit.h"
#include "command.h"

/* Command functions */
int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_intervals(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_overnight(struct navit *nav, char *function, struct attr **in, struct attr ***out);

/* Register commands */
void driver_break_register_commands(struct navit *nav);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_OSD_H */
