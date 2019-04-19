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

void initPathAttr(pathattr * pattr, g2sattr * attr);
void setEdge(parsedata * pdata, double _x, double _y, double _z,
             double length, double interval, double speed);
void setPathInterval(parsedata * pdata);
void setPathData(pathattr * pattr, g2sattr * attr);
void countUnconverted(parsedata * pdata);
void countPath(parsedata * pdata);
int checkPath(parsedata * pdata);
void setPath( parsedata * pdata);



/**
 * initialize a path attribute
 */
void initPathAttr(pathattr * pattr, g2sattr * attr) {
    strcpy(pattr->name, attr->name);
    strcpy(pattr->cmt, attr->cmt);
    strcpy(pattr->desc, attr->desc);
    strcpy(pattr->src, attr->src);
    strcpy(pattr->link, attr->link);
    pattr->number = attr->number;
    strcpy(pattr->type, attr->type);
    pattr->length = 0;
    pattr->interval = 0;
    pattr->speed = 0;
    pattr->count = 0;
    pattr->point = NULL;
}


/**
 * set edge data and store it
 */
void setEdge(parsedata * pdata, double _x, double _y, double _z,
             double length, double interval, double speed) {
    pathattr *pattr = pdata->pattr;
    static int isFirstTrkAsEdge = 1;
    static int isFirstRteAsEdge = 1;
    double x[2], y[2], z[2];
    double _length, _interval, _speed;
    if (!strcmp(pdata->current->name, "trkpt")) {
        if (isFirstTrkAsEdge) {
            isFirstTrkAsEdge = 0;
        }
    } else {
        if (isFirstRteAsEdge) {
            isFirstRteAsEdge = 0;
        }
    }
    _length = pattr->length;
    _interval = pattr->interval;
    _speed = pattr->speed;
    pattr->length = length;
    pattr->interval = interval;
    pattr->speed = speed;
    x[0] = _x;
    y[0] = _y;
    z[0] = _z;
    x[1] = pdata->attr->lon;
    y[1] = pdata->attr->lat;
    z[1] = pdata->attr->ele;
    if (pdata->prop->is3d) {
    } else {
    }
    pattr->length = _length;
    pattr->interval = _interval;
    pattr->speed = _speed;
}

/**
 * sets interval data between two track points
 */
void setPathInterval(parsedata * pdata) {
    pathattr *pattr = pdata->pattr;
    g2sattr *attr = pdata->attr;
    g2sprop *prop = pdata->prop;
    static char _t[TIMELENGTH];
    double intvl = 0;
    static double _x, _y, _z;
    double leng = 0;
    double spd;
    if (pattr->count == 1) {
        strcpy(_t, attr->time);
    } else {
        /* time interval */
        intvl = getTimeInterval(_t, attr->time);
        pattr->interval = pattr->interval + intvl;
        strcpy(_t, attr->time);
        /* length interval */
        leng = getDistance(_x, _y, attr->lon, attr->lat);
        pattr->length = pattr->length + leng;
        /* interval speed */
        spd = getSpeed(leng, intvl, prop->speed2meter, prop->speed2sec);
        /* sets edge data */
        if (prop->isEdge) {
            setEdge(pdata, _x, _y, _z, leng, intvl, spd);
        }
    }
    _x = attr->lon;
    _y = attr->lat;
    _z = attr->ele;
}

/**
 * sets each track point data in array.
 */
void setPathData(pathattr * pattr, g2sattr * attr) {
    const int reallocsize = 100;
    if (pattr->count == 0) {
        pattr->point = malloc(sizeof(double) * 3 * reallocsize);
    }
    if ((pattr->count % reallocsize) == 0) {
        pattr->point = realloc(pattr->point,
                               sizeof(double) * 3 * (pattr->count +
                                       reallocsize));
    }
    pattr->point[pattr->count * 3] = attr->lon;
    pattr->point[pattr->count * 3 + 1] = attr->lat;
    pattr->point[pattr->count * 3 + 2] = attr->ele;
    pattr->count++;
}

/**
 * counts paths that wasn't converted
 */
void countUnconverted(parsedata * pdata) {
    g2sstats *stats = pdata->prop->stats;
    if (!strcmp(pdata->current->name, "trkseg"))
        stats->trkunconverted++;
    else
        stats->rteunconverted++;
}

/**
 * counts paths
 */
void countPath(parsedata * pdata) {
    g2sstats *stats = pdata->prop->stats;
    pathattr *pattr = pdata->pattr;
    if (!strcmp(pdata->current->name, "trkseg")) {
        stats->trkcount++;
        stats->trklength += pattr->length;
        stats->trkpoints += pattr->count;
    } else {
        stats->rtecount++;
        stats->rtelength += pattr->length;
        stats->rtepoints += pattr->count;
    }
}

int checkPath(parsedata * pdata) {
    pathattr *pattr = pdata->pattr;
    g2sprop *prop = pdata->prop;
    /* check point count. */
    if (pattr->count < prop->minpoints) {
        fprintf
        (stderr,
         "gpx2navit_txt:%s:%i track was not converted because of less then %d points. \n",
         prop->sourcefile, XML_GetCurrentLineNumber(pdata->parser),
         prop->minpoints);
        countUnconverted(pdata);
        return 0;
        /* check path length */
    } else if (pattr->length < prop->minlength * prop->length2meter) {
        fprintf
        (stderr,
         "gpx2navit_txt:%s:%i track was not converted because it is shorter than %dm.\n",
         prop->sourcefile, XML_GetCurrentLineNumber(pdata->parser),
         prop->minlength);
        countUnconverted(pdata);
        return 0;
        /* check path time */
    } else if (pattr->interval < prop->mintime * prop->time2sec) {
        fprintf
        (stderr,
         "gpx2navit_txt:%s:%i track was not converted because it is shorter than %d sed.\n",
         prop->sourcefile, XML_GetCurrentLineNumber(pdata->parser),
         prop->mintime);
        countUnconverted(pdata);
        return 0;
        /* check path speed */
    } else if (!prop->nospeedcheck && pattr->speed == .0) {
        fprintf
        (stderr,
         "gpx2navit_txt:%s:%i track was not converted because no move recorded. Use --no-speed-check option to bypass this check.\n",
         prop->sourcefile, XML_GetCurrentLineNumber(pdata->parser));
        countUnconverted(pdata);
        return 0;
    }
    return 1;
}

/**
 * saves path data into files.
 */
void setPath( parsedata * pdata) {
    pathattr *pattr = pdata->pattr;
    g2sprop *prop = pdata->prop;
    int isOk = 0;
    pattr->speed =
        getSpeed(pattr->length, pattr->interval, prop->speed2meter,
                 prop->speed2sec);
    if (prop->isFast) {
        isOk = 1;
    } else {
        isOk = checkPath(pdata);
    }
    if (isOk) {
        double x[pattr->count];
        double y[pattr->count];
        double z[pattr->count];
        int i;
        fprintf(pdata->fp,"type=track label=\"%s\" desc=\"%s\" type=\"%s\"\ length=\"%5.3f\" count=\"%5d\"\n"
                ,pdata->pattr->name,pdata->pattr->desc,
                pdata->pattr->type,pdata->pattr->length,
                pdata->pattr->count);

        for (i = 0; i < pattr->count; i++) {
            x[i] = pattr->point[i * 3];
            y[i] = pattr->point[i * 3 + 1];
            z[i] = pattr->point[i * 3 + 2];
            fprintf(pdata->fp,"%3.6f %4.6f\n",x[i],y[i]);
        }
        if (pdata->prop->is3d) {
        } else {
        }
        countPath(pdata);
    }
    free(pattr->point);
}
