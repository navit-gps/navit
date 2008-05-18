#include "gpx2navit_txt.h"
#include "geodesic.h"

void version(void);
void usage(char **argv);
void setDefault(g2sprop * prop);
void setOptions(int argc, char **argv, g2sprop * prop);

/**
 * Shows a version
 */
void version(void)
{
    fprintf(stdout, "gpx2navit_txt 0.1\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "by Toshihiro Hiraoka\n");
    fprintf(stdout, "   Petter Reinholdtsen\n");
}

/**
 * Shows a usage message
 */
void usage(char **argv)
{
    fprintf(stdout, "Usage: %s gpxfile [options] [-o output basename]\n",
	    argv[0]);
    fprintf(stdout,
	    "-o, --output            Sets output basename. The default is (source file\n");
    fprintf(stdout, "                        name) - (extention name)\n");
    fprintf
	(stdout,
	 "-w, --waypoints         Converts only waypoints data from a gpx file.\n");
    fprintf(stdout,
	    "-t, --trackpoints       Converts only trackpoints data from a gpx file.\n");
    fprintf(stdout,
	    "-r, --routes            Converts only routes data from a gpx file.\n");
    fprintf(stdout,
	    "-a, --all               Converts all types of data from a gpx file.(default)\n");
    fprintf(stdout,
	    "-e, --as-edge           Makes a separated output by each edges.\n");
    fprintf(stdout,
	    "-p, --as-point          Makes a separated output by each points.\n");
    fprintf(stdout,
	    "-s, --stats             Shows simple statistics of the outputs.\n");
    fprintf(stdout,
	    "-b, --basic-columns     Stores only basic data as attribures to \n");
    fprintf(stdout,
	    "                        reduce memory and storage usage.\n");
    fprintf(stdout,
	    "                        (ele, name, cmt, type, time, fix, sym and number).\n");
    fprintf(stdout,
	    "-L, --no-length         Removes length column from a waypoint or trackpoint\n");
    fprintf(stdout, "                        attribute table.\n");
    fprintf
	(stdout,
	 "-S, --no-speed          Removes speed column from a waypoint or trackpoint\n");
    fprintf(stdout, "                        attribute table.\n");
    fprintf
	(stdout,
	 "-T, --no-time           Removes time column from an attribute table.\n");
    fprintf(stdout,
	    "-g, --gpxline           Adds line number of GPX file as attribures.\n");
    fprintf
	(stdout,
	 "-f, --fast              Make it faster without any checks.\n");
    fprintf(stdout,
	    "-3, --3d                Converts data using 3d format. (It's not compatible\n");
    fprintf(stdout, "                        for Arcview 3.x.)\n");
    fprintf(stdout,
	    "    --min-points        Sets path minimum points to convert for noise reduction.\n");
    fprintf(stdout, "                        Default is 2.\n");
    fprintf(stdout,
	    "    --min-length        Sets path minimum length to convert for noise reduction.\n");
    fprintf(stdout, "                        Default is 0.\n");
    fprintf(stdout,
	    "    --min-time          Sets path minimum time period to convert for noise\n");
    fprintf(stdout, "                        reduction.\n");
    fprintf(stdout, "                        Default is 0.\n");
    fprintf(stdout,
	    "    --length-unit       Sets length unit from m,km,feet,mi and etc.\n");
    fprintf(stdout, "                        The default is m.\n");
    fprintf(stdout,
	    "                        You can see the unit list from \"geod -lu\" command.\n");
    fprintf
	(stdout,
	 "    --time-unit         Sets time unit. The default is sec.\n");
    fprintf(stdout,
	    "                        You can set from day, hour, min and sec.\n");
    fprintf(stdout,
	    "    --speed-length-unit Sets length unit for speed.\n");
    fprintf(stdout, "                        The default is km.\n");
    fprintf(stdout,
	    "                        You can see the unit list from \"geod -lu\" command.\n");
    fprintf(stdout,
	    "    --speed-time-unit   Sets time unit for speed calulation. Default is hour.\n");
    fprintf(stdout,
	    "                        You can set from day, hour, min and sec.\n");
    fprintf(stdout,
	    "    --length-ellipsoid  Sets length ellipsoid like UGS84, clrk66. The default is\n");
    fprintf(stdout, "                        UGS84.\n");
    fprintf
	(stdout,
	 "                        You can see the unit list from \"geod -le\" command.\n");
    fprintf(stdout, "-v,  --verbose          Gives many messages.\n");
    fprintf(stdout, "     --version          Shows version.\n");
    fprintf(stdout, "-h,  --help             Shows this list.\n");
}

/**
 * Sets default values to the properties when there is no user setting.
 */
void setDefault(g2sprop * prop)
{
    char *pargv[2];
    int pargc = 2;
    char *ellps;
    /* if there are no options like -p,-w,-e, sets as -a */
    if (!(prop->parseWpt | prop->parseTrk | prop->parseRte)) {
	prop->parseWpt = 1;
	prop->parseTrk = 1;
	prop->parseRte = 1;
    }
    /* if there is no output setting, sets it as [sourcefile name] - ".gpx" */
    if (prop->output == NULL) {
	char *dot = strrchr(prop->sourcefile, '.');
	prop->output =
	    (char *) malloc(sizeof(char) * strlen(prop->sourcefile) + 1);
	if (0 == strcmp(dot, ".gpx")) {
	    int len = dot - prop->sourcefile;
	    strncpy(prop->output, prop->sourcefile, len);
	    prop->output[len] = 0;
	} else {
	    fprintf(stderr,
		    "The source file doesn't have .gpx extension.\n");
	    exit(ERR_ISNOTGPX);
	}
    }
    /* sets ellipsoid "WGS84" */
    if (prop->ellipsoid == NULL) {
	prop->ellipsoid = (char *) malloc(sizeof(char) * 7);
	strcpy(prop->ellipsoid, "WGS84");
    }
    /* sets lengthUnit "m" */
    if (prop->lengthUnit == NULL) {
	prop->lengthUnit = (char *) malloc(sizeof(char) * 2);
	strcpy(prop->lengthUnit, "m");
    }
    /* sets timeUnit "sec" */
    if (prop->timeUnit == NULL) {
	prop->timeUnit = (char *) malloc(sizeof(char) * 4);
	strcpy(prop->timeUnit, "sec");
    }
    /* sets speedLengthUnit "km" */
    if (prop->speedLengthUnit == NULL) {
	prop->speedLengthUnit = (char *) malloc(sizeof(char) * 3);
	strcpy(prop->speedLengthUnit, "km");
    }
    /* sets speedTimeUnit "hour" */
    if (prop->speedTimeUnit == NULL) {
	prop->speedTimeUnit = (char *) malloc(sizeof(char) * 5);
	strcpy(prop->speedTimeUnit, "hour");
    }
    /* sets ellipsoid setting to geod* programs */
    ellps = malloc(sizeof(char) * (strlen(prop->ellipsoid) + 8));
    strcpy(ellps, "+ellps=");
    strcat(ellps, prop->ellipsoid);
    pargv[0] = ellps;
    pargv[1] = prop->lengthUnit;
    checkEllpsUnit(prop->ellipsoid);
    prop->length2meter = checkLengthUnit(prop->lengthUnit);
    prop->time2sec = checkTimeUnit(prop->timeUnit);
    prop->speed2meter = checkLengthUnit(prop->speedLengthUnit);
    prop->speed2sec = checkTimeUnit(prop->speedTimeUnit);
    geod_set(pargc, pargv);
    if (prop->verbose) {
	printf("source filename:\t%s\n", prop->sourcefile);
	printf("output file base name:\t%s\n", prop->output);
    }
    free(ellps);
}

/**
 * Set options from command arguments
 */
void setOptions(int argc, char **argv, g2sprop * prop)
{
    int result;
    /* option struct for getopt_long */
    struct option const long_options[] = {
	{"waypoints", no_argument, 0, 'w'},
	{"trackpoints", no_argument, 0, 't'},
	{"routes", no_argument, 0, 'r'},
	{"output", required_argument, 0, 'o'},
	{"as-edge", no_argument, 0, 'e'},
	{"as-point", no_argument, 0, 'p'},
	{"min-points", required_argument, 0, 'P'},
	{"min-length", required_argument, 0, 'l'},
	{"min-time", required_argument, 0, 'm'},
	{"stats", no_argument, 0, 's'},
	{"basic-columns", no_argument, 0, 'b'},
	{"fast", no_argument, 0, 'f'},
	{"length-unit", required_argument, 0, '4'},
	{"time-unit", required_argument, 0, '8'},
	{"length-ellipsoid", required_argument, 0, '7'},
	{"speed-length-unit", required_argument, 0, '5'},
	{"speed-time-unit", required_argument, 0, '6'},
	{"no-speed", no_argument, 0, 'S'},
	{"no-length", no_argument, 0, 'L'},
	{"no-time", no_argument, 0, 'T'},
	{"verbose", no_argument, 0, 'v'},
	{"gpxline", no_argument, 0, 'g'},
	{"all", no_argument, 0, 'a'},
	{"version", no_argument, 0, 'V'},
	{"help", no_argument, 0, '?'},
	{0, no_argument, 0, '0'},
    };
    if (argc <= 1) {
	fprintf(stderr, "There is no argument.\n");
	usage(argv);
	exit(ERR_NOARGS);
    }
    /* set option attributes */
    while ((result =
	    getopt_long(argc, argv, "3wtrao:epfP:l:m:bS4:5:6:7:8:LTSsvg0",
			long_options, NULL)) != -1) {
	switch (result) {
	case '3':		/* 3d output */
	    prop->is3d = 1;
	    break;
	case 'w':		/* converts only waypoint */
	    prop->parseWpt = 1;
	    break;
	case 't':		/* converts only trackpoint */
	    prop->parseTrk = 1;
	    break;
	case 'r':		/* converts only route */
	    prop->parseRte = 1;
	    break;
	case 'a':		/* converts all */
	    prop->parseWpt = 1;
	    prop->parseTrk = 1;
	    prop->parseRte = 1;
	    break;
	case 'o':		/* sets basename of output file */
	    prop->output =
		(char *) malloc(sizeof(char) * strlen(optarg) + 1);
	    strcpy(prop->output, optarg);
	    break;
	case 'e':		/* make output by each edges */
	    if (prop->isEdge) {
		fprintf(stderr, "option -e cannot use with -f\n");
		exit(ERR_OPTIONCONFRICT);
	    }
	    prop->isEdge = 1;
	    break;
	case 'p':		/* make output by each edges */
	    prop->isPoint = 1;
	    break;
	case 'f':		/* make it faster */
	    if (prop->isEdge) {
		fprintf(stderr, "option -f cannot use with -e\n");
		exit(ERR_OPTIONCONFRICT);
	    }
	    prop->isFast = 1;
	    prop->cols->desc = 0;
	    prop->cols->src = 0;
	    prop->cols->link = 0;
	    prop->cols->magvar = 0;
	    prop->cols->geoidheight = 0;
	    prop->cols->sat = 0;
	    prop->cols->hdop = 0;
	    prop->cols->vdop = 0;
	    prop->cols->pdop = 0;
	    prop->cols->ageofdgpsdata = 0;
	    prop->cols->dgpsid = 0;
	    prop->cols->length = 0;
	    prop->cols->interval = 0;
	    prop->cols->speed = 0;
	    break;
	case 'P':		/* sets minimun points as a path */
	    prop->minpoints = atoi(optarg);
	    break;
	case 'l':		/* sets minimun length as a path */
	    prop->minlength = atoi(optarg);
	    break;
	case 'm':		/* sets minimun time as a path */
	    prop->mintime = atoi(optarg);
	    break;
	case 'b':		/* use only some columns */
	    prop->cols->desc = 0;
	    prop->cols->src = 0;
	    prop->cols->link = 0;
	    prop->cols->magvar = 0;
	    prop->cols->geoidheight = 0;
	    prop->cols->sat = 0;
	    prop->cols->hdop = 0;
	    prop->cols->vdop = 0;
	    prop->cols->pdop = 0;
	    prop->cols->ageofdgpsdata = 0;
	    prop->cols->dgpsid = 0;
	    prop->cols->length = 0;
	    prop->cols->interval = 0;
	    prop->cols->speed = 0;
	    break;
	case 'S':		/* doesn't make speed column */
	    prop->cols->speed = 0;
	    break;
	case '4':		/* sets length unit */
	    prop->lengthUnit = malloc(sizeof(char) * (strlen(optarg) + 1));
	    strcpy(prop->lengthUnit, optarg);
	    break;
	case '5':		/* sets length unit for calculating speed */
	    prop->speedLengthUnit =
		malloc(sizeof(char) * (strlen(optarg) + 1));
	    strcpy(prop->speedLengthUnit, optarg);
	    break;
	case '6':		/* sets time unit for calculating speed */
	    prop->speedTimeUnit =
		malloc(sizeof(char) * (strlen(optarg) + 1));
	    strcpy(prop->speedTimeUnit, optarg);
	    break;
	case '7':		/* sets ellipsoid for calculating length */
	    prop->ellipsoid = malloc(sizeof(char) * (strlen(optarg) + 1));
	    strcpy(prop->ellipsoid, optarg);
	    break;
	case '8':		/* sets time unit */
	    prop->timeUnit = malloc(sizeof(char) * (strlen(optarg) + 1));
	    strcpy(prop->timeUnit, optarg);
	    break;
	case 'L':		/* doesn't make length column */
	    prop->cols->length = 0;
	    break;
	case 'T':		/* doesn't make time column */
	    prop->cols->interval = 0;
	    break;
	case 's':		/* shows source file stats */
	    prop->needsStats = 1;
	    break;
	case 'v':		/* verbose mode */
	    prop->verbose = 1;
	    break;
	case 'V':		/* shows version */
	    version();
	    exit(EXIT_SUCCESS);
	    break;
	case 'g':		/* adds gpx line number column */
	    prop->cols->gpxline = 1;
	    break;
	case ':':
	    usage(argv);
	    exit(ERR_WRONGOPTION);
	    break;
	case '0':
	    usage(argv);
	    exit(ERR_WRONGOPTION);
	    break;
	default:
	    usage(argv);
	    exit(ERR_WRONGOPTION);
	    break;
	}
    }
    /* gets a source file name */
    if(argv[optind] == NULL) {
	fprintf(stderr, "There is no gpxfile description.\n");
	usage(argv);
	exit(ERR_WRONGOPTION);
    }
    prop->sourcefile = malloc(sizeof(char) * (strlen(argv[optind]) + 1));
    /** @note needs to change here to support 
     * a several files convertion */
    strcpy(prop->sourcefile, argv[optind]);
    setDefault(prop);
}

/**
 * Main
 */
int main(int argc, char **argv)
{
    g2sprop *prop;
    prop = createProp();
    setOptions(argc, argv, prop);
    parseMain(prop);
    if (prop->needsStats)
	showStats(prop);
    closeProp(prop);
    return (0);
}
