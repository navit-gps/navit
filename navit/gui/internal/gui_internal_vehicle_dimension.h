/*
 * gui_internal_vehicle_dimension.h
 *
 *  Created on: 17.04.2021
 *      Author: root
 */

#ifndef NAVIT_GUI_INTERNAL_GUI_INTERNAL_VEHICLE_DIMENSION_H_
#define NAVIT_GUI_INTERNAL_GUI_INTERNAL_VEHICLE_DIMENSION_H_


/* prototypes */
struct gui_priv;
struct widget;
void gui_internal_cmd_change_vehicle_dimensions_weight(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_change_vehicle_dimensions_axle_weight(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_change_vehicle_dimensions_length(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_change_vehicle_dimensions_width(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_change_vehicle_dimensions_height(struct gui_priv *this, struct widget *wm, void *data);
/* end of prototypes */


#endif /* NAVIT_GUI_INTERNAL_GUI_INTERNAL_VEHICLE_DIMENSION_H_ */
