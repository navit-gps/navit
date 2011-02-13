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


#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"
#include "item.h"
#include "map.h"
#include "zipfile.h"
#include "main.h"
#include "config.h"
#include "linguistics.h"
#include "plugin.h"
#include "util.h"
#include "maptool.h"

long long slice_size=1024*1024*1024;
int attr_debug_level=1;
int ignore_unkown = 0;
GHashTable *dedupe_ways_hash;
int phase;
int slices;
struct buffer node_buffer = {
	64*1024*1024,
};

int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;

int overlap=1;

int bytes_read;



void
sig_alrm(int sig)
{
#ifndef _WIN32
	signal(SIGALRM, sig_alrm);
	alarm(30);
#endif
	fprintf(stderr,"PROGRESS%d: Processed %d nodes (%d out) %d ways %d relations %d tiles\n", phase, processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles);
}

void
sig_alrm_end(void)
{
#ifndef _WIN32
	alarm(0);
#endif
}

static struct plugins *plugins;

static void add_plugin(char *path)
{
	struct attr **attrs;

	if (! plugins)
		plugins=plugins_new();
	attrs=(struct attr*[]){&(struct attr){attr_path,{path}},NULL};
	plugin_new(&(struct attr){attr_plugins,.u.plugins=plugins}, attrs);
}

static void
maptool_init(FILE* rule_file)
{
	if (plugins)
		plugins_init(plugins);
	osm_init(rule_file);
}

static void
usage(FILE *f)
{
	/* DEVELOPPERS : don't forget to update the manpage if you modify theses options */
	fprintf(f,"\n");
	fprintf(f,"maptool - parse osm textfile and converts to Navit binfile format\n\n");
	fprintf(f,"Usage :\n");
	fprintf(f,"bzcat planet.osm.bz2 | maptool mymap.bin\n");
	fprintf(f,"Available switches:\n");
	fprintf(f,"-h (--help)              : this screen\n");
	fprintf(f,"-5 (--md5)               : set file where to write md5 sum\n");
	fprintf(f,"-6 (--64bit)             : set zip 64 bit compression\n");
	fprintf(f,"-a (--attr-debug-level)  : control which data is included in the debug attribute\n");
	fprintf(f,"-c (--dump-coordinates)  : dump coordinates after phase 1\n");
#ifdef HAVE_POSTGRESQL
	fprintf(f,"-d (--db)                : get osm data out of a postgresql database with osm simple scheme and given connect string\n");
#endif
	fprintf(f,"-e (--end)               : end at specified phase\n");
	fprintf(f,"-i (--input-file)        : specify the input file name (OSM), overrules default stdin\n");
	fprintf(f,"-k (--keep-tmpfiles)     : do not delete tmp files after processing. useful to reuse them\n\n");
	fprintf(f,"-N (--nodes-only)        : process only nodes\n");
	fprintf(f,"-o (--coverage)          : map every street to item coverage\n");
	fprintf(f,"-P (--protobuf)          : input file is protobuf\n");
	fprintf(f,"-r (--rule-file)         : read mapping rules from specified file\n");
	fprintf(f,"-s (--start)             : start at specified phase\n");
	fprintf(f,"-S (--slice-size)        : defines the amount of memory to use, in bytes. Default is 1GB\n");
	fprintf(f,"-w (--dedupe-ways)       : ensure no duplicate ways or nodes. useful when using several input files\n");
	fprintf(f,"-W (--ways-only)         : process only ways\n");
	fprintf(f,"-z (--compression-level) : set the compression level\n");
	
	exit(1);
}

int main(int argc, char **argv)
{
	FILE *ways=NULL,*ways_split=NULL,*ways_split_index=NULL,*nodes=NULL,*turn_restrictions=NULL,*graph=NULL,*coastline=NULL,*tilesdir,*coords,*relations=NULL,*boundaries=NULL;
	FILE *files[10];
	FILE *references[10];

#if 0
	char *map=g_strdup(attrmap);
#endif
	int zipnum,c,start=1,end=99,dump_coordinates=0;
	int keep_tmpfiles=0;
	int process_nodes=1, process_ways=1, process_relations=1;
#ifdef HAVE_ZLIB
	int compression_level=9;
#else
	int compression_level=0;
#endif
	int zip64=0;
	int output=0;
	int input=0;
	int protobuf=0;
	int f,pos;
	char *result,*optarg_cp,*attr_name,*attr_value;
	char *protobufdb=NULL,*protobufdb_operation=NULL,*md5file=NULL;
#ifdef HAVE_POSTGRESQL
	char *dbstr=NULL;
#endif

	FILE* input_file = stdin;   // input data

	FILE* rule_file = NULL;    // external rule file

	struct attr *attrs[10];
	GList *map_handles=NULL;
	struct map *handle;
#if 0
	char *suffixes[]={"m0l0", "m0l1","m0l2","m0l3","m0l4","m0l5","m0l6"};
	char *suffixes[]={"m","r"};
#else
	char *suffixes[]={""};
#endif
	char *suffix=suffixes[0];

	int suffix_count=sizeof(suffixes)/sizeof(char *);
	int i;
	main_init(argv[0]);
	struct zip_info *zip_info=NULL;
	int suffix_start=0;
	char *timestamp=current_to_iso8601();
#ifndef HAVE_GLIB
	_g_slice_thread_init_nomessage();
#endif

	while (1) {
#if 0
		int this_option_optind = optind ? optind : 1;
#endif
		int option_index = 0;
		static struct option long_options[] = {
			{"md5", 1, 0, '5'},
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
			{"help", 0, 0, 'h'},
			{"keep-tmpfiles", 0, 0, 'k'},
			{"nodes-only", 0, 0, 'N'},
			{"map", 1, 0, 'm'},
			{"plugin", 1, 0, 'p'},
			{"protobuf", 0, 0, 'P'},
			{"start", 1, 0, 's'},
			{"input-file", 1, 0, 'i'},
			{"rule-file", 1, 0, 'r'},
			{"ignore-unknown", 0, 0, 'n'},
			{"ways-only", 0, 0, 'W'},
			{"slice-size", 1, 0, 'S'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "5:6B:DNO:PWS:a:bc"
#ifdef HAVE_POSTGRESQL
					      "d:"
#endif
					      "e:hi:knm:p:r:s:wz:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case '5':
			md5file=optarg;
			break;
		case '6':
			zip64=1;
			break;
		case 'B':
			protobufdb=optarg;
			break;
		case 'D':
			output=1;
			break;
		case 'N':
			process_ways=0;
			break;
		case 'R':
			process_relations=0;
			break;
		case 'O':
			protobufdb_operation=optarg;
			output=1;
			break;
		case 'P':
			protobuf=1;
			break;
		case 'S':
			slice_size=atoll(optarg);
			break;
		case 'W':
			process_nodes=0;
			break;
		case 'a':
			attr_debug_level=atoi(optarg);
			break;
		case 'b':
			input=1;
			break;
		case 'c':
			dump_coordinates=1;
			break;
#ifdef HAVE_POSTGRESQL
		case 'd':
			dbstr=optarg;
			break;
#endif
		case 'e':
			end=atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			break;
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
			map_handles=g_list_append(map_handles,handle);
			break;
		case 'n':
			fprintf(stderr,"I will IGNORE unknown types\n");
			ignore_unkown=1;
			break;
		case 'k':
			fprintf(stderr,"I will KEEP tmp files\n");
			keep_tmpfiles=1;
			break;
		case 'p':
			add_plugin(optarg);
			break;
		case 's':
			start=atoi(optarg);
			break;
		case 'w':
			dedupe_ways_hash=g_hash_table_new(NULL, NULL);
			break;
		case 'i':
			input_file = fopen( optarg, "r" );
			if ( input_file ==  NULL )
			{
			    fprintf( stderr, "\nInput file (%s) not found\n", optarg );
			    exit( -1 );
			}
			break;
		case 'r':
			rule_file = fopen( optarg, "r" );
			if ( rule_file ==  NULL )
			{
			    fprintf( stderr, "\nRule file (%s) not found\n", optarg );
			    exit( -1 );
			}
			break;
#ifdef HAVE_ZLIB
		case 'z':
			compression_level=atoi(optarg);
			break;
#endif
		case '?':
			usage(stderr);
			break;
		default:
			fprintf(stderr,"c=%d\n", c);
		}

	}
	if (optind != argc-(output == 1 ? 0:1))
		usage(stderr);
	result=argv[optind];

    // initialize plugins and OSM mappings
	maptool_init(rule_file);
	if (protobufdb_operation) {
		osm_protobufdb_load(input_file, protobufdb);
		return 0;
	}

    // input from an OSM file
	if (input == 0) {
	if (start == 1) {
		unlink("coords.tmp");
		if (process_ways)
			ways=tempfile(suffix,"ways",1);
		if (process_nodes)
			nodes=tempfile(suffix,"nodes",1);
		if (process_ways && process_nodes)
			turn_restrictions=tempfile(suffix,"turn_restrictions",1);
		if (process_relations)
			boundaries=tempfile(suffix,"boundaries",1);
		phase=1;
		fprintf(stderr,"PROGRESS: Phase 1: collecting data\n");
#ifdef HAVE_POSTGRESQL
		if (dbstr)
			map_collect_data_osm_db(dbstr,ways,nodes,turn_restrictions,boundaries);
		else
#endif
		if (map_handles) {
			GList *l;
			phase1_map(map_handles,ways,nodes);
			l=map_handles;
			while (l) {
				map_destroy(l->data);
				l=g_list_next(l);
			}
		}
		else if (protobuf)
			map_collect_data_osm_protobuf(input_file,ways,nodes,turn_restrictions,boundaries);
		else
			map_collect_data_osm(input_file,ways,nodes,turn_restrictions,boundaries);
		if (slices) {
			fprintf(stderr,"%d slices\n",slices);
			flush_nodes(1);
			for (i = slices-2 ; i>=0 ; i--) {
				fprintf(stderr, "slice %d of %d\n",slices-i-1,slices-1);
				load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
				resolve_ways(ways, NULL);
				save_buffer("coords.tmp",&node_buffer, i*slice_size);
			}
		} else
			save_buffer("coords.tmp",&node_buffer, 0);
		if (ways)
			fclose(ways);
		if (nodes)
			fclose(nodes);
		if (turn_restrictions)
			fclose(turn_restrictions);
		if (boundaries)
			fclose(boundaries);
	}
	if (!slices) {
		if (end == 1 || dump_coordinates)
			flush_nodes(1);
		else
			slices++;
	}
	if (end == 1)
		exit(0);
	if (start == 2) {
		load_buffer("coords.tmp",&node_buffer,0, slice_size);
	}
	if (start <= 2) {
		if (process_ways) {
			ways=tempfile(suffix,"ways",0);
			phase=2;
			fprintf(stderr,"PROGRESS: Phase 2: finding intersections\n");
			for (i = 0 ; i < slices ; i++) {
				int final=(i >= slices-1);
				ways_split=tempfile(suffix,"ways_split",1);
				ways_split_index=final ? tempfile(suffix,"ways_split_index",1) : NULL;
				graph=tempfile(suffix,"graph",1);
				coastline=tempfile(suffix,"coastline",1);
				if (i)
					load_buffer("coords.tmp",&node_buffer, i*slice_size, slice_size);
				map_find_intersections(ways,ways_split,ways_split_index,graph,coastline,final);
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
			if(!keep_tmpfiles)
				tempfile_unlink(suffix,"ways");
			tempfile_unlink(suffix,"ways_to_resolve");
		} else
			fprintf(stderr,"PROGRESS: Skipping Phase 2\n");
	}
	free(node_buffer.base);
	node_buffer.base=NULL;
	node_buffer.malloced=0;
	node_buffer.size=0;
	if (end == 2)
		exit(0);
	} else {
		ways_split=tempfile(suffix,"ways_split",0);
		process_binfile(stdin, ways_split);
		fclose(ways_split);
	}

#if 1
	coastline=tempfile(suffix,"coastline",0);
	if (coastline) {
		ways_split=tempfile(suffix,"ways_split",2);
		fprintf(stderr,"coastline=%p\n",coastline);
		process_coastlines(coastline, ways_split);
		fclose(ways_split);
		fclose(coastline);
	}
#endif
	if (start <= 3) {
		fprintf(stderr,"PROGRESS: Phase 3: sorting countries, generating turn restrictions\n");
		sort_countries(keep_tmpfiles);
		if (process_relations) {
#if 0
			boundaries=tempfile(suffix,"boundaries",0);
			ways_split=tempfile(suffix,"ways_split",0);
			process_boundaries(boundaries, ways_split);
			fclose(boundaries);
			fclose(ways_split);
			exit(0);
#endif
			turn_restrictions=tempfile(suffix,"turn_restrictions",0);
			if (turn_restrictions) {
				relations=tempfile(suffix,"relations",1);
				coords=fopen("coords.tmp","rb");
				ways_split=tempfile(suffix,"ways_split",0);
				ways_split_index=tempfile(suffix,"ways_split_index",0);
				process_turn_restrictions(turn_restrictions,coords,ways_split,ways_split_index,relations);
				fclose(ways_split_index);
				fclose(ways_split);
				fclose(coords);
				fclose(relations);
				fclose(turn_restrictions);
				if(!keep_tmpfiles)
					tempfile_unlink(suffix,"turn_restrictions");
			}
		}
		if(!keep_tmpfiles)
			tempfile_unlink(suffix,"ways_split_index");
	}
	if (end == 3)
		exit(0);
	if (output == 1) {
		fprintf(stderr,"PROGRESS: Phase 4: dumping\n");
		if (process_nodes) {
			nodes=tempfile(suffix,"nodes",0);
			if (nodes) {
				dump(nodes);
				fclose(nodes);
			}
		}
		if (process_ways) {
			ways_split=tempfile(suffix,"ways_split",0);
			if (ways_split) {
				dump(ways_split);
				fclose(ways_split);
			}
		}
		if (process_relations) {
			relations=tempfile(suffix,"relations",0);
			fprintf(stderr,"Relations=%p\n",relations);
			if (relations) {
				dump(relations);
				fclose(relations);
			}
		}
		exit(0);
	}
	for (i = suffix_start ; i < suffix_count ; i++) {
		suffix=suffixes[i];
		if (start <= 4) {
			phase=3;
			if (i == suffix_start) {
				zip_info=zip_new();
				zip_set_zip64(zip_info, zip64);
				zip_set_timestamp(zip_info, timestamp);
			}
			zipnum=zip_get_zipnum(zip_info);
			fprintf(stderr,"PROGRESS: Phase 4: generating tiles %s\n",suffix);
			tilesdir=tempfile(suffix,"tilesdir",1);
			if (!strcmp(suffix,"r")) {
				ch_generate_tiles(suffixes[0],suffix,tilesdir,zip_info);
			} else {
				for (f = 0 ; f < 3 ; f++)
					files[f]=NULL;
				if (process_relations)
					files[0]=tempfile(suffix,"relations",0);
				if (process_ways)
					files[1]=tempfile(suffix,"ways_split",0);
				if (process_nodes)
					files[2]=tempfile(suffix,"nodes",0);
				phase4(files,3,0,suffix,tilesdir,zip_info);
				for (f = 0 ; f < 3 ; f++) {
					if (files[f])
						fclose(files[f]);
				}
			}
			fclose(tilesdir);
			zip_set_zipnum(zip_info,zipnum);
		}
		if (end == 4)
			exit(0);
		if (zip_info) {
			zip_destroy(zip_info);
			zip_info=NULL;
		}
		if (start <= 5) {
			phase=4;
			fprintf(stderr,"PROGRESS: Phase 5: assembling map %s\n",suffix);
			if (i == suffix_start) {
				char *zipdir=tempfile_name("zipdir","");
				char *zipindex=tempfile_name("index","");
				zip_info=zip_new();
				zip_set_zip64(zip_info, zip64);
				zip_set_maxnamelen(zip_info, 14+strlen(suffixes[0]));
				zip_set_compression_level(zip_info, compression_level);
				if (md5file) 
					zip_set_md5(zip_info, 1);
				zip_open(zip_info, result, zipdir, zipindex);	
				index_init(zip_info, 1);
			}
			if (!strcmp(suffix,"r")) {
				ch_assemble_map(suffixes[0],suffix,zip_info);
			} else {
				for (f = 0 ; f < 3 ; f++) {
					files[f]=NULL;
					references[f]=NULL;
				}
				if (process_relations)
					files[0]=tempfile(suffix,"relations",0);
				if (process_ways) {
					files[1]=tempfile(suffix,"ways_split",0);
					references[1]=tempfile(suffix,"ways_split_ref",1);
				}
				if (process_nodes)
					files[2]=tempfile(suffix,"nodes",0);
				fprintf(stderr,"Slice %d\n",i);

				phase5(files,references,3,0,suffix,zip_info);
				for (f = 0 ; f < 3 ; f++) {
					if (files[f])
						fclose(files[f]);
					if (references[f])
						fclose(references[f]);
				}
			}
			if(!keep_tmpfiles) {
				tempfile_unlink(suffix,"relations");
				tempfile_unlink(suffix,"nodes");
				tempfile_unlink(suffix,"ways_split");
				tempfile_unlink(suffix,"ways_split_ref");
				tempfile_unlink(suffix,"coastline");
				tempfile_unlink(suffix,"turn_restrictions");
				tempfile_unlink(suffix,"graph");
				tempfile_unlink(suffix,"tilesdir");
				tempfile_unlink(suffix,"boundaries");
				unlink("coords.tmp");
			}
			if (i == suffix_count-1) {
				unsigned char md5_data[16];
				zipnum=zip_get_zipnum(zip_info);
				add_aux_tiles("auxtiles.txt", zip_info);
				write_countrydir(zip_info);
				zip_set_zipnum(zip_info, zipnum);
				write_aux_tiles(zip_info);
				zip_write_index(zip_info);
				zip_write_directory(zip_info);
				zip_close(zip_info);
				if (md5file && zip_get_md5(zip_info, md5_data)) {
					FILE *md5=fopen(md5file,"w");
					int i;
					for (i = 0 ; i < 16 ; i++)
						fprintf(md5,"%02x",md5_data[i]);
					fprintf(md5,"\n");
					fclose(md5);
				}
				if (!keep_tmpfiles) {
					remove_countryfiles();
					tempfile_unlink("index","");
					tempfile_unlink("zipdir","");
				}
			}
		}
	}
	return 0;
}
