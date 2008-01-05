#include "gpx2navit_txt.h"
#include "projects.h"
#include "geodesic.h"

double getDistanceCore(char *p1, char *l1, char *p2, char *l2);
void checkEllpsUnit(char *unit);
double checkLengthUnit(char *unit);
int checkTimeUnit(char *unit);
double getTimeInterval(char *_t, char *t);
double getSpeed(double length, double ti, double to_meter, int to_sec);
double getDistance(double _x, double _y, double x, double y);
// todo void closeShpFiles(shphandles * shps);
// todo void closeDbfFiles(dbfhandles * dbfs);
void *myMallocRep(size_t size, const char *fileName, int line);

void checkEllpsUnit(char *unit)
{
/*
 * checks ellipse unit can be used by proj4
 */
    int isOK = 0;
    struct PJ_ELLPS *el;	/* project.h of proj4 */
    for (el = pj_ellps; el->id; ++el) {
	if (!strcmp(el->id, unit)) {
	    isOK = 1;
	}
    }
    if (!isOK) {
	fputs
	    ("The ellipse argument is not correct or supported by libproj\n",
	     stderr);
	fputs("You can choose the argument from a list below.\n\n",
	      stderr);
	for (el = pj_ellps; el->id; el++) {
	    printf("%10s\t%s\n", el->id, el->name);
	}
	exit(ERR_ELLPSUNIT);
    }
}

double checkLengthUnit(char *unit)
{
/*
 * checks length unit can be used by proj4 
 * then returns unit value to meter
 */
    int isOK = 0;
    double to_meter = 0;
    struct PJ_UNITS *ut;	/* project.h of proj4 */
    for (ut = pj_units; ut->id; ut++) {
	if (!strcmp(ut->id, unit)) {
	    isOK = 1;
	    to_meter = atof(ut->to_meter);
	}
    }
    if (!isOK) {
	fputs
	    ("The length unit argument is not correct or supported by libproj.\n",
	     stderr);
	fputs("You can choose the argument from a list below.\n\n",
	      stderr);
	for (ut = pj_units; ut->id; ut++) {
	    printf("%s\t%s\n", ut->id, ut->name);
	}
	exit(ERR_LENGTHUNIT);
    }
    return to_meter;
}

int checkTimeUnit(char *unit)
{
    char *u[8] = { "sec", "s", "min", "m", "hour", "h", "day", "d" };
    int p[8] = { 1, 1, 60, 60, 3600, 3600, 86400, 86400 };
    int i, to_sec = 0;
    for (i = 0; i < 8; i++) {
	if (!strcmp(u[i], unit)) {
	    to_sec = p[i];
	}
    }
    if (!to_sec) {
	fputs("The time unit argument is not correct.\n", stderr);
	fputs("You can choose the argument from sec, min, hour or day.\n",
	      stderr);
	exit(ERR_TIMEUNIT);
    }
    return to_sec;
}

double getTimeInterval(char *_t, char *t)
{
/*
 * Returns a time interval between _t and t.
 * The arguments should be "YYYY-MM-DDThh:mm:ssZ" (xml schema
 * datetime format without time zone) format.
 */
    double ti;
    struct tm _tt;
    struct tm tt;
    time_t _tmt, tmt;
    memset(&_tt, 0, sizeof(_tt));
    memset(&tt, 0, sizeof(tt));
    sscanf(_t, "%d-%d-%dT%d:%d:%dZ", &_tt.tm_year, &_tt.tm_mon,
	   &_tt.tm_mday, &_tt.tm_hour, &_tt.tm_min, &_tt.tm_sec);
    _tt.tm_year -= 1900;
    _tt.tm_mon -= 1;
    sscanf(t, "%d-%d-%dT%d:%d:%d", &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
	   &tt.tm_hour, &tt.tm_min, &tt.tm_sec);
    tt.tm_year -= 1900;
    tt.tm_mon -= 1;
    _tmt = mktime(&_tt);
    tmt = mktime(&tt);
    ti = difftime(tmt, _tmt);
    return ti;
}

double getSpeed(double length, double ti, double to_meter, int to_sec)
{
/*
 * Culculates speed from length and time.
 */
    double speed;
    if (!length || !ti)
	speed = 0;
    else
	speed = (length / to_meter) / (ti / to_sec);
    return speed;
}

double getDistanceCore(char *p1, char *l1, char *p2, char *l2)
{
    /*
     * Culculates a geodesic length between two points
     * using geod_*.c
     */
    phi1 = dmstor(p1, &p1);
    lam1 = dmstor(l1, &l1);
    phi2 = dmstor(p2, &p2);
    lam2 = dmstor(l2, &l2);
    geod_inv();
    return geod_S;
}

double getDistance(double _x, double _y, double x, double y)
{
    /*
     * Culculates a geodesic length between two points
     */
    double length;
    char p1[17], l1[17], p2[17], l2[17];
    sprintf(p1, "%f", _x);
    sprintf(l1, "%f", _y);
    sprintf(p2, "%f", x);
    sprintf(l2, "%f", y);
    length = getDistanceCore(p1, l1, p2, l2);
    return length;
}

//todo void closeShpFiles(shphandles * shps)
//{
    /*
     * Closes all SHP files if they opened
     */
//    if (shps->wpt)
//	SHPClose(shps->wpt);
//    if (shps->trk)
//	SHPClose(shps->trk);
//    if (shps->trk_edg)
//	SHPClose(shps->trk_edg);
//    if (shps->trk_pnt)
//	SHPClose(shps->trk_pnt);
//    if (shps->rte)
//	SHPClose(shps->rte);
//    if (shps->rte_edg)
//	SHPClose(shps->rte_edg);
//    if (shps->rte_pnt)
//	SHPClose(shps->rte_pnt);
//}

//todo void closeDbfFiles(dbfhandles * dbfs)
//{
    /*
     * Closes all DBF files if they opened
     */
//    if (dbfs->wpt)
//	DBFClose(dbfs->wpt);
//    if (dbfs->trk)
//	DBFClose(dbfs->trk);
//    if (dbfs->trk_edg)
//	DBFClose(dbfs->trk_edg);
//    if (dbfs->trk_pnt)
//	DBFClose(dbfs->trk_pnt);
//    if (dbfs->rte)
//	DBFClose(dbfs->rte);
//    if (dbfs->rte_edg)
//	DBFClose(dbfs->rte_edg);
//    if (dbfs->rte_pnt)
//	DBFClose(dbfs->rte_pnt);
//}

