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

#include <stdio.h>
#include "attr.h"
#include "coord.h"

#define TEXTFILE_COMMENT_CHAR '#'
#define TEXTFILE_COMMENT_NAVI_STOPPED "# navigation stopped\n"

struct map_priv {
	int id;
	char *filename;
	char *charset;
	int is_pipe;
	int flags;
};

#define TEXTFILE_LINE_SIZE 512

struct map_rect_priv {
	struct map_selection *sel;

	FILE *f;
	long pos;
	char line[TEXTFILE_LINE_SIZE];
	int attr_pos;
	enum attr_type attr_last;
	char attrs[TEXTFILE_LINE_SIZE];
	char attr[TEXTFILE_LINE_SIZE];
	char attr_name[TEXTFILE_LINE_SIZE];
	struct coord c;
	int eoc;
	int more;
	struct map_priv *m;
	struct item item;
	char *args;
	int lastlen;
};

