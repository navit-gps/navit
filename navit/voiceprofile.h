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


struct voiceprofile {
    NAVIT_OBJECT
    int mode;				/**< 0 = Auto, 1 = On-Road, 2 = Off-Road */
    int flags_forward_mask;			/**< Flags mask for moving in positive direction */
    int flags_reverse_mask;			/**< Flags mask for moving in reverse direction */
    int flags;				/**< Required flags to move through a segment */
    int static_speed;			/**< Maximum speed of voice to consider it stationary */
    int static_distance;			/**< Maximum distance of previous position of voice to consider it stationary */
    char *name;				/**< the voice profile name */
    char *route_depth;			/**< the route depth attribute */
    int width;				/**< Width of the voice in cm */
    int height;				/**< Height of the voice in cm */
    int length;				/**< Length of the voice in cm */
    int weight;				/**< Weight of the voice in kg */
    int axle_weight;			/**< Axle Weight of the voice in kg */
    int dangerous_goods;			/**< Flags of dangerous goods present */
    int through_traffic_penalty;		/**< Penalty when driving on a through traffic limited road */
    GHashTable *roadprofile_hash;
    struct attr active_callback;
    int turn_around_penalty;		/**< Penalty when turning around */
    int turn_around_penalty2;		/**< Penalty when turning around, for planned turn arounds */
};

struct voiceprofile * voiceprofile_new(struct attr *parent, struct attr **attrs);
struct attr_iter *voiceprofile_attr_iter_new(void * unused);
void voiceprofile_attr_iter_destroy(struct attr_iter *iter);
int voiceprofile_get_attr(struct voiceprofile *this_, enum attr_type type, struct attr *attr,
                            struct attr_iter *iter);
int voiceprofile_set_attr(struct voiceprofile *this_, struct attr *attr);
int voiceprofile_add_attr(struct voiceprofile *this_, struct attr *attr);
int voiceprofile_remove_attr(struct voiceprofile *this_, struct attr *attr);
struct roadprofile * voiceprofile_get_roadprofile(struct voiceprofile *this_, enum item_type type);

//! Returns the voice profile's name.
char * voiceprofile_get_name(struct voiceprofile *this_);
#ifdef __cplusplus
}
#endif
