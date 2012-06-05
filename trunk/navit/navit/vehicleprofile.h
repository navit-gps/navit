/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */


#ifdef __cplusplus
extern "C" {
#endif
struct vehicleprofile {
	int mode;				/**< 0 = Auto, 1 = On-Road, 2 = Off-Road */
	int flags_forward_mask;			/**< Flags mask for moving in positive direction */
	int flags_reverse_mask;			/**< Flags mask for moving in reverse direction */
	int flags;				/**< Required flags to move through a segment */
	int maxspeed_handling;			/**< 0 = Always, 1 = Only if lower, 2 = Never */
	int static_speed;			/**< Maximum speed of vehicle to consider it stationary */
	int static_distance;			/**< Maximum distance of previous position of vehicle to consider it stationary */
	char *name;				/**< the vehicle profile name */
	int width;				/**< Width of the vehicle in cm */
	int height;				/**< Height of the vehicle in cm */
	int length;				/**< Length of the vehicle in cm */
	int weight;				/**< Weight of the vehicle in kg */
	int axle_weight;			/**< Axle Weight of the vehicle in kg */
	int dangerous_goods;			/**< Flags of dangerous goods present */
	int through_traffic_penalty;		/**< Penalty when driving on a through traffic limited road */
	struct attr **attrs;
	GHashTable *roadprofile_hash;
};

struct vehicleprofile * vehicleprofile_new(struct attr *parent, struct attr **attrs);
struct attr_iter *vehicleprofile_attr_iter_new(void);
void vehicleprofile_attr_iter_destroy(struct attr_iter *iter);
int vehicleprofile_get_attr(struct vehicleprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int vehicleprofile_set_attr(struct vehicleprofile *this_, struct attr *attr);
int vehicleprofile_add_attr(struct vehicleprofile *this_, struct attr *attr);
int vehicleprofile_remove_attr(struct vehicleprofile *this_, struct attr *attr);
struct roadprofile * vehicleprofile_get_roadprofile(struct vehicleprofile *this_, enum item_type type);

//! Returns the vehicle profile's name.
char * vehicleprofile_get_name(struct vehicleprofile *this_);
#ifdef __cplusplus
}
#endif
