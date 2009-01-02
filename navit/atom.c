#include <glib.h>
#include "atom.h"

static GHashTable *atom_hash;

char *
atom_lookup(char *name)
{
	return g_hash_table_lookup(atom_hash,name);
}

char *
atom(char *name)
{
	char *id=atom_lookup(name);
	if (id)
		return id;
	id=g_strdup(name);
	g_hash_table_insert(atom_hash, id, id);
	return id;
}

void
atom_init(void)
{
	atom_hash=g_hash_table_new(g_str_hash, g_str_equal);
}
