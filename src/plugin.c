#include <stdio.h>
#include <dlfcn.h>
#include "plugin.h"
#define PLUGIN_C
#include "plugin.h"

void
plugin_load(void)
{
	char *plugin="plugins/poi_geodownload/plugin_poi_geodownload.so";
	void *h=dlopen(plugin,RTLD_LAZY);
	void (*init)(void);

	if (! h) {
		/* printf("can't load '%s', Error '%s'\n", plugin, dlerror()); */
	} else {
		init=dlsym(h,"plugin_init");
		if (init) 
			(*init)();
	}
}
