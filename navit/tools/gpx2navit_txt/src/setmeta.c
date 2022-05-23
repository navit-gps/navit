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

#include "gpx2navit_txt.h"

/**
 * store gpx metadata into text file
 */
void setMetadata(parsedata * pdata) {
    g2sprop *prop = pdata->prop;
    g2sattr *attr = pdata->attr;
    FILE *metafile;
    char *output = malloc(sizeof(char) * (strlen(prop->output) + 10));
    strcpy(output, prop->output);
    strcat(output, "_meta.txt");
    metafile = fopen(output, "w");
    if (metafile == NULL) {
        fprintf(stderr, "Cannot create file: %s\n", output);
        exit(ERR_CREATEFILE);
    }
    if (attr->name) {
        fprintf(metafile, "name\t%s\n", attr->name);
    }
    if (attr->desc) {
        fprintf(metafile, "description\t%s\n", attr->desc);
    }
    if (attr->author) {
        fprintf(metafile, "author\t%s\n", attr->author);
    }
    if (attr->copyright) {
        fprintf(metafile, "copyright\t%s\n", attr->copyright);
    }
    if (attr->link) {
        fprintf(metafile, "link\t%s\n", attr->link);
    }
    if (attr->time) {
        fprintf(metafile, "time\t%s\n", attr->time);
    }
    if (attr->keywords) {
        fprintf(metafile, "keywords\t%s\n", attr->keywords);
    }
    if (attr->minlat) {
        fprintf(metafile, "min latitude\t%f\n", attr->minlat);
    }
    if (attr->minlon) {
        fprintf(metafile, "min longitude\t%f\n", attr->minlon);
    }
    if (attr->maxlat) {
        fprintf(metafile, "max latitude\t%f\n", attr->maxlat);
    }
    if (attr->maxlon) {
        fprintf(metafile, "max longitude\t%f\n", attr->maxlon);
    }
    fclose(metafile);
    free(output);
}
