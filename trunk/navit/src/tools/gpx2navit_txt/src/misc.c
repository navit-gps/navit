#include "gpx2navit_txt.h"

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

/**
 * message when fail to write attribute
 */
void failToWriteAttrRep(int iShape, int col, char *file, int line)
{
    printf("Fail to write a attribute at %s:%i. shapeid:%i col:%i\n", file,
	   line, iShape, col);
}

/**
 * shows short statistics
 */
void showStats(g2sprop * prop)
{
    g2sstats *stats = prop->stats;
    double ratio;
    if (prop->needsStats) {
	if (prop->parseTrk) {
	    if (stats->trkunconverted != 0) {
		ratio =
		    (double) stats->trkunconverted / (stats->trkcount +
						      stats->
						      trkunconverted) *
		    100;
	    } else {
		ratio = 0;
	    }
	    printf("Track Points:\n");
	    printf("\ttrack count:\t%i\n", stats->trkcount);
	    printf("\tpoint count:\t%i\n", stats->trkpoints);
	    if (!prop->isFast) {
		printf("\ttotal length:\t%f\n", stats->trklength);
		printf("\tunconverted:\t%i(%5.2f%%)\n",
		       stats->trkunconverted, ratio);
	    }
	}
	if (prop->parseRte) {
	    if (stats->rteunconverted != 0) {
		ratio =
		    (double) stats->rteunconverted / (stats->rtecount +
						      stats->
						      rteunconverted) *
		    100;
	    } else {
		ratio = 0;
	    }
	    printf("Routes:\n");
	    printf("\troute count:\t%i\n", stats->rtecount);
	    printf("\tpoint count:\t%i\n", stats->rtepoints);
	    if (!prop->isFast) {
		printf("\ttotal length:\t%f\n", stats->rtelength);
		printf("\tunconverted:\t%i(%5.2f%%)\n",
		       stats->rteunconverted, ratio);
	    }
	}
	if (prop->parseWpt) {
	    printf("Waypoints:\n");
	    printf("\tpoint count:\t%i\n", stats->wptpoints);
	}
    }
}

/**
 * clears a path attribute structure 
 */
void wipePathAttr(pathattr * pattr)
{
    pattr->name[0] = '\0';
    pattr->cmt[0] = '\0';
    pattr->desc[0] = '\0';
    pattr->src[0] = '\0';
    pattr->link[0] = '\0';
    pattr->number = 0;
    pattr->type[0] = '\0';
    pattr->length = 0;
    pattr->interval = 0;
    pattr->speed = 0;
    //pattr->point = NULL;
    pattr->count = 0;
}

/**
 * creates a new path attribute 
 */
pathattr *createPathAttr(void)
{
    pathattr *pattr;
    pattr = (pathattr *) malloc(sizeof(pathattr));
    wipePathAttr(pattr);
    return pattr;
}

/**
 * clears a element attribute structure 
 */
void wipeAttr(g2sattr * attr)
{
    attr->lon = 0;
    attr->lat = 0;
    attr->minlon = 0;
    attr->minlat = 0;
    attr->maxlon = 0;
    attr->maxlat = 0;
    attr->name[0] = '\0';
    attr->cmt[0] = '\0';
    attr->desc[0] = '\0';
    attr->src[0] = '\0';
    attr->link[0] = '\0';
    attr->type[0] = '\0';
    attr->time[0] = '\0';
    attr->number = 0;
    attr->ele = 0;
    attr->magvar = 0;
    attr->geoidheight = 0;
    attr->sym[0] = '\0';
    attr->fix[0] = '\0';
    attr->sat = 0;
    attr->hdop = 0;
    attr->vdop = 0;
    attr->pdop = 0;
    attr->ageofdgpsdata = 0;
    attr->dgpsid = 0;
    attr->author[0] = '\0';
    attr->keywords[0] = '\0';
    attr->copyright[0] = '\0';
    attr->year = 0;
    attr->license[0] = '\0';
    attr->minlat = 0;
    attr->minlon = 0;
    attr->maxlat = 0;
    attr->maxlon = 0;
}

/**
 * sets default values to a column properties.
 */
void setColsDefault(g2scolumns * cols)
{
    cols->name = 1;
    cols->cmt = 1;
    cols->desc = 1;
    cols->src = 1;
    cols->link = 1;
    cols->type = 1;
    cols->time = 1;
    cols->number = 1;
    cols->ele = 1;
    cols->magvar = 1;
    cols->geoidheight = 1;
    cols->sym = 1;
    cols->fix = 1;
    cols->sat = 1;
    cols->hdop = 1;
    cols->vdop = 1;
    cols->pdop = 1;
    cols->ageofdgpsdata = 1;
    cols->dgpsid = 1;
    cols->length = 1;
    cols->interval = 1;
    cols->speed = 1;
    cols->points = 1;
    cols->gpxline = 0;
}

/**
 * creates a column structure
 */
g2scolumns *createCols(void)
{
    g2scolumns *cols;
    cols = (g2scolumns *) malloc(sizeof(g2scolumns));
    setColsDefault(cols);
    return cols;
}

/**
 * creates a element attribute structure.
 */
g2sattr *createAttr(void)
{
    g2sattr *attr;
    attr = (g2sattr *) malloc(sizeof(g2sattr));
    wipeAttr(attr);
    return attr;
}

/**
 * creates a properties structure for gpx2shp
 */
g2sprop *createProp(void)
{
    g2sprop *prop;
    g2sstats *stats;
    g2scolumns *cols;
    prop = malloc(sizeof(g2sprop));
    stats = malloc(sizeof(g2sstats));
    cols = createCols();
    prop->stats = stats;
    prop->parseWpt = 0;
    prop->parseTrk = 0;
    prop->parseRte = 0;
    prop->minpoints = 2;
    prop->minlength = 0;
    prop->mintime = 0;
    prop->is3d = 0;
    prop->isEdge = 0;
    prop->isPoint = 0;
    prop->isFast = 0;
    prop->needsStats = 0;
    prop->verbose = 0;
    prop->output = NULL;
    prop->ellipsoid = NULL;
    prop->lengthUnit = NULL;
    prop->speedLengthUnit = NULL;
    prop->speedTimeUnit = NULL;
    prop->timeUnit = NULL;
    prop->stats->trkcount = 0;
    prop->stats->trkpoints = 0;
    prop->stats->trklength = 0;
    prop->stats->trkunconverted = 0;
    prop->stats->rtecount = 0;
    prop->stats->rtepoints = 0;
    prop->stats->rtelength = 0;
    prop->stats->rteunconverted = 0;
    prop->stats->wptpoints = 0;
    prop->cols = cols;
    return prop;
}

/**
 * close and free a propertires structure
 */
void closeProp(g2sprop * prop)
{
    free(prop->stats);
    free(prop->sourcefile);
    free(prop->ellipsoid);
    free(prop->timeUnit);
    free(prop->speedLengthUnit);
    free(prop->speedTimeUnit);
    free(prop->lengthUnit);
    free(prop->output);
    free(prop->cols);
    free(prop);
}

/**
 * creates a shapehandles structure
 */
//shphandles *createShps(void)
//{
//    shphandles *shps;
//    shps = malloc(sizeof(shphandles));
//    shps->trk = NULL;
//    shps->wpt = NULL;
//    shps->rte = NULL;
//    shps->trk_edg = NULL;
//    shps->rte_edg = NULL;
//    shps->trk_pnt = NULL;
//    shps->rte_pnt = NULL;
//    return shps;
//}

/**
 * creates a dbfhandles structure
 */
/* dbfhandles *createDbfs(void)
{
    dbfhandles *dbfs;
    dbfs = malloc(sizeof(dbfhandles));
    dbfs->trk = NULL;
    dbfs->wpt = NULL;
    dbfs->rte = NULL;
    dbfs->trk_edg = NULL;
    dbfs->rte_edg = NULL;
    dbfs->trk_pnt = NULL;
    dbfs->rte_pnt = NULL;
    return dbfs;
} */

/**
 * creates a parse structure
 */
parsedata *createParsedata(XML_Parser parser, g2sprop * prop)
{
    parsedata *pdata = (parsedata *) malloc(sizeof(parsedata));
    pdata->fp = NULL;
    //shphandles *shps = createShps();
    //dbfhandles *dbfs = createDbfs();
    pathattr *pattr = createPathAttr();
    g2sattr *attr = createAttr();
    parent *p = (parent *) malloc(sizeof(parent));
    parent *c = (parent *) malloc(sizeof(parent));
    p->name = NULL;
    p->parentptr = NULL;
    c->name = "root";
    c->parentptr = p;
    pdata->depth = 0;
    pdata->databuf = malloc(sizeof(char) * DATABUFSIZE);
    pdata->bufptr = NULL;
    pdata->failed = 0;
    pdata->failedid = 0;
    pdata->parser = parser;
    pdata->parent = p;
    pdata->current = c;
    //pdata->shps = shps;
    //pdata->dbfs = dbfs;
    pdata->prop = prop;
    pdata->pattr = pattr;
    pdata->attr = attr;
    return pdata;
}

/*
 * close and free resoures
 */
void closeParsedata(parsedata * pdata)
{
    //free(pdata->shps);
    //free(pdata->dbfs);
    free(pdata->parent);
    free(pdata->current);
    free(pdata->databuf);
    free(pdata->attr);
    free(pdata->pattr);
    free(pdata);
}
