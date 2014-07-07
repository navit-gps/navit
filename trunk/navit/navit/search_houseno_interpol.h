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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *
 * Boston, MA  02110-1301, USA.
 */

enum include_end_nodes {
	end_nodes_yes,
	end_nodes_no,
};

/** Data for a house number interpolation. */
struct house_number_interpolation {
        /** Index of interpolation attribute currently used. */
	int curr_interpol_attr_idx;
	/** Interpolation increment */
	int increment;
	/** Reverse interpolation? (0/1) */
	int rev;
	/** First number. */
	char *first;
	/** Last number. */
        char *last;
	/** Include first and last node in interpolation results? */
	enum include_end_nodes include_end_nodes;
	/** Current number in running interpolation. */
        char *curr;
};

void
house_number_interpolation_clear_current(struct house_number_interpolation *inter);

void
house_number_interpolation_clear_all(struct house_number_interpolation *inter);

char *
search_next_interpolated_house_number(struct item *item, struct house_number_interpolation
		*inter, char *inter_match, int inter_partial);

struct pcoord *
search_house_number_coordinate(struct item *item, struct house_number_interpolation *inter);
