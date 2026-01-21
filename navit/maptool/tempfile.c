/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "maptool.h"
#include "debug.h"

char *tempfile_obtain_prefix() {
    static char *tmpfile_prefix = NULL;

    if (!tmpfile_prefix) {
#define tmpfile_prefix_size 64
        tmpfile_prefix=calloc(tmpfile_prefix_size, 1);

        snprintf(tmpfile_prefix, tmpfile_prefix_size, "maptool_%d.tmp", getpid());
        if (mkdir(tmpfile_prefix, 0755)) {
            perror("Error creating directory");
            exit(1);
        }
    }
    return tmpfile_prefix;
}

void tempfile_cleanup() {
    rmdir(tempfile_obtain_prefix());
}

char *tempfile_name(char *suffix, char *name) {
    return g_strdup_printf("%s/%s_%s.tmp", tempfile_obtain_prefix(), name, suffix);
}
FILE *tempfile(char *suffix, char *name, int mode) {
    char *buffer=tempfile_name(suffix, name);
    FILE *ret=NULL;
    switch (mode) {
    case 0:
        ret=fopen(buffer, "rb");
        break;
    case 1:
        ret=fopen(buffer, "wb+");
        break;
    case 2:
        ret=fopen(buffer, "ab");
        break;
    }
    g_free(buffer);
    return ret;
}

void tempfile_unlink(char *suffix, char *name) {
    char buffer[4096];
    sprintf(buffer,"%s/%s_%s.tmp",tempfile_obtain_prefix(), name, suffix);
    unlink(buffer);
}

void tempfile_rename(char *suffix, char *from, char *to) {
    char buffer_from[4096],buffer_to[4096];
    sprintf(buffer_from,"%s/%s_%s.tmp",tempfile_obtain_prefix(),from,suffix);
    sprintf(buffer_to,"%s/%s_%s.tmp",tempfile_obtain_prefix(),to,suffix);
    dbg_assert(rename(buffer_from, buffer_to) == 0);

}
