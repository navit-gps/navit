#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <glib.h>
#include "android.h"
#include <android/log.h>
#include "navit.h"
#include "config_.h"
#include "command.h"
#include "debug.h"
#include "event.h"
#include "callback.h"
#include "country.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "navit_nls.h"
#include "transform.h"
#include "color.h"
#include "types.h"
#include "search.h"


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

struct attr attr;

struct config {
	struct attr **attrs;
	struct callback_list *cbl;
} *config;


struct gui_config_settings {
	int dummy;
};

struct gui_internal_data {
	int dummy;
};

struct route_data {
	int dummy;
};

struct widget {
	int dummy;
};

struct gui_priv {
	struct navit *nav;
	struct attr self;
	struct window *win;
	struct graphics *gra;
	struct graphics_gc *background;
	struct graphics_gc *background2;
	struct graphics_gc *highlight_background;
	struct graphics_gc *foreground;
	struct graphics_gc *text_foreground;
	struct graphics_gc *text_background;
	struct color background_color, background2_color, text_foreground_color, text_background_color;
	int spacing;
	int font_size;
	int fullscreen;
	struct graphics_font *fonts[3];
	/**
	 * The size (in pixels) that xs style icons should be scaled to.
	 * This icon size can be too small to click it on some devices.
	 */
	int icon_xs;
	/**
	 * The size (in pixels) that s style icons (small) should be scaled to
	 */
	int icon_s;
	/**
	 * The size (in pixels) that l style icons should be scaled to
	 */
	int icon_l;
	int pressed;
	struct widget *widgets;
	int widgets_count;
	int redraw;
	struct widget root;
	struct widget *highlighted,*editable;
	struct widget *highlighted_menu;
	int clickp_valid, vehicle_valid;
	struct pcoord clickp, vehiclep;
	struct attr *click_coord_geo, *position_coord_geo;
	struct search_list *sl;
	int ignore_button;
	int menu_on_map_click;
	int signal_on_map_click;
	char *country_iso2;
	int speech;
	int keyboard;
	int keyboard_required;
	/**
	 * The setting information read from the configuration file.
	 * values of -1 indicate no value was specified in the config file.
	 */
        struct gui_config_settings config;
	struct event_idle *idle;
	struct callback *motion_cb,*button_cb,*resize_cb,*keypress_cb,*window_closed_cb,*idle_cb, *motion_timeout_callback;
	struct event_timeout *motion_timeout_event;
	struct point current;

	struct callback * vehicle_cb;
	  /**
	   * Stores information about the route.
	   */
	struct route_data route_data;

	struct gui_internal_data data;
	struct callback_list *cbl;
	int flags;
	int cols;
	struct attr osd_configuration;
	int pitch;
	int flags_town,flags_street,flags_house_number;
	int radius;
/* html */
	char *html_text;
	int html_depth;
	struct widget *html_container;
	int html_skip;
	char *html_anchor;
	char *href;
	int html_anchor_found;
	struct form *form;
	struct html {
		int skip;
		enum html_tag {
			html_tag_none,
			html_tag_a,
			html_tag_h1,
			html_tag_html,
			html_tag_img,
			html_tag_script,
			html_tag_form,
			html_tag_input,
			html_tag_div,
		} tag;
		char *command;
		char *name;
		char *href;
		char *refresh_cond;
		struct widget *w;
		struct widget *container;
	} html[10];
};

extern struct navit *global_navit;

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
	const char *langstr;
	const char *displaydensitystr;
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
	const char *s;
	dbg(0,"enter %p %p\n",(struct callback *)id,str);
	s=(*env)->GetStringUTFChars(env, str, NULL);
	dbg(0,"key=%d",s);
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

void
android_return_search_result(struct jni_object *jni_o, char *str)
{
	jstring js2 = NULL;
	JNIEnv* env2;
	env2=jni_o->env;
	js2 = (*env2)->NewStringUTF(jni_o->env,str);
	(*env2)->CallVoidMethod(jni_o->env, jni_o->jo, jni_o->jm, js2);
	(*env2)->DeleteLocalRef(jni_o->env, js2);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_CallbackSearchResultList( JNIEnv* env, jobject thiz, int partial, jobject str)
{
	const char *search_string =(*env)->GetStringUTFChars(env, str, NULL);
	dbg(0,"search '%s'\n", search_string);

	config_get_attr(config, attr_navit, &attr, NULL);

	jclass cls = (*env)->GetObjectClass(env,thiz);
	jmethodID aMethodID = (*env)->GetMethodID(env, cls, "fillStringArray", "(Ljava/lang/String;)V");
	if(aMethodID != 0)
	{
		struct jni_object my_jni_object;
		GList *ret;
		struct mapset *ms4=navit_get_mapset(attr.u.navit);

		my_jni_object.env=env;
		my_jni_object.jo=thiz;
		my_jni_object.jm=aMethodID;

/* TODO (rikky#1#): does return nothing yet, also should use a generic callback instead of jni_object */		ret=search_by_address(ms4,search_string,partial,&my_jni_object);
		dbg(0,"ret=%p\n",ret);

		g_list_free(ret);
	}
	else
		dbg(0,"**** Unable to get methodID: fillStringArray");

	(*env)->ReleaseStringUTFChars(env, str, search_string);
}


JNIEXPORT jstring JNICALL
Java_org_navitproject_navit_NavitGraphics_CallbackLocalizedString( JNIEnv* env, jobject thiz, jobject str)
{
	const char *s;
	const char *localized_str;

	s=(*env)->GetStringUTFChars(env, str, NULL);
	//dbg(0,"*****string=%s\n",s);

	localized_str=gettext(s);
	//dbg(0,"localized string=%s",localized_str);

	// jstring dataStringValue = (jstring) localized_str;
	jstring js = (*env)->NewStringUTF(env,localized_str);

	(*env)->ReleaseStringUTFChars(env, str, s);

	return js;
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_CallbackMessageChannel( JNIEnv* env, jobject thiz, int channel, jobject str)
{
	const char *s;
	dbg(0,"enter %d %p\n",channel,str);

	config_get_attr(config, attr_navit, &attr, NULL);

	switch(channel)
	{
	case 1:
		// zoom in
		navit_zoom_in_cursor(global_navit, 2);
		break;
	case 2:
		// zoom out
		navit_zoom_out_cursor(global_navit, 2);
		break;
	case 6:
		// this procedure is not used at the moment!!

		// hopefully use a newly downloaded map, we just hope its set in navit.xml (make nicer soon)
		// remove all curents maps
		// ******global_navit->mapsets=NULL; // is this the correct way to clear the list?
		// now add the default /sdcard/navitmap.bin entry to the list
		// ******navit_add_mapset(global_navit,ms); // but how? please write me!!
		// now reload some stuff to make the change stick
		dbg(0,"trying to apply newly downloaded map to mapset %p\n",global_navit->mapsets);
		if (global_navit->mapsets)
		{
			struct mapset *ms;
			struct map *map;
			struct mapset_handle *msh;

			ms=global_navit->mapsets->data;
			msh=mapset_open(ms);
			while (msh && (map=mapset_next(msh, 0))) {
			}
			mapset_close(msh);
		}
		break;
	case 5:
		// call a command (like in gui)
		s=(*env)->GetStringUTFChars(env, str, NULL);
		dbg(0,"*****string=%s\n",s);
		command_evaluate(&attr.u.navit->self,s);
		(*env)->ReleaseStringUTFChars(env, str, s);
		break;
	case 4:
	{
		char *pstr;
		struct point p;
		struct coord c;
		struct pcoord pc;

		s=(*env)->GetStringUTFChars(env, str, NULL);
		char parse_str[strlen(s) + 1];
		strcpy(parse_str, s);
		(*env)->ReleaseStringUTFChars(env, str, s);
		dbg(0,"*****string=%s\n",parse_str);

		// set destination to (pixel-x#pixel-y)
		// pixel-x
		pstr = strtok (parse_str,"#");
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
		navit_set_destination(global_navit, &pc, parse_str, 1);
	}
	break;
	case 3:
	{
		char *name;
		s=(*env)->GetStringUTFChars(env, str, NULL);
		char parse_str[strlen(s) + 1];
		strcpy(parse_str, s);
		(*env)->ReleaseStringUTFChars(env, str, s);
		dbg(0,"*****string=%s\n",s);

		// set destination to (lat#lon#title)
		struct coord_geo g;
		char *p;
		char *stopstring;

		// lat
		p = strtok (parse_str,"#");
		g.lat = strtof(p, &stopstring);
		// lon
		p = strtok (NULL, "#");
		g.lng = strtof(p, &stopstring);
		// description
		name = strtok (NULL, "#");

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
		navit_set_destination(global_navit, &pc, name, 1);

	}
	break;
	default:
		dbg(0, "Unknown command");
	}
}

