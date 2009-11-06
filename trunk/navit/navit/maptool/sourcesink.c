#include <stdio.h>
#include <glib.h>
#include <string.h>
#include "coord.h"
#include "item.h"
#include "attr.h"
#include "maptool.h"

struct item_bin_sink *
item_bin_sink_new(void)
{
	struct item_bin_sink *ret=g_new0(struct item_bin_sink, 1);

	return ret;
}

struct item_bin_sink_func *
item_bin_sink_func_new(int (*func)(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data))
{
	struct item_bin_sink_func *ret=g_new0(struct item_bin_sink_func, 1);
	ret->func=func;
	return ret;
}

void
item_bin_sink_func_destroy(struct item_bin_sink_func *func)
{
	g_free(func);
}

void
item_bin_sink_add_func(struct item_bin_sink *sink, struct item_bin_sink_func *func)
{
	sink->sink_funcs=g_list_append(sink->sink_funcs, func);
}

void
item_bin_sink_destroy(struct item_bin_sink *sink)
{
	/* g_list_foreach(sink->sink_funcs, (GFunc)g_free, NULL); */
	g_list_free(sink->sink_funcs);
	g_free(sink);
}

int
item_bin_write_to_sink(struct item_bin *ib, struct item_bin_sink *sink, struct tile_data *tile_data)
{
	GList *list=sink->sink_funcs;
	int ret=0;
	while (list) {
		struct item_bin_sink_func *func=list->data;
		ret=func->func(func, ib, tile_data);
		if (ret)
			break;
		list=g_list_next(list);
	}
	return ret;
}

struct item_bin_sink *
file_reader_new(FILE *in, int limit, int offset)
{
	struct item_bin_sink *ret;
	if (!in)
		return NULL;
	ret=item_bin_sink_new();
	ret->priv_data[0]=in;
	ret->priv_data[1]=(void *)(long)limit;
	ret->priv_data[2]=(void *)(long)offset;
	fseek(in, 0, SEEK_SET);
	return ret;
}

int
file_reader_finish(struct item_bin_sink *sink)
{
	struct item_bin *ib=(struct item_bin *)buffer;
	int ret =0;
	FILE *in=sink->priv_data[0];
	int limit=(int)(long)sink->priv_data[1];
	int offset=(int)(long)sink->priv_data[2];
	for (;;) {
		switch (item_bin_read(ib, in)) {
		case 0:
			item_bin_sink_destroy(sink);
			return 0;
		case 2:
			if (offset > 0) {
				offset--;
			} else {
				ret=item_bin_write_to_sink(ib, sink, NULL);
				if (ret || (limit != -1 && !--limit)) {
					item_bin_sink_destroy(sink);
					return ret;
				}
			}
		default:
			continue;
		}
	}
}

int
file_writer_process(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data)
{
	FILE *out=func->priv_data[0];
	item_bin_write(ib, out);
	return 0;
}

struct item_bin_sink_func *
file_writer_new(FILE *out)
{
	struct item_bin_sink_func *file_writer;
	if (!out)
		return NULL;
	file_writer=item_bin_sink_func_new(file_writer_process);
	file_writer->priv_data[0]=out;
	return file_writer;
}

int
file_writer_finish(struct item_bin_sink_func *file_writer)
{
	item_bin_sink_func_destroy(file_writer);
	return 0;
}


int
tile_collector_process(struct item_bin_sink_func *tile_collector, struct item_bin *ib, struct tile_data *tile_data)
{
	int *buffer,*buffer2;
	int len=ib->len+1;
	GHashTable *hash=tile_collector->priv_data[0];
	buffer=g_hash_table_lookup(hash, tile_data->buffer);
	buffer2=g_malloc((len+(buffer ? buffer[0] : 1))*4);
	if (buffer) {
		memcpy(buffer2, buffer, buffer[0]*4);
	} else 
		buffer2[0]=1;
	memcpy(buffer2+buffer2[0], ib, len*4);
	buffer2[0]+=len;
	g_hash_table_insert(hash, g_strdup(tile_data->buffer), buffer2);
	return 0;
}

struct item_bin_sink_func *
tile_collector_new(struct item_bin_sink *out)
{
	struct item_bin_sink_func *tile_collector;
	tile_collector=item_bin_sink_func_new(tile_collector_process);
	tile_collector->priv_data[0]=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	tile_collector->priv_data[1]=out;
	return tile_collector;
}

