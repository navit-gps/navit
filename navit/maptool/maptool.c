/**
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

#include "config.h"
#include "navit_lfs.h"
#include <stdlib.h>
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#ifdef _MSC_VER
#include "getopt_long.h"
#define atoll _atoi64
#else
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"
#include "item.h"
#include "map.h"
#include "main.h"
#include "zipfile.h"
#include "linguistics.h"
#include "plugin.h"
#include "util.h"
#include "maptool.h"

#define SLIZE_SIZE_DEFAULT_GB 1
long long slice_size=SLIZE_SIZE_DEFAULT_GB*1024ll*1024*1024;
int attr_debug_level=1;
int ignore_unknown = 0;
GHashTable *dedupe_ways_hash;
int phase;
int slices;
int unknown_country;
char ch_suffix[] ="r"; /* Used to make compiler happy due to Bug 35903 in gcc */
/** Textual description of available experimental features, or NULL (=none available). */
char* experimental_feature_description =
    "Move coastline data to order 6 tiles. Makes map look more smooth, but may affect drawing/searching performance."; /* add description here */
/** Indicates if experimental features (if available) were enabled. */
int experimental;

struct buffer node_buffer = {
    64*1024*1024,
};

int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;

int overlap=1;

int bytes_read;

static long start_brk;
#ifdef _WIN32
#define timespec timeval
#endif
static struct timespec start_ts;

/*
  Asynchronous signal safe lltoa function (note: no trailing \0 char!)
*/
static int assafe_lltoa(long long n, int maxlen, char *buf) {
    int i;
    int out_length;

    if(maxlen<1)
        return 0;

    if(n<0) {
        n=-n;
        buf[0]='-';
        maxlen--;
        buf++;
    } else if(n==0) {
        buf[0]='0';
        return 1;
    }

    for(i=0; n>0 && i<maxlen; i++) {
        buf[i]='0'+(n%10);
        n/=10;
    }
    out_length=i;
    for(i=0; i<out_length/2; i++) {
        char c=buf[i];
        buf[i]=buf[out_length-i-1];
        buf[out_length-i-1]=c;
    }
    return out_length;
}

/*
  Asynchronous signal safe string copy to buffer function (note: no trailing \0 char!)
*/
static int assafe_strcp2buf(char *str, int maxlen, char *buf) {
    int i;
    for(i=0; str[i] && i<maxlen; i++)
        buf[i]=str[i];
    return i;
}

static void progress_time(void) {
    struct timespec ts;
    int seconds;
    const int buflen=20;
    char buf[buflen];
    int pos=1;
    buf[0]=' ';
#ifdef _WIN32
    gettimeofday(&ts, NULL);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    seconds=ts.tv_sec-start_ts.tv_sec;
    pos+=assafe_lltoa(seconds/60, buflen-pos, buf+pos);
    seconds%=60;
    pos+=assafe_strcp2buf(seconds>9?":":":0", buflen-pos, buf+pos);
    pos+=assafe_lltoa(seconds, buflen-pos, buf+pos);
    if (write(2,buf,pos) == -1) {
        dbg(lvl_warning, "Writing progress time failed. Error-Code: %d", errno);
    }
}

static void progress_memory(void) {
#ifdef HAVE_SBRK
    long mem=(long)sbrk(0)-start_brk;
    const int buflen=20;
    char buf[buflen];
    int pos=1;
    int write_result;
    buf[0]=' ';
    pos+=assafe_lltoa(mem/1024/1024, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" MB", buflen-pos, buf+pos);
    write_result = write(2,buf,pos);
    if (write_result == -1) {
        dbg(lvl_warning, "Writing progress memory failed. Error-Code: %d", errno);
    }
#endif
}

static void sig_alrm_do(int sig) {
    const int buflen=1024;
    char buf[buflen];
    int pos=0;
    int write_result;
#ifndef _WIN32
    signal(SIGALRM, sig_alrm_do);
    alarm(30);
#endif
    pos+=assafe_strcp2buf("PROGRESS", buflen-pos, buf+pos);
    pos+=assafe_lltoa(phase, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(": Processed ", buflen-pos, buf+pos);
    pos+=assafe_lltoa(processed_nodes, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" nodes (", buflen-pos, buf+pos);
    pos+=assafe_lltoa(processed_nodes_out, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" out) ", buflen-pos, buf+pos);
    pos+=assafe_lltoa(processed_ways, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" ways ", buflen-pos, buf+pos);
    pos+=assafe_lltoa(processed_relations, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" relations ", buflen-pos, buf+pos);
    pos+=assafe_lltoa(processed_tiles, buflen-pos, buf+pos);
    pos+=assafe_strcp2buf(" tiles", buflen-pos, buf+pos);
    write_result = write(2,buf,pos);
    if (write_result == -1) {
        dbg(lvl_warning, "Writing sig alrm ailed. Error-Code: %d", errno);
    }
    progress_time();
    progress_memory();
#ifndef _WIN32
    write_result = write(2,"\r\n",2);
    if (write_result == -1) {
        dbg(lvl_warning, "Writing new line in sig alrm ailed. Error-Code: %d", errno);
    }
#else
    write_result = write(2,"\n",1);
    if (write_result == -1) {
        dbg(lvl_warning, "Writing new line in sig alrm ailed. Error-Code: %d", errno);
    }
#endif
}

void sig_alrm(int sig) {
    fflush(stderr);
    sig_alrm_do(sig);
}



void sig_alrm_end(void) {
#ifndef _WIN32
    alarm(0);
#endif
}

static struct files_relation_processing *files_relation_processing_new(FILE *line2poi, char *suffix) {
    struct files_relation_processing *result = g_new(struct files_relation_processing, 1);
    result->ways_in=tempfile(suffix,"ways_split",0);
    result->ways_out=tempfile(suffix,"ways_split_relproc_tmp",1);
    result->nodes_in=tempfile(suffix,"nodes",0);
    result->nodes_out=tempfile(suffix,"nodes_relproc_tmp",1);
    result->nodes2_in=NULL;
    result->nodes2_out=NULL;
    if(line2poi) {
        result->nodes2_in=tempfile(suffix,"way2poi_result",0);
        result->nodes2_out=tempfile(suffix,"way2poi_result_relproc_tmp",1);
    }
    return result;
}

static void files_relation_processing_destroy(struct files_relation_processing *files_relproc, char *suffix) {
    fclose(files_relproc->ways_in);
    fclose(files_relproc->nodes_in);
    fclose(files_relproc->ways_out);
    fclose(files_relproc->nodes_out);
    tempfile_rename(suffix,"ways_split_relproc_tmp","ways_split");
    tempfile_rename(suffix,"nodes_relproc_tmp","nodes");
    if(files_relproc->nodes2_in) {
        fclose(files_relproc->nodes2_in);
        fclose(files_relproc->nodes2_out);
        tempfile_rename(suffix,"way2poi_result_relproc_tmp","way2poi_result");
    }
    g_free(files_relproc);
}

static struct plugins *plugins;

static void add_plugin(char *path) {
    struct attr pa_attr= {attr_path};
    struct attr pl_attr= {attr_plugins};
    struct attr *attrs[2]= {&pa_attr,NULL};

    if (! plugins) {
        file_init();
        plugins=plugins_new();
    }
    pa_attr.u.str=path;
    pl_attr.u.plugins=plugins;
    plugin_new(&pl_attr,attrs);
}

static void maptool_init(FILE* rule_file) {
    if (plugins)
        plugins_init(plugins);
    osm_init(rule_file);
}

static void usage(void) {
    FILE *f = stdout;
    /* DEVELOPERS : don't forget to update the manpage if you modify theses options */
    fprintf(f,"\n");
    fprintf(f,"maptool - parse osm textfile and convert to Navit binfile format\n\n");
    fprintf(f,"Usage (for OSM XML data):\n");
    fprintf(f,"bzcat planet.osm.bz2 | maptool mymap.bin\n");
    fprintf(f,"Usage (for OSM Protobuf/PBF data):\n");
    fprintf(f,"maptool --protobuf -i planet.osm.pbf planet.bin\n");
    fprintf(f,"Available switches:\n");
    fprintf(f,"-h (--help)                       : this screen\n");
    fprintf(f,"-6 (--64bit)                      : set zip 64 bit compression\n");
    fprintf(f,"-a (--attr-debug-level)  <level>  : control which data is included in the debug attribute\n");
    fprintf(f,"-c (--dump-coordinates)           : dump coordinates after phase 1\n");
#ifdef HAVE_POSTGRESQL
    fprintf(f,
            "-d (--db) <conn. string>          : get osm data out of a postgresql database with osm simple scheme and given connect string\n");
#endif
    fprintf(f,"-D (--dump)                       : dump map data to standard output in Navit textfile format\n");
    fprintf(f,"-e (--end) <phase>                : end at specified phase\n");
    fprintf(f,"-E (--experimental)               : Enable experimental features (%s)\n",
            experimental_feature_description ? experimental_feature_description : "-not available in this version-");
    fprintf(f,"-i (--input-file) <file>          : specify the input file name (OSM), overrules default stdin\n");
    fprintf(f,"-k (--keep-tmpfiles)              : do not delete tmp files after processing. useful to reuse them\n");
    fprintf(f,"-M (--o5m)                        : input data is in o5m format\n");
    fprintf(f,"-n (--ignore-unknown)             : do not output ways and nodes with unknown type\n");
    fprintf(f,"-N (--nodes-only)                 : process only nodes\n");
    fprintf(f,"-P (--protobuf)                   : input data is in pbf (Protocol Buffer) format\n");
    fprintf(f,"-r (--rule-file) <file>           : read mapping rules from specified file\n");
    fprintf(f,"-s (--start) <phase>              : start at specified phase\n");
    fprintf(f,
            "-S (--slice-size) <size>          : limit memory to use for some large internal buffers, in bytes. Default is %dGB.\n",
            SLIZE_SIZE_DEFAULT_GB);
    fprintf(f,"-t (--timestamp) <y-m-dTh:m:s>    : Set zip timestamp\n");
    fprintf(f,
            "-w (--dedupe-ways)                : ensure no duplicate ways or nodes. useful when using several input files\n");
    fprintf(f,"-W (--ways-only)                  : process only ways\n");
    fprintf(f,"-U (--unknown-country)            : add objects with unknown country to index\n");
    fprintf(f,"-x (--index-size)                 : set maximum country index size in bytes\n");
    fprintf(f,"-z (--compression-level) <level>  : set the compression level\n");
    fprintf(f,"Internal options (undocumented):\n");
    fprintf(f,"-b (--binfile)\n");
    fprintf(f,"-B \n");
    fprintf(f,"-m (--map) \n");
    fprintf(f,"-O \n");
    fprintf(f,"-p (--plugin) \n");
    fprintf(f,"-u (--url) \n");

    exit(0);
}

struct maptool_params {
    int zip64;
    int keep_tmpfiles;
    int process_nodes;
    int process_ways;
    int process_relations;
    char *protobufdb;
    char *protobufdb_operation;
    int start;
    int end;
    int dump;
    int o5m;
    int compression_level;
    int protobuf;
    int dump_coordinates;
    int input;
    GList *map_handles;
    FILE* input_file;
    FILE* rule_file;
    char *url;
    struct maptool_osm osm;
    FILE *ways_split;
    char *timestamp;
    char *result;
    char *dbstr;
    int node_table_loaded;
    int countries_loaded;
    int tilesdir_loaded;
    int max_index_size;
};

static int parse_option(struct maptool_params *p, char **argv, int argc, int *option_index) {
    char *optarg_cp,*attr_name,*attr_value;
    struct map *handle;
    struct attr *attrs[10];
    int pos,c,i;

    static struct option long_options[] = {
        {"64bit", 0, 0, '6'},
        {"attr-debug-level", 1, 0, 'a'},
        {"binfile", 0, 0, 'b'},
        {"compression-level", 1, 0, 'z'},
#ifdef HAVE_POSTGRESQL
        {"db", 1, 0, 'd'},
#endif
        {"dedupe-ways", 0, 0, 'w'},
        {"dump", 0, 0, 'D'},
        {"dump-coordinates", 0, 0, 'c'},
        {"end", 1, 0, 'e'},
        {"experimental", 0, 0, 'E'},
        {"help", 0, 0, 'h'},
        {"keep-tmpfiles", 0, 0, 'k'},
        {"nodes-only", 0, 0, 'N'},
        {"map", 1, 0, 'm'},
        {"o5m", 0, 0, 'M'},
        {"plugin", 1, 0, 'p'},
        {"protobuf", 0, 0, 'P'},
        {"start", 1, 0, 's'},
        {"timestamp", 1, 0, 't'},
        {"input-file", 1, 0, 'i'},
        {"rule-file", 1, 0, 'r'},
        {"ignore-unknown", 0, 0, 'n'},
        {"url", 1, 0, 'u'},
        {"ways-only", 0, 0, 'W'},
        {"slice-size", 1, 0, 'S'},
        {"unknown-country", 0, 0, 'U'},
        {"index-size", 0, 0, 'x'},
        {0, 0, 0, 0}
    };
    c = getopt_long (argc, argv, "6B:DEMNO:PS:Wa:bc"
#ifdef HAVE_POSTGRESQL
                     "d:"
#endif
                     "e:hi:knm:p:r:s:t:wu:z:Ux:", long_options, option_index);
    if (c == -1)
        return 1;
    switch (c) {
    case '6':
        p->zip64=1;
        break;
    case 'B':
        p->protobufdb=optarg;
        break;
    case 'D':
        p->dump=1;
        break;
    case 'E':
        experimental=1;
        break;
    case 'M':
        p->o5m=1;
        break;
    case 'N':
        p->process_ways=0;
        break;
    case 'R':
        p->process_relations=0;
        break;
    case 'O':
        p->protobufdb_operation=optarg;
        p->dump=1;
        break;
    case 'P':
        p->protobuf=1;
        break;
    case 'S':
        slice_size=atoll(optarg);
        break;
    case 'W':
        p->process_nodes=0;
        break;
    case 'U':
        unknown_country=1;
        break;
    case 'a':
        attr_debug_level=atoi(optarg);
        break;
    case 'b':
        p->input=1;
        break;
    case 'c':
        p->dump_coordinates=1;
        break;
#ifdef HAVE_POSTGRESQL
    case 'd':
        p->dbstr=optarg;
        break;
#endif
    case 'e':
        p->end=atoi(optarg);
        break;
    case 'h':
        return 2;
    case 'm':
        optarg_cp=g_strdup(optarg);
        pos=0;
        i=0;
        attr_name=g_strdup(optarg);
        attr_value=g_strdup(optarg);
        while (i < 9 && attr_from_line(optarg_cp, NULL, &pos, attr_value, attr_name)) {
            attrs[i]=attr_new_from_text(attr_name,attr_value);
            if (attrs[i]) {
                i++;
            } else {
                fprintf(stderr,"Failed to convert %s=%s to attribute\n",attr_name,attr_value);
            }
            attr_value=g_strdup(optarg);
        }
        attrs[i++]=NULL;
        g_free(attr_value);
        g_free(optarg_cp);
        handle=map_new(NULL, attrs);
        if (! handle) {
            fprintf(stderr,"Failed to create map from attributes\n");
            exit(1);
        }
        p->map_handles=g_list_append(p->map_handles,handle);
        break;
    case 'n':
        fprintf(stderr,"I will IGNORE unknown types\n");
        ignore_unknown=1;
        break;
    case 'k':
        fprintf(stderr,"I will KEEP tmp files\n");
        p->keep_tmpfiles=1;
        break;
    case 'p':
        add_plugin(optarg);
        break;
    case 's':
        p->start=atoi(optarg);
        break;
    case 't':
        p->timestamp=optarg;
        break;
    case 'w':
        dedupe_ways_hash=g_hash_table_new(NULL, NULL);
        break;
    case 'i':
        p->input_file = fopen( optarg, "r" );
        if (p->input_file ==  NULL ) {
            fprintf( stderr, "\nInput file (%s) not found\n", optarg );
            exit( 1 );
        }
        break;
    case 'r':
        p->rule_file = fopen( optarg, "r" );
        if (p->rule_file ==  NULL ) {
            fprintf( stderr, "\nRule file (%s) not found\n", optarg );
            exit( 1 );
        }
        break;
    case 'u':
        p->url=optarg;
        break;
    case 'x':
        p->max_index_size=atoi(optarg);
        break;
#ifdef HAVE_ZLIB
    case 'z':
        p->compression_level=atoi(optarg);
        break;
#endif
    case '?':
    default:
        return 0;
    }
    return 3;
}

static int start_phase(struct maptool_params *p, char *str) {
    phase++;
    if (p->start <= phase && p->end >= phase) {
        fprintf(stderr,"PROGRESS: Phase %d: %s",phase,str);
        fflush(stderr);
        progress_time();
        progress_memory();
        fprintf(stderr,"\n");
        return 1;
    } else
        return 0;
}

static void exit_with_error(char* error_message) {
    fprintf(stderr, "%s", error_message);
    exit(1);
}

static void osm_read_input_data(struct maptool_params *p, char *suffix) {
    unlink("coords.tmp");
    if (p->process_ways)
        p->osm.ways=tempfile(suffix,"ways",1);
    if (p->process_nodes) {
        p->osm.nodes=tempfile(suffix,"nodes",1);
        p->osm.towns=tempfile(suffix,"towns",1);
    }
    if (p->process_ways && p->process_nodes) {
        p->osm.turn_restrictions=tempfile(suffix,"turn_restrictions",1);
        p->osm.line2poi=tempfile(suffix,"line2poi",1);
        p->osm.poly2poi=tempfile(suffix,"poly2poi",1);
    }
    if (p->process_relations) {
        p->osm.boundaries=tempfile(suffix,"boundaries",1);
        p->osm.associated_streets=tempfile(suffix,"associated_streets",1);
        p->osm.house_number_interpolations=tempfile(suffix,"house_number_interpolations",1);
    }
#ifdef HAVE_POSTGRESQL
    if (p->dbstr)
        map_collect_data_osm_db(p->dbstr,&p->osm);
    else
#endif
        if (p->map_handles) {
            GList *l;
            phase1_map(p->map_handles,p->osm.ways,p->osm.nodes);
            l=p->map_handles;
            while (l) {
                map_destroy(l->data);
                l=g_list_next(l);
            }
        } else if (p->protobuf) {
#ifdef _MSC_VER
            exit_with_error("Option -P not yet supported on MSVC\n");
#else
            map_collect_data_osm_protobuf(p->input_file,&p->osm);
#endif
        } else if (p->o5m)
            map_collect_data_osm_o5m(p->input_file,&p->osm);
        else
            map_collect_data_osm(p->input_file,&p->osm);

    if (node_buffer.size==0 && !p->map_handles) {
        fprintf(stderr,"No nodes found - looks like an invalid input file.\n");
        exit(1);
    }
    flush_nodes(1);
    if (p->osm.ways)
        fclose(p->osm.ways);
    if (p->osm.nodes)
        fclose(p->osm.nodes);
    if (p->osm.turn_restrictions)
        fclose(p->osm.turn_restrictions);
    if (p->osm.associated_streets)
        fclose(p->osm.associated_streets);
    if (p->osm.house_number_interpolations)
        fclose(p->osm.house_number_interpolations);
    if (p->osm.boundaries)
        fclose(p->osm.boundaries);
    if (p->osm.poly2poi)
        fclose(p->osm.poly2poi);
    if (p->osm.line2poi)
        fclose(p->osm.line2poi);
    if (p->osm.towns)
        fclose(p->osm.towns);
}
int debug_ref=0;

static void osm_count_references(struct maptool_params *p, char *suffix, int clear) {
    int i,first=1;
    fprintf(stderr,"%d slices\n",slices);
    for (i = slices-1 ; i>=0 ; i--) {
        fprintf(stderr, "slice %d of %d\n",slices-i-1,slices-1);
        if (!first) {
            FILE *ways=tempfile(suffix,"ways",0);
            load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
            if (clear)
                clear_node_item_buffer();
            ref_ways(ways);
            save_buffer("coords.tmp",&node_buffer, i*slice_size);
            fclose(ways);
        }
        FILE *poly2poi=tempfile(suffix,first?"poly2poi":"poly2poi_resolved",0);
        FILE *poly2poinew=tempfile(suffix,"poly2poi_resolved_new",1);
        FILE *line2poi=tempfile(suffix,first?"line2poi":"line2poi_resolved",0);
        FILE *line2poinew=tempfile(suffix,"line2poi_resolved_new",1);
        resolve_ways(poly2poi, poly2poinew);
        resolve_ways(line2poi, line2poinew);
        fclose(poly2poi);
        fclose(poly2poinew);
        fclose(line2poi);
        fclose(line2poinew);
        tempfile_rename(suffix,"poly2poi_resolved_new","poly2poi_resolved");
        tempfile_rename(suffix,"line2poi_resolved_new","line2poi_resolved");
        if (first && !p->keep_tmpfiles) {
            tempfile_unlink(suffix,"poly2poi");
            tempfile_unlink(suffix,"line2poi");
        }
        first=0;
    }
}


static void osm_resolve_coords_and_split_at_intersections(struct maptool_params *p, char *suffix) {
    FILE *ways, *ways_split, *ways_split_index, *graph, *coastline;
    int i;

    ways=tempfile(suffix,"ways",0);
    for (i = 0 ; i < slices ; i++) {
        int final=(i >= slices-1);
        ways_split=tempfile(suffix,"ways_split",1);
        ways_split_index=final ? tempfile(suffix,"ways_split_index",1) : NULL;
        graph=tempfile(suffix,"graph",1);
        coastline=tempfile(suffix,"coastline",1);
        if (i)
            load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
        map_resolve_coords_and_split_at_intersections(ways,ways_split,ways_split_index,graph,coastline,final);
        fclose(ways_split);
        if (ways_split_index)
            fclose(ways_split_index);
        fclose(ways);
        fclose(graph);
        fclose(coastline);
        if (! final) {
            tempfile_rename(suffix,"ways_split","ways_to_resolve");
            ways=tempfile(suffix,"ways_to_resolve",0);
        }
    }
    if(!p->keep_tmpfiles)
        tempfile_unlink(suffix,"ways");
    tempfile_unlink(suffix,"ways_to_resolve");
}

static void osm_process_way2poi(struct maptool_params *p, char *suffix) {
    FILE *poly2poi=tempfile(suffix,"poly2poi_resolved",0);
    FILE *line2poi=tempfile(suffix,"line2poi_resolved",0);
    FILE *way2poi_result=tempfile(suffix,"way2poi_result",1);
    if (poly2poi) {
        process_way2poi(poly2poi, way2poi_result, type_area);
        fclose(poly2poi);
    }
    if (line2poi) {
        process_way2poi(line2poi, way2poi_result, type_line);
        fclose(line2poi);
    }
    fclose(way2poi_result);
}

static void osm_process_coastlines(struct maptool_params *p, char *suffix) {
    FILE *coastline=tempfile(suffix,"coastline",0);
    if (coastline) {
        FILE *coastline_result=tempfile(suffix,"coastline_result",1);
        process_coastlines(coastline, coastline_result);
        fclose(coastline_result);
        fclose(coastline);
    }
}

static void osm_process_turn_restrictions(struct maptool_params *p, char *suffix) {
    FILE *ways_split, *ways_split_index, *relations, *coords;
    p->osm.turn_restrictions=tempfile(suffix,"turn_restrictions",0);
    if (!p->osm.turn_restrictions)
        return;
    relations=tempfile(suffix,"relations",1);
    coords=fopen("coords.tmp","rb");
    ways_split=tempfile(suffix,"ways_split",0);
    ways_split_index=tempfile(suffix,"ways_split_index",0);
    process_turn_restrictions(p->osm.turn_restrictions,coords,ways_split,ways_split_index,relations);
    fclose(ways_split_index);
    fclose(ways_split);
    fclose(coords);
    fclose(relations);
    fclose(p->osm.turn_restrictions);
    if(!p->keep_tmpfiles)
        tempfile_unlink(suffix,"turn_restrictions");
}

static void maptool_dump(struct maptool_params *p, char *suffix) {
    char *files[10];
    int i,files_count=0;
    if (p->process_nodes)
        files[files_count++]="nodes";
    if (p->process_ways)
        files[files_count++]="ways_split";
    if (p->process_relations)
        files[files_count++]="relations";
    for (i = 0 ; i < files_count ; i++) {
        FILE *f=tempfile(suffix,files[i],0);
        if (f) {
            dump(f);
            fclose(f);
        }
    }
}

static void maptool_generate_tiles(struct maptool_params *p, char *suffix, char **filenames, int filename_count,
                                   int first,
                                   char *suffix0) {
    struct zip_info *zip_info;
    FILE *tilesdir;
    FILE *files[10];
    int zipnum, f;
    if (first) {
        zip_info=zip_new();
        zip_set_zip64(zip_info, p->zip64);
        zip_set_timestamp(zip_info, p->timestamp);
    }
    zipnum=zip_get_zipnum(zip_info);
    tilesdir=tempfile(suffix,"tilesdir",1);
    if (!strcmp(suffix,ch_suffix)) { /* Makes compiler happy due to bug 35903 in gcc */
        ch_generate_tiles(suffix0,suffix,tilesdir,zip_info);
    } else {
        for (f = 0 ; f < filename_count ; f++)
            files[f]=tempfile(suffix,filenames[f],0);
        phase4(files,filename_count,0,suffix,tilesdir,zip_info);
        for (f = 0 ; f < filename_count ; f++) {
            if (files[f])
                fclose(files[f]);
        }
    }
    fclose(tilesdir);
    zip_set_zipnum(zip_info,zipnum);
}

static void maptool_assemble_map(struct maptool_params *p, char *suffix, char **filenames, char **referencenames,
                                 int filename_count, int first, int last, char *suffix0) {
    FILE *files[10];
    FILE *references[10];
    struct zip_info *zip_info;
    int zipnum,f;

    if (first) {
        char *zipdir=tempfile_name("zipdir","");
        char *zipindex=tempfile_name("index","");
        zip_info=zip_new();
        zip_set_zip64(zip_info, p->zip64);
        zip_set_timestamp(zip_info, p->timestamp);
        zip_set_maxnamelen(zip_info, 14+strlen(suffix0));
        zip_set_compression_level(zip_info, p->compression_level);
        if(!zip_open(zip_info, p->result, zipdir, zipindex)) {
            fprintf(stderr,"Fatal: Could not write output file.\n");
            exit(1);
        }
        if (p->url) {
            map_information_attrs[1].type=attr_url;
            map_information_attrs[1].u.str=p->url;
        }
        index_init(zip_info, 1);
    }
    if (!strcmp(suffix,ch_suffix)) {  /* Makes compiler happy due to bug 35903 in gcc */
        ch_assemble_map(suffix0,suffix,zip_info);
    } else {
        for (f = 0 ; f < filename_count ; f++) {
            files[f]=tempfile(suffix, filenames[f], 0);
            if (referencenames[f])
                references[f]=tempfile(suffix,referencenames[f],1);
            else
                references[f]=NULL;
        }
        phase5(files,references,filename_count,0,suffix,zip_info);
        for (f = 0 ; f < filename_count ; f++) {
            if (files[f])
                fclose(files[f]);
            if (references[f])
                fclose(references[f]);
        }
    }
    if(!p->keep_tmpfiles) {
        tempfile_unlink(suffix,"relations");
        tempfile_unlink(suffix,"nodes");
        tempfile_unlink(suffix,"ways_split");
        tempfile_unlink(suffix,"poly2poi_resolved");
        tempfile_unlink(suffix,"line2poi_resolved");
        tempfile_unlink(suffix,"ways_split_ref");
        tempfile_unlink(suffix,"coastline");
        tempfile_unlink(suffix,"turn_restrictions");
        tempfile_unlink(suffix,"graph");
        tempfile_unlink(suffix,"tilesdir");
        tempfile_unlink(suffix,"boundaries");
        tempfile_unlink(suffix,"way2poi_result");
        tempfile_unlink(suffix,"coastline_result");
        tempfile_unlink(suffix,"towns_poly");
        unlink("coords.tmp");
    }
    if (last) {
        zipnum=zip_get_zipnum(zip_info);
        add_aux_tiles("auxtiles.txt", zip_info);
        write_countrydir(zip_info,p->max_index_size);
        zip_set_zipnum(zip_info, zipnum);
        write_aux_tiles(zip_info);
        zip_write_index(zip_info);
        zip_write_directory(zip_info);
        zip_close(zip_info);
        if (!p->keep_tmpfiles) {
            remove_countryfiles();
            tempfile_unlink("index","");
            tempfile_unlink("zipdir","");
        }
    }
}

static void maptool_load_node_table(struct maptool_params *p, int last) {
    if (!p->node_table_loaded) {
        slices=(sizeof_buffer("coords.tmp")+(long long)slice_size-(long long)1)/(long long)slice_size;
        assert(slices>0);
        load_buffer("coords.tmp",&node_buffer,last?(slices-1)*slice_size:0, slice_size);
        p->node_table_loaded=1;
    }
}

static void maptool_load_countries(struct maptool_params *p) {
    if (!p->countries_loaded) {
        load_countries();
        p->countries_loaded=1;
    }
}

static void maptool_load_tilesdir(struct maptool_params *p, char *suffix) {
    if (!p->tilesdir_loaded) {
        FILE *tilesdir=tempfile(suffix,"tilesdir",0);
        load_tilesdir(tilesdir);
        p->tilesdir_loaded=1;
    }
}

int main(int argc, char **argv) {
    struct maptool_params p;
    char *suffixes[]= {""};
    char *suffix=suffixes[0];
    char *filenames[20];
    char *referencenames[20];
    int filename_count=0;

    int suffix_count=sizeof(suffixes)/sizeof(char *);
    int i;
    int suffix_start=0;
    int option_index=0;
    main_init(argv[0]);
#ifndef HAVE_GLIB
    _g_slice_thread_init_nomessage();
#endif
    linguistics_init();

    memset(&p, 0, sizeof(p));
#ifdef HAVE_ZLIB
    p.compression_level=9;
#endif
    p.start=1;
    p.end=99;
    p.input_file=stdin;
    p.process_nodes=1;
    p.process_ways=1;
    p.process_relations=1;
    p.timestamp=current_to_iso8601();
    p.max_index_size=65536;

#ifdef HAVE_SBRK
    start_brk=(long)sbrk(0);
#endif
#ifdef _WIN32
    gettimeofday(&start_ts,NULL);
#else
    clock_gettime(CLOCK_REALTIME, &start_ts);
#endif
    while (1) {
        int parse_result=parse_option(&p, argv, argc, &option_index);
        if (!parse_result) {
            exit(1);
        }
        if (parse_result == 1)
            break;
        if (parse_result == 2) {
            usage();
            exit(0);
        }
    }
    if (experimental && (!experimental_feature_description )) {
        exit_with_error("No experimental features available in this version, aborting. \n");
    }
    if (optind < argc -1) {
        exit_with_error("Only one non-option argument allowed.\n");
    }
    if (p.dump == 0 && optind != argc -1) {
        exit_with_error("Please specify an output file.\n");
    }
    if (p.dump == 1 && optind != argc) {
        exit_with_error("Please do not specify an output file in dump mode.\n");
    }

    p.result=argv[optind];


    // initialize plugins and OSM mappings
    maptool_init(p.rule_file);
    if (p.protobufdb_operation) {
#ifdef _MSC_VER
        exit_with_error("Option -O not yet supported on MSVC\n");
#else
        osm_protobufdb_load(p.input_file, p.protobufdb);
        return 0;
#endif
    }
    phase=0;

    // input from an OSM file
    if (p.input == 0) {
        if (start_phase(&p, "reading input data")) {
            osm_read_input_data(&p, suffix);
            p.node_table_loaded=1;
        }
        if (start_phase(&p, "counting references and resolving ways")) {
            maptool_load_node_table(&p,1);
            osm_count_references(&p, suffix, p.start == phase);
        }
        if (start_phase(&p,"converting ways to pois")) {
            osm_process_way2poi(&p, suffix);
        }
        if (start_phase(&p,"splitting at intersections")) {
            if (p.process_ways) {
                maptool_load_node_table(&p,0);
                osm_resolve_coords_and_split_at_intersections(&p, suffix);
            }
        }
        g_free(node_buffer.base);
        node_buffer.base=NULL;
        node_buffer.malloced=0;
        node_buffer.size=0;
        p.node_table_loaded=0;
    } else {
        if (start_phase(&p,"reading data")) {
            FILE *ways_split=tempfile(suffix,"ways_split",1);
            process_binfile(stdin, ways_split);
            fclose(ways_split);
        }
    }
    if (start_phase(&p,"generating coastlines")) {
        osm_process_coastlines(&p, suffix);
    }
    if (start_phase(&p,"assigning towns to countries")) {
        FILE *towns=tempfile(suffix,"towns",0),*boundaries=NULL,*ways=NULL;
        if (towns) {
            boundaries=tempfile(suffix,"boundaries",0);
            ways=tempfile(suffix,"ways_split",0);
            osm_process_towns(towns,boundaries,ways,suffix);
            fclose(ways);
            fclose(boundaries);
            fclose(towns);
            if(!p.keep_tmpfiles)
                tempfile_unlink(suffix,"towns");
        }
    }
    if (start_phase(&p,"sorting countries")) {
        sort_countries(p.keep_tmpfiles);
        p.countries_loaded=1;
    }
    if (start_phase(&p,"generating turn restrictions")) {
        if (p.process_relations) {
            osm_process_turn_restrictions(&p, suffix);
        }
        if(!p.keep_tmpfiles)
            tempfile_unlink(suffix,"ways_split_index");
    }
    if (p.process_relations && p.process_ways && p.process_nodes
            && start_phase(&p,"processing associated street relations")) {
        struct files_relation_processing *files_relproc = files_relation_processing_new(p.osm.line2poi, suffix);
        p.osm.associated_streets=tempfile(suffix,"associated_streets",0);
        if (p.osm.associated_streets) {

            process_associated_streets(p.osm.associated_streets, files_relproc);

            fclose(p.osm.associated_streets);
            files_relation_processing_destroy(files_relproc, suffix);
            if(!p.keep_tmpfiles) {
                tempfile_unlink(suffix,"associated_streets");
            }
        }
    }
    if (p.process_relations && p.process_ways && p.process_nodes
            && start_phase(&p,"processing house number interpolations")) {
        // OSM house number interpolations are handled like a relation.
        struct files_relation_processing *files_relproc = files_relation_processing_new(p.osm.line2poi, suffix);
        p.osm.house_number_interpolations=tempfile(suffix,"house_number_interpolations",0);
        if (p.osm.house_number_interpolations) {

            process_house_number_interpolations(p.osm.house_number_interpolations, files_relproc);

            fclose(p.osm.house_number_interpolations);
            files_relation_processing_destroy(files_relproc, suffix);
            if(!p.keep_tmpfiles) {
                tempfile_unlink(suffix,"house_number_interpolations");
            }
        }
    }
    if (p.dump == 1 && start_phase(&p,"dumping")) {
        maptool_dump(&p, suffix);
        exit(0);
    }
    if (p.process_relations) {
        filenames[filename_count]="relations";
        referencenames[filename_count++]=NULL;
        filenames[filename_count]="towns_poly";
        referencenames[filename_count++]=NULL;
    }
    if (p.process_ways) {
        filenames[filename_count]="ways_split";
        referencenames[filename_count++]=NULL;
        filenames[filename_count]="coastline_result";
        referencenames[filename_count++]=NULL;
    }
    if (p.process_nodes) {
        filenames[filename_count]="nodes";
        referencenames[filename_count++]=NULL;
        filenames[filename_count]="way2poi_result";
        referencenames[filename_count++]=NULL;
    }
    for (i = suffix_start ; i < suffix_count ; i++) {
        suffix=suffixes[i];
        if (start_phase(&p,"generating tiles")) {
            maptool_load_countries(&p);
            maptool_generate_tiles(&p, suffix, filenames, filename_count, i == suffix_start, suffixes[0]);
            p.tilesdir_loaded=1;
        }
        if (start_phase(&p,"assembling map")) {
            maptool_load_countries(&p);
            maptool_load_tilesdir(&p, suffix);
            maptool_assemble_map(&p, suffix, filenames, referencenames, filename_count, i == suffix_start, i == suffix_count-1,
                                 suffixes[0]);
        }
        phase-=2;
    }
    phase+=2;
    start_phase(&p,"done");
    return 0;
}
