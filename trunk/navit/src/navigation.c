#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "coord.h"
#include "graphics.h"
#include "param.h"
#include "block.h"
#include "route.h"
#include "street.h"
#include "street_name.h"
#include "speech.h"
#include "navigation.h"
#include "data_window.h"
#include "osd.h"

struct data_window *navigation_window;


struct navigation_item {
	char name1[128];
	char name2[128];
	int length;
	int time;
	int crossings_start;
	int crossings_end;
	int angle_start;
	int angle_end;
	int points;	
	struct coord start;
	struct coord end;
};

static int
road_angle(struct coord *c, int dir)
{
	double angle;
	int dx=c[1].x-c[0].x;
	int dy=c[1].y-c[0].y;
	angle=atan2(dx,dy);
	angle*=180/M_PI;
	if (dir == -1)
		angle=angle-180;
	if (angle < 0)
		angle+=360;
	return angle;
}

static void
expand_str(char *str)
{
	int len=strlen(str);
	if (len > 4 && !strcmp(str+len-4,"str.")) 
		strcpy(str+len-4,"strasse");
	if (len > 4 && !strcmp(str+len-4,"Str.")) 
		strcpy(str+len-4,"Strasse");
}

static void
navigation_goto(struct data_window *navigation_window, char **cols)
{
	extern struct container *co;
	long x,y;

	printf("goto %s\n",cols[8]);
	sscanf(cols[8],"%lx,%lx",&x,&y);
	graphics_set_view(co, &x, &y, NULL);
}

static int
is_same_street(struct navigation_item *old, struct navigation_item *new)
{
	if (strlen(old->name2) && !strcmp(old->name2, new->name2)) {
		strcpy(old->name1, new->name1);
		return 1;
	}
	if (strlen(old->name1) && !strcmp(old->name1, new->name1)) {
		strcpy(old->name2, new->name2);
		return 1;
	}
	return 0;
}

static int
maneuver_required(struct navigation_item *old, struct navigation_item *new, int *delta)
{
	if (is_same_street(old, new)) 
		return 0;
	if (old->crossings_end == 2)
		return 0;
	*delta=new->angle_start-old->angle_end;
	if (*delta < -180)
		*delta+=360;
	if (*delta > 180)
		*delta-=360;
	if (*delta < 20 && *delta >-20)
		return 0;
	return 1;
}

int flag,old_level;
extern void *speech_handle;

static void
get_distance(char *dst, int dist)
{
	if (dist < 100) {
		dist=(dist+5)/10;
		dist*=10;
		sprintf(dst,gettext("%d meter"), dist);
		return;
	}
	if (dist < 250) {
		dist=(dist+13)/25;
		dist*=25;
		sprintf(dst,gettext("%d meter"), dist);
		return;
	}
	if (dist < 500) {
		dist=(dist+25)/50;
		dist*=50;
		sprintf(dst,gettext("%d meter"), dist);
		return;
	}
	if (dist < 1000) {
		dist=(dist+50)/100;
		dist*=100;
		sprintf(dst,gettext("%d meter"), dist);
		return;
	}
	if (dist < 5000) {
		dist=(dist+50)/100;
		if (dist % 10) 
			sprintf(dst,gettext("%d,%d kilometer"), dist/10,dist%10);
		else
			sprintf(dst,gettext("%d kilometer"), dist/10);
		return;
	}
	if (dist < 100000) {
		dist=(dist+500)/1000;
		sprintf(dst,gettext("%d kilometer"), dist);
		return;
	}
	dist=(dist+5000)/10000;
	dist*=10;
	sprintf(dst,gettext("%d kilometer"), dist);
}

static void
make_maneuver(struct navigation_item *old, struct navigation_item *new)
{
	
	int delta,navmode=1,add_dir,level;
	struct param_list param_list[20];
	
	char roadname[256];

	char angle_old[30],angle_new[30],angle_delta[30],cross_roads[30],position[30],distance[30];
	char command[256],*p,*dir,*strength;
	char dist[256];

	param_list[0].name="Name1 Old";
	param_list[0].value=old->name1;
	param_list[1].name="Name2 Old";
	param_list[1].value=old->name2;
	param_list[2].name="Name1 New";
	param_list[2].value=new->name1;
	param_list[3].name="Name2 New";
	param_list[3].value=new->name2;
	param_list[4].name="Angle\nOld";
	param_list[5].name="Angle\nNew";
	param_list[6].name="Delta";
	param_list[7].name="X-\nRoads";
	param_list[8].name="Position";
	param_list[9].name="Dist";
	param_list[10].name="Command";
	if (old->points) {
		if (!maneuver_required(old, new, &delta)) {
			old->length+=new->length;
			old->time+=new->time;
			old->crossings_end=new->crossings_end;
			old->angle_end=new->angle_end;
			old->points+=new->points;
		} else {	
			sprintf(angle_old,"%d", old->angle_end);
			param_list[4].value=angle_old;
			sprintf(angle_new,"%d", new->angle_start);
			param_list[5].value=angle_new;
			sprintf(angle_delta,"%d", delta);
			param_list[6].value=angle_delta;
			sprintf(cross_roads,"%d", old->crossings_end);
			param_list[7].value=cross_roads;
			sprintf(position,"0x%lx,0x%lx", new->start.x, new->start.y);
			param_list[8].value=position;
			sprintf(distance,"%d", old->length);
			param_list[9].value=distance;
			add_dir=1;
			dir=gettext("rechts");
			if (delta < 0) {
				dir=gettext("links");
				delta=-delta;
			}
			if (delta < 45) {
				strength=gettext("leicht ");
			} else if (delta < 105) {
				strength="";
			} else if (delta < 165) {
				strength=gettext("scharf ");
			} else {
#ifdef DEBUG
				printf("delta=%d\n", delta);
#endif
				strength=gettext("unbekannt ");
			}
			level=0;
			if (navmode) {
				if (old->length < 20) {
					level=1;
					sprintf(command,gettext("Jetzt "));
				} else if (old->length <= 200) {
					level=2;
					get_distance(dist, old->length);
					sprintf(command,gettext("In %sn "), dist);
				} else if (old->length <= 500) {
					level=3;
					sprintf(command,gettext("In Kürze "));
				} else {
					level=4;
					get_distance(dist, old->length);
					sprintf(command,gettext("Dem Strassenverlauf %s folgen"), dist);
					add_dir=0;
				}
			} else {
				sprintf(command,gettext("Dem Strassenverlauf %d Meter folgen, dann "), old->length);
				add_dir=1;
			}
			if (add_dir) {
/*				p=command+strlen(command);
				strcpy(p,strength);
				p+=strlen(p);
				strcpy(p,dir);
				p+=strlen(p);
				strcpy(p,gettext(" abbiegen"));*/
				sprintf(command,gettext(" %s %s abbiegen"),strength,dir);
				p=command+strlen(command);
			}
			param_list[10].value=command;
			if (flag) {
				sprintf(roadname,"%s %s",old->name1,old->name2);
				osd_set_next_command(command,roadname);
				if (level != old_level) {
					printf("command='%s'\n", command);
					if (speech_handle)
						speech_say(speech_handle, command);
					old_level=level;
				}
				flag=0;
			}
			data_window_add(navigation_window, param_list, 11);
			*old=*new;
		}
	} else {
		*old=*new;
	}
}

void
navigation_path_description(void *route)
{
	int id;
	struct route_path_segment *curr=route_path_get_all(route);
	struct block_info blk_inf;
	struct street_str *str;
	struct street_name name;
	struct street_coord *coord;
	struct map_data *mdata=route_mapdata_get(route);
	struct coord *start,*end,*tmp;
	int angle_start, angle_end, angle_tmp;
	struct route_crossings *crossings_start,*crossings_end;
	struct navigation_item item_curr,item_last;

	memset(&item_last,0,sizeof(item_last));

	if (!navigation_window)
		navigation_window=data_window("Navigation",NULL,navigation_goto);
	data_window_begin(navigation_window);
	flag=1;
	while (curr) {
		str=NULL;
		id=curr->segid;
		if (id) {
			if (id < 0)
				id=-id;
			street_get_by_id(mdata, curr->country, id, &blk_inf, &str);
			coord=street_coord_get(&blk_inf, str);
			start=coord->c;
			end=coord->c+coord->count-1;
			angle_start=road_angle(coord->c, curr->dir);
			if (coord->count > 2)
				angle_end=road_angle(coord->c+coord->count-2,curr->dir);
			else
				angle_end=angle_start;
			if (curr->dir < 0) {
				tmp=start;
				angle_tmp=angle_start;
				start=end;
				angle_start=angle_end;
				end=tmp;
				angle_end=angle_tmp;
			}
			crossings_start=route_crossings_get(route, start);
			crossings_end=route_crossings_get(route, end);
			if (str && str->nameid) {
                		street_name_get_by_id(&name, blk_inf.mdata, str->nameid);
				strcpy(item_curr.name1,name.name1);
				strcpy(item_curr.name2,name.name2);
				expand_str(item_curr.name1);
				expand_str(item_curr.name2);
			} else {
				item_curr.name1[0]='\0';
				item_curr.name2[0]='\0';
			}
			item_curr.length=curr->length;
			item_curr.time=curr->time;
			item_curr.crossings_start=crossings_start->count;
			item_curr.crossings_end=crossings_end->count;
			item_curr.angle_start=angle_start;
			item_curr.angle_end=angle_end;
			item_curr.points=coord->count;
			item_curr.start=*start;
			item_curr.end=*end;
			make_maneuver(&item_last,&item_curr);
			free(coord);
			free(crossings_start);
			free(crossings_end);
		}
		curr=curr->next;
	}
	data_window_end(navigation_window);
}

