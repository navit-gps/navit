/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

struct attr;
struct navit;
struct callback_list;
struct gui;
PLUGIN_FUNC1(draw, struct container *, co)
PLUGIN_FUNC3(popup, struct container *, map, struct popup *, p, struct popup_item **, list)
PLUGIN_CATEGORY(graphics, (struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl))
PLUGIN_CATEGORY(gui, (struct navit *nav, struct gui_methods *meth, struct attr **attrs, struct gui *gui))
PLUGIN_CATEGORY(map, (struct map_methods *meth, struct attr **attrs, struct callback_list *cbl))
PLUGIN_CATEGORY(osd, (struct navit *nav, struct osd_methods *meth, struct attr **attrs))
PLUGIN_CATEGORY(speech, (struct speech_methods *meth, struct attr **attrs, struct attr *parent))
PLUGIN_CATEGORY(vehicle, (struct vehicle_methods *meth, struct callback_list *cbl, struct attr **attrs))
PLUGIN_CATEGORY(event, (struct event_methods *meth))
PLUGIN_CATEGORY(font, (void *meth))
PLUGIN_CATEGORY(traffic, (struct navit *nav, struct traffic_methods *meth, struct attr **attrs, struct callback_list *cbl))
