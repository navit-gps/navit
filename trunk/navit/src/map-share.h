#ifndef NAVIT_MAP_SHARE_H
#define NAVIT_MAP_SHARE_H

void map_srv_start_poa(CORBA_ORB orb, CORBA_Environment * ev);
CORBA_Object map_srv_start_object(CORBA_Environment * ev, struct container *co);

#endif

