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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "maptool.h"
#include "debug.h"
#include "linguistics.h"
#include "file.h"
#ifdef HAVE_POSTGRESQL
#include <postgresql/libpq-fe.h>

int map_collect_data_osm_db(char *dbstr, struct maptool_osm *osm) {
    PGconn *conn;
    PGresult *res;
    char query[256];

    sig_alrm(0);
    conn=PQconnectdb(dbstr);
    if (! conn) {
        fprintf(stderr,"Failed to connect to database with '%s'\n",dbstr);
        exit(1);
    }
    fprintf(stderr,"connected to database with '%s'\n",dbstr);
    res=PQexec(conn, "begin");
    if (! res) {
        fprintf(stderr, "Cannot begin transaction: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }
    res=PQexec(conn, "set transaction isolation level serializable");
    if (! res) {
        fprintf(stderr, "Cannot set isolation level: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }
    res=PQexec(conn, "declare nodes cursor for select id, ST_Y(geom), ST_X(geom) from nodes order by id");
    if (! res) {
        fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }
    res=PQexec(conn, "declare ways cursor for select id from ways order by id");
    if (! res) {
        fprintf(stderr, "Cannot setup cursor for ways: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }
    res=PQexec(conn, "declare relations cursor for select id from relations order by id");
    if (! res) {
        fprintf(stderr, "Cannot setup cursor for relations: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }

    for (;;) {
        int j=0, count=0;
        long min, max, id, tag_id;
        PGresult *node, *tag;
        node=PQexec(conn, "fetch 100000 from nodes");
        if (! node) {
            fprintf(stderr, "Cannot setup cursor for nodes: %s\n", PQerrorMessage(conn));
            PQclear(node);
            exit(1);
        }
        count=PQntuples(node);
        fprintf(stderr, "fetch got %i nodes\n", count);
        if (! count)
            break;
        min=atol(PQgetvalue(node, 0, 0));
        max=atol(PQgetvalue(node, count-1, 0));
        sprintf(query,"select node_id,k,v from node_tags where node_id >= %ld and node_id <= %ld order by node_id", min, max);
        tag=PQexec(conn, query);
        if (! tag) {
            fprintf(stderr, "Cannot query node_tag: %s\n", PQerrorMessage(conn));
            exit(1);
        }
        fprintf(stderr, "query node_tag got : %i tuples\n", PQntuples(tag));
        for (int i = 0 ; i < count ; i++) {
            id=atol(PQgetvalue(node, i, 0));
            osm_add_node(id, atof(PQgetvalue(node, i, 1)), atof(PQgetvalue(node, i, 2)));
            processed_nodes++;
            while (j < PQntuples(tag)) {
                tag_id=atol(PQgetvalue(tag, j, 0));
                if (tag_id == id) {
                    osm_add_tag(PQgetvalue(tag, j, 1), PQgetvalue(tag, j, 2));
                    j++;
                }
                if (tag_id < id)
                    j++;
                if (tag_id > id)
                    break;
            }
            osm_end_node(osm);
        }
        PQclear(tag);
        PQclear(node);
    }

    for (;;) {
        int j=0, k=0, count=0, tagged=0;
        long min, max, id, tag_id, node_id;
        PGresult *node,*way,*tag;
        way=PQexec(conn, "fetch 25000 from ways");
        if (! way) {
            fprintf(stderr, "Cannot setup cursor for ways: %s\n", PQerrorMessage(conn));
            PQclear(way);
            exit(1);
        }
        count=PQntuples(way);
        fprintf(stderr, "fetch got %i ways\n", count);
        if (! count)
            break;
        min=atol(PQgetvalue(way, 0, 0));
        max=atol(PQgetvalue(way, count-1, 0));
        fprintf(stderr, "continue with %i ways\n", count);
        sprintf(query,"select way_id,node_id from way_nodes where way_id >= %ld and way_id <= %ld order by way_id,sequence_id",
                min, max);
        node=PQexec(conn, query);
        if (! node) {
            fprintf(stderr, "Cannot query way_node: %s\n", PQerrorMessage(conn));
            exit(1);
        }
        sprintf(query,"select way_id,k,v from way_tags where way_id >= %ld and way_id <= %ld order by way_id", min, max);
        tag=PQexec(conn, query);
        if (! tag) {
            fprintf(stderr, "Cannot query way_tag: %s\n", PQerrorMessage(conn));
            exit(1);
        }
        for (int i = 0 ; i < count ; i++) {
            id=atol(PQgetvalue(way, i, 0));
            osm_add_way(id);
            tagged=0;
            processed_ways++;
            while (k < PQntuples(node)) {
                node_id=atol(PQgetvalue(node, k, 0));
                if (node_id == id) {
                    osm_add_nd(atoll(PQgetvalue(node, k, 1)));
                    tagged=1;
                    k++;
                }
                if (node_id < id)
                    k++;
                if (node_id > id)
                    break;
            }
            while (j < PQntuples(tag)) {
                tag_id=atol(PQgetvalue(tag, j, 0));
                if (tag_id == id) {
                    osm_add_tag(PQgetvalue(tag, j, 1), PQgetvalue(tag, j, 2));
                    tagged=1;
                    j++;
                }
                if (tag_id < id)
                    j++;
                if (tag_id > id)
                    break;
            }
            if (tagged)
                osm_end_way(osm);
        }
        PQclear(tag);
        PQclear(node);
        PQclear(way);
    }

    for (;;) {
        int j=0, k=0, count=0, tagged=0;
        long min, max, id;
        PGresult *tag, *relation, *member;
        relation=PQexec(conn, "fetch 40000 from relations");
        if (! relation) {
            fprintf(stderr, "Cannot setup cursor for relations: %s\n", PQerrorMessage(conn));
            PQclear(relation);
            exit(1);
        }
        count=PQntuples(relation);
        fprintf(stderr, "Got %i relations\n", count);
        if (! count)
            break;
        min=atol(PQgetvalue(relation, 0, 0));
        max=atol(PQgetvalue(relation, count-1, 0));
        sprintf(query,
                "select relation_id,k,v from relation_tags where relation_id >= %ld and relation_id <= %ld order by relation_id", min,
                max);
        tag=PQexec(conn, query);
        if (! tag) {
            fprintf(stderr, "Cannot query relation_tag: %s\n", PQerrorMessage(conn));
            exit(1);
        }
        sprintf(query,
                "select relation_id, member_id, member_type, member_role from relation_members where relation_id >= %ld and relation_id <= %ld order by relation_id, sequence_id",
                min, max);
        member=PQexec(conn, query);
        if (! member) {
            fprintf(stderr, "Cannot query relation_members: %s\n", PQerrorMessage(conn));
            exit(1);
        }
        for (int i = 0 ; i < count ; i++) {
            id=atol(PQgetvalue(relation, i, 0));
            osm_add_relation(id);
            tagged = 0;
            while (j < PQntuples(tag)) {
                long tag_relation_id=atol(PQgetvalue(tag, j, 0));
                if (tag_relation_id == id) {
                    osm_add_tag(PQgetvalue(tag, j, 1), PQgetvalue(tag, j, 2));
                    tagged=1;
                    j++;
                }
                if (tag_relation_id < id)
                    j++;
                if (tag_relation_id > id)
                    break;
            }
            while (k < PQntuples(member)) {
                long member_relation_id=atol(PQgetvalue(member, k, 0));
                if (member_relation_id == id) {
                    int relmember_type=0; //type unknown
                    if (!strcmp(PQgetvalue(member,k, 2),"W")) {
                        relmember_type=2;
                    } else {
                        if (!strcmp(PQgetvalue(member,k, 2),"N")) {
                            relmember_type=1;
                        } else {
                            if (!strcmp(PQgetvalue(member,k, 2),"R")) {
                                relmember_type=3;
                            }
                        }
                    }
                    osm_add_member(relmember_type,atoll(PQgetvalue(member,k, 1)),PQgetvalue(member,k, 3));
                    k++;
                }
                if (member_relation_id < id)
                    k++;
                if (member_relation_id > id)
                    break;
            }
            if (tagged)
                osm_end_relation(osm);
        }
        PQclear(relation);
        PQclear(member);
        PQclear(tag);
    }
    res=PQexec(conn, "commit");
    if (! res) {
        fprintf(stderr, "Cannot commit transaction: %s\n", PQerrorMessage(conn));
        PQclear(res);
        exit(1);
    }
    sig_alrm(0);
    sig_alrm_end();
    return 1;
}
#endif
