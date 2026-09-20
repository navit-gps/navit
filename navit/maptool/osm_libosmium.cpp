/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2026 Navit Team
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

/*
 * libosmium-based OSM input frontend. Replaces the hand-rolled XML, o5m
 * and protobuf parsers: reads any libosmium-supported format (osm, osm.gz,
 * osm.bz2, pbf, o5m, ...) and feeds the osm_add_* / osm_end_* callbacks
 * in osm.c, which do all the Navit-specific work.
 */

#include <cstdio>
#include <cstring>
#include <exception>

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/visitor.hpp>

/* Callbacks from osm.c / maptool.c. Declared here instead of including
 * maptool.h, which is not C++-clean. Signatures must match maptool.h. */
extern "C" {
typedef unsigned long long int osmid;
struct maptool_osm;

enum relation_member_type {
    UNUSED,
    rel_member_node,
    rel_member_way,
    rel_member_relation,
};

void osm_add_node(osmid id, double lat, double lon);
void osm_add_tag(char *k, char *v);
void osm_add_way(osmid id);
void osm_add_nd(osmid ref);
void osm_add_relation(osmid id);
void osm_add_member(enum relation_member_type type, osmid ref, char *role);
void osm_end_node(struct maptool_osm *osm);
void osm_end_way(struct maptool_osm *osm);
void osm_end_relation(struct maptool_osm *osm);
void sig_alrm(int sig);
void sig_alrm_end(void);
extern int processed_nodes, processed_ways, processed_relations;

int map_collect_data_osmium(const char *filename, const char *format, struct maptool_osm *osm);
}

namespace {

/* matches BUFFER_SIZE in maptool.h; the old XML parser had the same limit */
const int buffer_size = 1280;

/* osm_add_tag/osm_add_member take char*, so hand them mutable copies */
void copy_str(char *dst, const char *src) {
    snprintf(dst, buffer_size, "%s", src);
}

void add_tags(const osmium::TagList &tags) {
    char k[buffer_size], v[buffer_size];
    for (const auto &tag : tags) {
        copy_str(k, tag.key());
        copy_str(v, tag.value());
        osm_add_tag(k, v);
    }
}

class NavitHandler : public osmium::handler::Handler {
    struct maptool_osm *osm;

  public:
    explicit NavitHandler(struct maptool_osm *osm) : osm(osm) {
    }

    void node(const osmium::Node &n) {
        if (!n.location().valid()) {
            fprintf(stderr, "WARNING: node %lld has no location, skipping\n", (long long)n.id());
            return;
        }
        osm_add_node(n.id(), n.location().lat(), n.location().lon());
        processed_nodes++;
        add_tags(n.tags());
        osm_end_node(osm);
    }

    void way(const osmium::Way &w) {
        osm_add_way(w.id());
        processed_ways++;
        add_tags(w.tags());
        for (const auto &nr : w.nodes())
            osm_add_nd(nr.ref());
        osm_end_way(osm);
    }

    void relation(const osmium::Relation &r) {
        char role[buffer_size];
        osm_add_relation(r.id());
        processed_relations++;
        add_tags(r.tags());
        for (const auto &member : r.members()) {
            enum relation_member_type type;
            switch (member.type()) {
            case osmium::item_type::node:
                type = rel_member_node;
                break;
            case osmium::item_type::way:
                type = rel_member_way;
                break;
            case osmium::item_type::relation:
                type = rel_member_relation;
                break;
            default:
                continue;
            }
            copy_str(role, member.role());
            osm_add_member(type, member.ref(), role);
        }
        osm_end_relation(osm);
    }
};

}  // namespace

/**
 * Read OSM data and feed it through the osm_add_* callbacks.
 *
 * @param filename input file; NULL, "" or "-" reads stdin
 * @param format libosmium format hint ("pbf", "o5m", "osm", ...); NULL or ""
 *        detects the format from the filename suffix, falling back to XML
 * @param osm output tempfiles
 * @return 1 on success, -1 on error
 */
int map_collect_data_osmium(const char *filename, const char *format, struct maptool_osm *osm) {
    try {
        osmium::io::File file;
        if (!filename || !*filename || !strcmp(filename, "-")) {
            file = osmium::io::File("-", (format && *format) ? format : "osm");
        } else {
            file = osmium::io::File(filename, format ? format : "");
            if (file.format() == osmium::io::file_format::unknown)
                file = osmium::io::File(filename, "osm");
        }
        osmium::io::Reader reader(file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way
                                            | osmium::osm_entity_bits::relation);
        NavitHandler handler(osm);
        sig_alrm(0);
        osmium::apply(reader, handler);
        reader.close();
        sig_alrm(0);
        sig_alrm_end();
        return 1;
    } catch (const std::exception &e) {
        fprintf(stderr, "FATAL: failed to read OSM data: %s\n", e.what());
        return -1;
    }
}
