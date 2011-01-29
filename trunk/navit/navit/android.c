#include <string.h>
#include <poll.h>
#include <glib.h>
#include <stdlib.h>
#include "android.h"
#include <android/log.h>
#include "debug.h"
#include "event.h"
#include "callback.h"
#include "projection.h"
#include "map.h"


JNIEnv *jnienv;
jobject *android_activity;
struct callback_list *android_activity_cbl;
int android_version;

struct navit {
        struct attr self;
        GList *mapsets;
        GList *layouts;
        struct gui *gui;
        struct layout *layout_current;
        struct graphics *gra;
        struct action *action;
        struct transformation *trans, *trans_cursor;
        struct compass *compass;
        struct route *route;
        struct navigation *navigation;
        struct speech *speech;
        struct tracking *tracking;
        int ready;
        struct window *win;
        struct displaylist *displaylist;
        int tracking_flag;
        int orientation;
        int recentdest_count;
        int osd_configuration;
        GList *vehicles;
        GList *windows_items;
        struct navit_vehicle *vehicle;
        struct callback_list *attr_cbl;
        struct callback *nav_speech_cb, *roadbook_callback, *popup_callback, *route_cb, *progress_cb;
        struct datawindow *roadbook_window;
        struct map *former_destination;
        struct point pressed, last, current;
        int button_pressed,moved,popped,zoomed;
        int center_timeout;
        int autozoom_secs;
        int autozoom_min;
        int autozoom_active;
        struct event_timeout *button_timeout, *motion_timeout;
        struct callback *motion_timeout_callback;
        int ignore_button;
        int ignore_graphics_events;
        struct log *textfile_debug_log;
        struct pcoord destination;
        int destination_valid;
        int blocked;
        int w,h;
        int drag_bitmap;
        int use_mousewheel;
        struct messagelist *messages;
        struct callback *resize_callback,*button_callback,*motion_callback,*predraw_callback;
        struct vehicleprofile *vehicleprofile;
        GList *vehicleprofiles;
        int pitch;
        int follow_cursor;
        int prevTs;
        int graphics_flags;
        int zoom_min, zoom_max;
        int radius;
        struct bookmarks *bookmarks;
        int flags;
                 /* 1=No graphics ok */
                 /* 2=No gui ok */
        int border;
};


struct navit *global_navit;


struct attr attr;
struct config {
        struct attr **attrs;
        struct callback_list *cbl;
} *config;


int
android_find_class_global(char *name, jclass *ret)
{
	*ret=(*jnienv)->FindClass(jnienv, name);
	if (! *ret) {
		dbg(0,"Failed to get Class %s\n",name);
		return 0;
	}
	(*jnienv)->NewGlobalRef(jnienv, *ret);
	return 1;
}

int
android_find_method(jclass class, char *name, char *args, jmethodID *ret)
{
	*ret = (*jnienv)->GetMethodID(jnienv, class, name, args);
	if (*ret == NULL) {
		dbg(0,"Failed to get Method %s with signature %s\n",name,args);
		return 0;
	}
	return 1;
}

int
android_find_static_method(jclass class, char *name, char *args, jmethodID *ret)
{
	*ret = (*jnienv)->GetStaticMethodID(jnienv, class, name, args);
	if (*ret == NULL) {
		dbg(0,"Failed to get static Method %s with signature %s\n",name,args);
		return 0;
	}
	return 1;
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_Navit_NavitMain( JNIEnv* env, jobject thiz, jobject activity, jobject lang, int version, jobject display_density_string)
{
	char *strings[]={"/data/data/org.navitproject.navit/bin/navit",NULL};
	char *langstr;
	char *displaydensitystr;
	android_version=version;
	__android_log_print(ANDROID_LOG_ERROR,"test","called");
	android_activity_cbl=callback_list_new();
	jnienv=env;
	android_activity=activity;
	(*jnienv)->NewGlobalRef(jnienv, activity);
	langstr=(*env)->GetStringUTFChars(env, lang, NULL);
	dbg(0,"enter env=%p thiz=%p activity=%p lang=%s version=%d\n",env,thiz,activity,langstr,version);
	setenv("LANG",langstr,1);
	(*env)->ReleaseStringUTFChars(env, lang, langstr);

	displaydensitystr=(*env)->GetStringUTFChars(env, display_density_string, NULL);
	dbg(0,"*****displaydensity=%s\n",displaydensitystr);
	setenv("ANDROID_DENSITY",displaydensitystr,1);
	(*env)->ReleaseStringUTFChars(env, display_density_string, displaydensitystr);
	main_real(1, strings);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_Navit_NavitActivity( JNIEnv* env, jobject thiz, int param)
{
	dbg(0,"enter %d\n",param);
	callback_list_call_1(android_activity_cbl, param);
	if (param == -3)
		exit(0);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_SizeChangedCallback( JNIEnv* env, jobject thiz, int id, int w, int h)
{
	dbg(0,"enter %p %d %d\n",(struct callback *)id,w,h);
	if (id)
		callback_call_2((struct callback *)id,w,h);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_ButtonCallback( JNIEnv* env, jobject thiz, int id, int pressed, int button, int x, int y)
{
	dbg(1,"enter %p %d %d\n",(struct callback *)id,pressed,button);
	if (id)
		callback_call_4((struct callback *)id,pressed,button,x,y);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_MotionCallback( JNIEnv* env, jobject thiz, int id, int x, int y)
{
	dbg(1,"enter %p %d %d\n",(struct callback *)id,x,y);
	if (id)
		callback_call_2((struct callback *)id,x,y);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_KeypressCallback( JNIEnv* env, jobject thiz, int id, jobject str)
{
	char *s;
	dbg(0,"enter %p %p\n",(struct callback *)id,str);
	s=(*env)->GetStringUTFChars(env, str, NULL);
	dbg(0,"key=%s\n",s);
	if (id)
		callback_call_1((struct callback *)id,s);
	(*env)->ReleaseStringUTFChars(env, str, s);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitTimeout_TimeoutCallback( JNIEnv* env, jobject thiz, int delete, int id)
{
	dbg(1,"enter %p %d %p\n",thiz, delete, (void *)id);
	callback_call_0((struct callback *)id);
	if (delete) 
		(*jnienv)->DeleteGlobalRef(jnienv, thiz);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitVehicle_VehicleCallback( JNIEnv * env, jobject thiz, int id, jobject location)
{
	callback_call_1((struct callback *)id, (void *)location);
} 

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitIdle_IdleCallback( JNIEnv* env, jobject thiz, int id)
{
	dbg(1,"enter %p %p\n",thiz, (void *)id);
	callback_call_0((struct callback *)id);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitWatch_poll( JNIEnv* env, jobject thiz, int fd, int cond)
{
	struct pollfd pfd;
	pfd.fd=fd;
	dbg(1,"%p poll called for %d %d\n",env, fd, cond);
	switch ((enum event_watch_cond)cond) {
	case event_watch_cond_read:
		pfd.events=POLLIN;
		break;
	case event_watch_cond_write:
		pfd.events=POLLOUT;
		break;
	case event_watch_cond_except:
		pfd.events=POLLERR;
		break;
	default:
		pfd.events=0;
	}
	pfd.revents=0;
	poll(&pfd, 1, -1);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitWatch_WatchCallback( JNIEnv* env, jobject thiz, int id)
{
	dbg(1,"enter %p %p\n",thiz, (void *)id);
	callback_call_0((struct callback *)id);
}


JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitSensors_SensorCallback( JNIEnv* env, jobject thiz, int id, int sensor, float x, float y, float z)
{
	dbg(1,"enter %p %p %f %f %f\n",thiz, (void *)id,x,y,z);
	callback_call_4((struct callback *)id, sensor, &x, &y, &z);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_CallbackMessageChannel( JNIEnv* env, jobject thiz, int i, jobject str)
{
	char *s;
	dbg(0,"enter %p %p\n",(struct callback *)i,str);

	config_get_attr(config, attr_navit, &attr, NULL);
	// attr.u.navit

	if (i)
	{
		if (i == 1)
		{
			// zoom in
			navit_zoom_in_cursor(global_navit, 2);
			// navit_zoom_in_cursor(attr.u.navit, 2);
		}
		else if (i == 2)
		{
			// zoom out
			navit_zoom_out_cursor(global_navit, 2);
			// navit_zoom_out_cursor(attr.u.navit, 2);
		}
		else if (i==5)
		{
			// call a command (like in gui)
			s=(*env)->GetStringUTFChars(env, str, NULL);
			dbg(0,"*****string=%s\n",s);

			command_evaluate(global_navit,s);

			(*env)->ReleaseStringUTFChars(env, str, s);
		}
		else if (i == 4)
		{
			s=(*env)->GetStringUTFChars(env, str, NULL);
			dbg(0,"*****string=%s\n",s);

			// set destination to (pixel-x#pixel-y)
			char name[strlen(s)];
			*name = "Target";
			char *pstr;
			// char *stopstring;

		        struct point p;
			struct coord c;
			struct pcoord pc;

			// pixel-x
			pstr = strtok (s,"#");
			p.x = atoi(pstr);
			// pixel-y
			pstr = strtok (NULL, "#");
			p.y = atoi(pstr);

			dbg(0,"11x=%d\n",p.x);
			dbg(0,"11y=%d\n",p.y);

		        transform_reverse(global_navit->trans, &p, &c);


		        pc.x = c.x;
		        pc.y = c.y;
		        pc.pro = transform_get_projection(global_navit->trans);

			dbg(0,"22x=%d\n",pc.x);
			dbg(0,"22y=%d\n",pc.y);

			// start navigation asynchronous
			navit_set_destination(global_navit, &pc, &name, 1);

			(*env)->ReleaseStringUTFChars(env, str, s);
		}
		else if (i == 3)
		{
			s=(*env)->GetStringUTFChars(env, str, NULL);
			dbg(0,"*****string=%s\n",s);

			// set destination to (lat#lon#title)
			struct coord_geo g;
			char *p;
			char name[strlen(s)];
			char *stopstring;

			// lat
			p = strtok (s,"#");
			g.lat = strtof(p, &stopstring);
			// lon
			p = strtok (NULL, "#");
			g.lng = strtof(p, &stopstring);
			// description
			*name = strtok (NULL, "#");

			dbg(0,"lat=%f\n",g.lat);
			dbg(0,"lng=%f\n",g.lng);
			dbg(0,"str1=%s\n",name);

			struct coord c;
			transform_from_geo(projection_mg, &g, &c);

			struct pcoord pc;
			pc.x=c.x;
			pc.y=c.y;
			pc.pro=projection_mg;

			// start navigation asynchronous
			navit_set_destination(global_navit, &pc, &name, 1);

			(*env)->ReleaseStringUTFChars(env, str, s);
		}
	}
}
