#include <stdio.h>
#include <string.h>
#include <time.h>
#include "maptool.h"

static int print;

static char *types[]= {"node","way","relation"};

struct o5m {
    unsigned char buffer[65536*2];
    int buffer_size;
    unsigned char *buffer_start;
    unsigned char *buffer_end;
    FILE *in;
    int error;

    int lat, lon, uid, version;
    unsigned long long id, rid[3], changeset;
    time_t timestamp;
    char *user;
};

static struct string_table {
    char strings[15000][256];
    int pos;
} st;

static double latlon_scale=10000000.0;

#define buffer_end(o,x) ((o)->buffer_start+(x) > (o)->buffer_end && !fill_buffer((o), (x)))

static int fill_buffer(struct o5m *o, int min) {
    int count;

    memmove(o->buffer, o->buffer_start, o->buffer_end-o->buffer_start);
    o->buffer_end-=o->buffer_start-o->buffer;
    o->buffer_start=o->buffer;
    count=fread(o->buffer_end, 1, o->buffer+o->buffer_size-o->buffer_end, o->in);
    if (!count)
        return 0;
    o->buffer_end+=count;
    return (min <= o->buffer_end - o->buffer);
}

static unsigned long long get_uval(unsigned char **p) {
    unsigned char c;
    unsigned long long ret=0;
    int shift=0;

    for (;;) {
        c=*((*p)++);
        ret+=((unsigned long long)c & 0x7f) << shift;
        if (!(c & 0x80))
            return ret;
        shift+=7;
    }
}

static long long get_sval(unsigned char **p) {
    unsigned long long ret=get_uval(p);
    if (ret & 1) {
        return -((long long)(ret >> 1)+1);
    } else {
        return ret >> 1;
    }
}

static void get_strings(struct string_table *st, unsigned char **p, char **s1, char **s2) {
    int len,xlen=0;
    *s1=(char *)*p;
    len=strlen(*s1);
    xlen=1;
    if (s2) {
        *s2=*s1+len+1;
        len+=strlen(*s2);
        xlen++;
    }
    (*p)+=len+xlen;
    if (len <= 250) {
        memcpy(st->strings[st->pos++], *s1, len+xlen);
        if (st->pos >= 15000)
            st->pos=0;
    }
}

static void get_strings_ref(struct string_table *st, int ref, char **s1, char **s2) {
    int pos=st->pos-ref;

    if (pos < 0)
        pos+=15000;
    *s1=st->strings[pos];
    if (s2)
        *s2=*s1+strlen(*s1)+1;
}

static void print_escaped(char *s) {
    for (;;) {
        switch (*s) {
        case 0:
            return;
        case 9:
        case 13:
        case 34:
        case 38:
        case 39:
        case 60:
        case 62:
            printf("&#%d;",*s);
            break;
        default:
            putc(*s,stdout);
            break;
        }
        s++;
    }
}

static void o5m_reset(struct o5m *o) {
    o->lat=0;
    o->lon=0;
    o->id=0;
    o->rid[0]=0;
    o->rid[1]=0;
    o->rid[2]=0;
    o->changeset=0;
    o->timestamp=0;
}

static void o5m_print_start(struct o5m *o, int c) {
    printf("\t<%s id=\""LONGLONG_FMT"\"",types[c-0x10],o->id);
}

static void o5m_print_version(struct o5m *o, int tags) {
    char timestamp_str[64];
    if (o->version) {
        strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%SZ", gmtime(&o->timestamp));
        printf(" version=\"%d\" timestamp=\"%s\" changeset=\""LONGLONG_FMT"\"",o->version,timestamp_str,o->changeset);
        if (o->uid) {
            printf(" uid=\"%d\" user=\"",o->uid);
            print_escaped(o->user);
            printf("\"");
        }
    }
    printf("%s\n",tags?">":"/>");
}

static void o5m_print_end(char c) {
    printf("\t</%s>\n",types[c-0x10]);
}

int map_collect_data_osm_o5m(FILE *in, struct maptool_osm *osm) {
    struct o5m o;
    unsigned char c, *end, *rend;
    int len, rlen, ref, tags;
    char *uidstr, *role;

    if (print) {
        printf("<?xml version='1.0' encoding='UTF-8'?>\n");
        printf("<osm version=\"0.6\" generator=\"osmconvert 0.1X6\">\n");
    }


    o5m_reset(&o);
    o.buffer_size=sizeof(o.buffer);
    o.buffer_start=o.buffer;
    o.buffer_end=o.buffer;
    o.error=0;
    o.in=in;

    fill_buffer(&o,1);
    for (;;) {
        if (buffer_end(&o, 1)) {
            fprintf(stderr,"unexpected eof\n");
            return 1;
        }
        c=*(o.buffer_start++);
        switch (c) {
        case 0x10:
        case 0x11:
        case 0x12:
            (void)buffer_end(&o, 4);
            len=get_uval(&o.buffer_start);
            if (o.buffer_start > o.buffer_end) {
                fprintf(stderr,"unexpected eof\n");
                return 0;
            }
            if (buffer_end(&o, len)) {
                fprintf(stderr,"unexpected eof or buffer too small, item type %d, item size %d\n", c, len);
                return 0;
            }
            end=o.buffer_start+len;
            o.id+=get_sval(&o.buffer_start);
            o.version=get_uval(&o.buffer_start);
            if (o.version) {
                o.timestamp+=get_sval(&o.buffer_start);
                if (o.timestamp) {
                    o.changeset+=get_sval(&o.buffer_start);
                    ref=get_uval(&o.buffer_start);
                    if (ref)
                        get_strings_ref(&st, ref, &uidstr, &o.user);
                    else
                        get_strings(&st, &o.buffer_start, &uidstr, &o.user);
                    o.uid=get_uval((unsigned char **)&uidstr);
                }
            }
            if (print)
                o5m_print_start(&o, c);
            switch (c) {
            case 0x10:
                o.lon+=get_sval(&o.buffer_start);
                o.lat+=get_sval(&o.buffer_start);
                osm_add_node(o.id, o.lat/latlon_scale,o.lon/latlon_scale);
                tags=end > o.buffer_start;
                if (print) {
                    printf(" lat=\"%.7f\" lon=\"%.7f\"",o.lat/10000000.0,o.lon/10000000.0);
                    o5m_print_version(&o, tags);
                }
                break;
            case 0x11:
                osm_add_way(o.id);
                rlen=get_uval(&o.buffer_start);
                tags=end > o.buffer_start;
                rend=o.buffer_start+rlen;
                if (print)
                    o5m_print_version(&o, tags);
                while (o.buffer_start < rend) {
                    o.rid[0]+=get_sval(&o.buffer_start);
                    osm_add_nd(o.rid[0]);
                    if (print)
                        printf("\t\t<nd ref=\""LONGLONG_FMT"\"/>\n",o.rid[0]);
                }
                break;
            case 0x12:
                osm_add_relation(o.id);
                rlen=get_uval(&o.buffer_start);
                tags=end > o.buffer_start;
                rend=o.buffer_start+rlen;
                if (print)
                    o5m_print_version(&o, tags);
                while (o.buffer_start < rend) {
                    long long delta=get_sval(&o.buffer_start);
                    int r;
                    ref=get_uval(&o.buffer_start);
                    if (ref)
                        get_strings_ref(&st, ref, &role, NULL);
                    else
                        get_strings(&st, &o.buffer_start, &role, NULL);
                    r=role[0]-'0';
                    if (r < 0)
                        r=0;
                    if (r > 2)
                        r=2;
                    o.rid[r]+=delta;
                    osm_add_member(r+1, o.rid[r], role+1);
                    if (print)
                        printf("\t\t<member type=\"%s\" ref=\""LONGLONG_FMT"\" role=\"%s\"/>\n",types[r], o.rid[r], role+1);
                }
                break;
            }
            while (end > o.buffer_start) {
                char *k, *v;
                ref=get_uval(&o.buffer_start);
                if (ref)
                    get_strings_ref(&st, ref, &k, &v);
                else
                    get_strings(&st, &o.buffer_start, &k, &v);
                osm_add_tag(k, v);
                if (print) {
                    printf("\t\t<tag k=\"");
                    print_escaped(k);
                    printf("\" v=\"");
                    print_escaped(v);
                    printf("\"/>\n");
                }
            }
            if (print && tags) {
                o5m_print_end(c);
            }
            switch (c) {
            case 0x10:
                osm_end_node(osm);
                break;
            case 0x11:
                osm_end_way(osm);
                break;
            case 0x12:
                osm_end_relation(osm);
                break;
            }
            break;
        case 0xdb:
            if (print)
                printf("\t<bounds minlat=\"-180.0000000\" minlon=\"-90.0000000\" maxlat=\"180.0000000\" maxlon=\"90.0000000\"/>\n");
            len=get_uval(&o.buffer_start);
            if (o.buffer_start > o.buffer_end) {
                return 0;
            }
            if (buffer_end(&o, len)) {
                return 0;
            }
            o.buffer_start+=len;
            break;
        case 0xe0:
            if (buffer_end(&o, 5))
                return 0;
            o.buffer_start+=5;
            break;
        case 0xfe:
            return 1;
        case 0xff:
            o5m_reset(&o);
            break;
        default:
            fprintf(stderr,"Unknown tag 0x%x\n",c);
        /* Fall through */
        case 0xdc: /* File timestamp: silently ignore it */
            len=get_uval(&o.buffer_start);
            if (o.buffer_start > o.buffer_end) {
                return 0;
            }
            if (buffer_end(&o, len)) {
                return 0;
            }
            o.buffer_start+=len;
            break;
        }
    }
    return 0;
}
