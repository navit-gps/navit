/*
 * CORBA map test
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Elliot Lee <sopwith@redhat.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"


#define ABORT_IF_EXCEPTION(_ev, _message)                    \
if ((_ev)->_major != CORBA_NO_EXCEPTION) {                   \
  g_error("%s: %s", _message, CORBA_exception_id (_ev));     \
  CORBA_exception_free (_ev);                                \
  abort();                                                   \
}

Map map_client, bec;
Mappel mappel;

gboolean map_opt_quiet = FALSE;

int main(int argc, char *argv[])
{
	CORBA_Environment ev;
	CORBA_ORB orb;
	char buf[1024];
	FILE *ior;
	PointObj pnt,src,dst;

	ior=fopen("map.ior","r");
	if (ior) {
		fread(buf,1024,1,ior);
		fclose(ior);
	}
	CORBA_exception_init(&ev);
	orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);

	
	/* bind to object */
	map_client = CORBA_ORB_string_to_object(orb, buf, &ev);
	ABORT_IF_EXCEPTION(&ev, "cannot bind to object");
	g_assert(map_client != NULL);

#if 0
	/* Method call without any argument, usefull to tell
	 * lifeness */
	Map_doNothing(map_client, &ev);
	ABORT_IF_EXCEPTION(&ev, "service raised exception ");

	Map_PlaceFlag(map_client, &ev);
	ABORT_IF_EXCEPTION(&ev, "service raised exception ");
#endif

	if (argc > 0) {
		if (!strcmp(argv[1],"--view")) {
			pnt=Map_PointFromCoord(map_client, argv[2], &ev);
			ABORT_IF_EXCEPTION(&ev, "service raised exception ");
			Map_View(map_client, &pnt, &ev);
			ABORT_IF_EXCEPTION(&ev, "service raised exception ");
		}
		if (!strcmp(argv[1],"--route")) {
			src=Map_PointFromCoord(map_client, argv[2], &ev);
			ABORT_IF_EXCEPTION(&ev, "service raised exception ");
			dst=Map_PointFromCoord(map_client, argv[3], &ev);
			ABORT_IF_EXCEPTION(&ev, "service raised exception ");
			Map_Route(map_client, &src, &dst, &ev);
			ABORT_IF_EXCEPTION(&ev, "service raised exception ");
		}
		mappel=Map_Get(map_client, &ev);
		ABORT_IF_EXCEPTION(&ev, "service raised exception ");
	}

	/* release initial object reference */
	CORBA_Object_release(map_client, &ev);
	ABORT_IF_EXCEPTION(&ev, "service raised exception ");

	/* shutdown ORB, shutdown IO channels */
	CORBA_ORB_shutdown(orb, FALSE, &ev);
	ABORT_IF_EXCEPTION(&ev, "ORB shutdown ...");

	/* destroy local ORB */
	CORBA_ORB_destroy(orb, &ev);
	ABORT_IF_EXCEPTION(&ev, "destroying local ORB raised exception");

	return 0;
}
