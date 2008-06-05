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

#ifndef GPX2SHP_H_INCLUDED
#define GPX2SHP_H_INCLUDED

#define PROG gpx2navit_txt
#define FILENAMELENGTH 255	/* 255 is max length for dbf string column */
#define COMMENTLENGTH 255	/* 255 is max length for dbf string column */
#define NAMELENGTH 32
#define TIMELENGTH 32
#define TYPELENGTH 16
#define BUFFSIZE 8192
#define DATABUFSIZE 16
#define failToWriteAttr(S, T) failToWriteAttrRep((S), (T),__FILE__, __LINE__ )

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>
#include <expat.h>
#include "errorcode.h"
//#include "shapefil.h"


#define DEBUG 0
/**
 * make xml parent and child list
 */
typedef struct parent {
    char *name;		/** element name */
    struct parent *parentptr;
    			/** parent pointer */
} parent;

/**
 * set attribute columns on/off
 */
typedef struct g2scolumns {
/**
 * each member corresponds to attribute column of attribute table
 */
    int name;
    int cmt;
    int desc;
    int src;
    int link;
    int type;
    int time;
    int number;
    int ele;
    int magvar;
    int geoidheight;
    int sym;
    int fix;
    int sat;
    int hdop;
    int vdop;
    int pdop;
    int ageofdgpsdata;
    int dgpsid;
    int length;
    int interval;
    int speed;
    int points;
    int gpxline;
} g2scolumns;

/**
 * store each path attribute values for trackpoint and route.
 */
typedef struct pathattr {
    char name[NAMELENGTH];
    char cmt[COMMENTLENGTH];
    char desc[COMMENTLENGTH];
    char src[COMMENTLENGTH];
    char link[FILENAMELENGTH];
    int number;
    char type[TYPELENGTH];
    double length;
    double interval;
    double speed;
    /*
    double *x;
    double *y;
    double *z;
    */
    double *point;
    int count;
} pathattr;

/**
 * store each point attribute values.
 */
typedef struct g2sattr {
/**
 * the structure cames from GPX1.1 format
 */
    double lon;
    double lat;
    double minlon;
    double minlat;
    double maxlon;
    double maxlat;
    char name[NAMELENGTH];
    char cmt[COMMENTLENGTH];
    char desc[COMMENTLENGTH];
    char src[COMMENTLENGTH];
    char link[FILENAMELENGTH];
    char type[TYPELENGTH];
    char time[TIMELENGTH];
    int number;
    double ele;
    double magvar;
    double geoidheight;
    char sym[NAMELENGTH];
    char fix[NAMELENGTH];
    int sat;
    double hdop;
    double vdop;
    double pdop;
    double ageofdgpsdata;
    int dgpsid;
    char author[NAMELENGTH];
    char keywords[NAMELENGTH];
    char copyright[NAMELENGTH];
    int year;
    char license[NAMELENGTH];
} g2sattr;

/**
 * statistics structure 
 */
typedef struct g2sstats {
	int trkpoints;	/** track point total count */
	int trkcount;	/** track path total count */
    double trklength;	/** track total length */
    int rtepoints;	/** route point total count */
    int rtecount;	/** route path totol count */
    double rtelength;	/** route total length */
    int wptpoints;	/** way point total count */
    int trkunconverted;	/** unconverted track path count */
    int rteunconverted;	/** unconverted route path count */ 
} g2sstats;

/**
 * cluster of all dbfhandles
 */ 
//typedef struct dbfhandles {
//    DBFHandle trk;	/** for track */
//    DBFHandle wpt;	/** for waypoint */
//    DBFHandle rte;	/** for route */
//    DBFHandle trk_edg;	/** for track each edge */
//    DBFHandle trk_pnt;	/** for track each point */
//    DBFHandle rte_edg;	/** for route each edge */
//    DBFHandle rte_pnt;	/** for route each point */
//} dbfhandles;

/**
 * cluster of all shphandles
 */
//typedef struct shphandles {
//    SHPHandle trk;	/** for track */
//    SHPHandle wpt;	/** for waypoint */
//    SHPHandle rte;	/** for route */
//    SHPHandle trk_edg;	/** for track each edge */
//    SHPHandle trk_pnt;	/** for track each point */
//    SHPHandle rte_edg;	/** for route each edge */
//    SHPHandle rte_pnt;	/** for route each point */
//} shphandles;

/**
 * propaties structure for gpx2navit_txt
 */
typedef struct g2sprop {
    int parseWpt;	/** convert waypoint data or not */
    int parseTrk;	/** convert track data or not */
    int parseRte;	/** convert route data or not */
    int is3d;		/** using 3D mode */
    int isEdge;		/** convert path data as each separated path	*/
    int isPoint;	/** convert path data as point */
    int isFast;		/** fast mode that skips path check */
    int needsStats;	/** shows statistics at last */
    int minpoints;	/** minimum points to convert as a path */
    int minlength;	/** minimum length to convert as a path */
    int mintime;	/** minimum time to convert as a path */
    int verbose;	/** verbose mode on/off */
    char *sourcefile;	/** source .gpx file */
    char *output;	/** output file base name */
    char *ellipsoid;	/** ellipsoid type to calculate length */
    char *lengthUnit;	/** length unit for attributes*/
    double length2meter;/** meter value of lenght unit */
    char *timeUnit;	/** time unit for attributes */
    double time2sec;	/** value to convert time unit to second */
    char *speedLengthUnit;
    			/** lenght unit to calculate speed*/
    double speed2meter; /** meter value of speedLengthUnit */
    char *speedTimeUnit;/** time unit to calculate speed */
    int speed2sec;	/** value to convert speedTimeUnit to seconde */
    g2sstats *stats;	/** convert statistics */
    g2scolumns *cols;	/** attribute table column switch */
} g2sprop;

/**
 * userdata structure between expat methods 
 */
typedef struct parsedata {
    int depth;		/** xml path depth */
    char *databuf;	/** character buffer in tags */
    char *bufptr;	/** pointer to databuf to add '\0' to databuf */
    int failed;		/** xml parse failed flag */
    int failedid;	/** xml parse failed id */
    XML_Parser parser;	/** xml parser itself*/
    parent *parent;	/** pointer to parent node */
    parent *current;	/** pointer to current node */
    FILE *fp;           /** File handle to write out data points*/
//    shphandles *shps;	/** .shp file cluster that is used in this program */
//    dbfhandles *dbfs;	/** .dbf file cluster that is used in this program */
    g2sattr *attr;	/** each point attributes */
    pathattr *pattr;	/** each path attributes */
    g2sprop *prop;	/** propaties for this program */
} parsedata;

/* utils.c */
void checkEllpsUnit(char *unit);
double checkLengthUnit(char *unit);
int checkTimeUnit(char *unit);
double getTimeInterval(char *_t, char *t);
double getSpeed(double length, double ti, double to_meter, int to_sec);
double getDistance(double _x, double _y, double x, double y);
//void closeShpFiles(shphandles * shps);
//void closeDbfFiles(dbfhandles * dbfs);
void *myMallocRep(size_t size, const char *fileName, int line);

/* misc.c */
void failToWriteAttrRep(int iShape, int col, char *file, int line);
void showStats(g2sprop * prop);
void wipePathAttr(pathattr * pattr);
pathattr *createPathAttr(void);
void wipeAttr(g2sattr * attr);
void setColsDefault(g2scolumns * cols);
g2scolumns *createCols(void);
g2sattr *createAttr(void);
g2sprop *createProp(void);
void closeProp(g2sprop * prop);
//shphandles *createShps(void);
//dbfhandles *createDbfs(void);
parsedata *createParsedata(XML_Parser parser, g2sprop * prop);
void closeParsedata(parsedata * pdata);

/* parser.c */
void parseMain(g2sprop * pr);

/* elementControl.c */
void startElementControl(parsedata * pdata, const char *element,
			 const char **attr);
void endElementControl(parsedata * pdata, const char *element);

/* setwpt.c */
void setWpt( parsedata * pdata);

/* setpath.c */
void initPathAttr(pathattr * pattr, g2sattr * attr);
void setPathInterval(parsedata *pdata);
void setPathData(pathattr * parrt, g2sattr * attr);
void setPath( parsedata * pdata);

/* setmetadata.c */
void setMetadata(parsedata * pdata);

#endif
