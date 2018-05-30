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

void startElementControl(parsedata * pdata, const char *element,
                         const char **attr);
void endElementControl(parsedata * pdata, const char *element);

/*
 * This method controls tag start event.
 * It corrects attributes.
 */
void startElementControl(parsedata * pdata, const char *element,
                         const char **attr) {
    int i;
    static int isFirstTrk = 1;
    static int isFirstRte = 1;
    static int isFirstPathpt = 1;
    for (i = 0; attr[i]; i += 2) {
        if (!strcmp(attr[i], "lon")) {
            pdata->attr->lon = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "lat")) {
            pdata->attr->lat = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "minlon")) {
            pdata->attr->minlon = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "minlat")) {
            pdata->attr->minlat = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "maxlon")) {
            pdata->attr->maxlon = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "maxlat")) {
            pdata->attr->maxlat = atof(attr[i + 1]);
        }
        if (!strcmp(attr[i], "author")) {
            strcpy(pdata->attr->author, attr[i + 1]);
        }
    }
    if (pdata->prop->parseTrk) {
        if (!strcmp(element, "trk")) {
            if (isFirstTrk) {
                isFirstTrk = 0;
            }
        }
        if (!strcmp(element, "trkseg")) {
            isFirstPathpt = 1;
        }
        if (!strcmp(element, "trkpt")) {
            if (isFirstPathpt) {
                initPathAttr(pdata->pattr, pdata->attr);
                isFirstPathpt = 0;
            }
        }
    }
    if (pdata->prop->parseRte) {
        if (!strcmp(element, "rte")) {
            if (isFirstRte) {
                isFirstRte = 0;
                isFirstPathpt = 1;
            }
        }
        if (!strcmp(element, "rtept")) {
            if (isFirstPathpt) {
                initPathAttr(pdata->pattr, pdata->attr);
                isFirstPathpt = 0;
            }
        }
    }
}

/**
 * This method is kicked by tag end event.
 * It corrects char elements when the element tag has some data,
 * then start to convert when tag is top level tag like <wpt>.
 */
void endElementControl(parsedata * pdata, const char *element) {
    static int isFirstWpt = 1;
    static int isFirstTrkAsPoint = 1;
    static int isFirstRteAsPoint = 1;
    /* common elements */
    if (!strcmp(element, "name")) {
        strcpy(pdata->attr->name, pdata->databuf);
    }
    if (!strcmp(element, "cmt")) {
        strcpy(pdata->attr->cmt, pdata->databuf);
    }
    if (!strcmp(element, "desc")) {
        strcpy(pdata->attr->desc, pdata->databuf);
    }
    if (!strcmp(element, "src")) {
        strcpy(pdata->attr->src, pdata->databuf);
    }
    if (!strcmp(element, "link")) {
        strcpy(pdata->attr->link, pdata->databuf);
    }
    if (!strcmp(element, "type")) {
        strcpy(pdata->attr->type, pdata->databuf);
    }
    /* waypoint and metadata elements */
    if (!strcmp(element, "time")) {
        strcpy(pdata->attr->time, pdata->databuf);
    }
    /* route and track point elements */
    if (!strcmp(element, "number")) {
        pdata->attr->number = atoi(pdata->databuf);
    }
    /* waypoint elements */
    if (!strcmp(element, "ele")) {
        pdata->attr->ele = atof(pdata->databuf);
    }
    if (!strcmp(element, "magvar")) {
        pdata->attr->magvar = atof(pdata->databuf);
    }
    if (!strcmp(element, "geoidheight")) {
        pdata->attr->geoidheight = atof(pdata->databuf);
    }
    if (!strcmp(element, "sym")) {
        strcpy(pdata->attr->sym, pdata->databuf);
    }
    if (!strcmp(element, "fix")) {
        strcpy(pdata->attr->fix, pdata->databuf);
    }
    if (!strcmp(element, "sat")) {
        pdata->attr->sat = atoi(pdata->databuf);
    }
    if (!strcmp(element, "hdop")) {
        pdata->attr->hdop = atof(pdata->databuf);
    }
    if (!strcmp(element, "vdop")) {
        pdata->attr->vdop = atof(pdata->databuf);
    }
    if (!strcmp(element, "pdop")) {
        pdata->attr->pdop = atof(pdata->databuf);
    }
    if (!strcmp(element, "ageofdgpsdata")) {
        pdata->attr->ageofdgpsdata = atof(pdata->databuf);
    }
    /* metadata elements */
    if (!strcmp(element, "author")) {
        strcpy(pdata->attr->author, pdata->databuf);
    }
    if (!strcmp(element, "keywords")) {
        strcpy(pdata->attr->keywords, pdata->databuf);
    }
    if (!strcmp(element, "copyright")) {
        strcpy(pdata->attr->copyright, pdata->databuf);
    }
    if (!strcmp(element, "year")) {
        pdata->attr->year = atoi(pdata->databuf);
    }
    if (!strcmp(element, "license")) {
        strcpy(pdata->attr->license, pdata->databuf);
    }
    if (!strcmp(element, "bounds")) {
        /* none */
    }
    /* top elements */
    /* set waypoint data */
    if (!strcmp(element, "wpt")) {
        if (pdata->prop->parseWpt) {
            if (isFirstWpt) {
                isFirstWpt = 0;
            }
            //todo
            if (DEBUG) {
                fprintf(stderr,"\neectrl wpt %s %s",
                        pdata->attr->desc,pdata->attr->name);
            }
            setWpt(pdata);
            wipeAttr(pdata->attr);
        }
    }
    /* set trackpoint data */
    if (!strcmp(element, "trkpt")) {
        if (pdata->prop->parseTrk) {
            setPathData(pdata->pattr, pdata->attr);
            if (!pdata->prop->isFast)
                setPathInterval(pdata);
        }
        /* set trackpoint data as point */
        if (pdata->prop->isPoint) {
            if (isFirstTrkAsPoint) {
                isFirstTrkAsPoint = 0;
            }
            setWpt(pdata);
        }
        wipeAttr(pdata->attr);
    }
    /* write trackpoint */
    if (!strcmp(element, "trkseg")) {
        if (pdata->prop->parseTrk) {
            setPath( pdata);
        }
    }
    /* set route data */
    if (!strcmp(element, "rtept")) {
        if (pdata->prop->parseRte) {
            setPathData(pdata->pattr, pdata->attr);
            if (!pdata->prop->isFast)
                setPathInterval(pdata);
        }
        /* set route data as point */
        if (pdata->prop->isPoint) {
            if (isFirstRteAsPoint) {
                isFirstRteAsPoint = 0;
            }
            setWpt( pdata);
        }
        wipeAttr(pdata->attr);
    }
    /* write route */
    if (!strcmp(element, "rte")) {
        if (pdata->prop->parseRte) {
            setPath( pdata);
        }
    }
    if (!strcmp(element, "metadata")) {
        setMetadata(pdata);
        wipeAttr(pdata->attr);
    }
    pdata->bufptr = NULL; //reset bufptr now
}
