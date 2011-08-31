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
#include "start_real.h"


JNIEnv *jnienv;
jobject *android_activity;
struct callback_list *android_activity_cbl;
int android_version;

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

JNIEXPORT void JNICALL
Java_org_navitproject_navit_Navit_NavitMain( JNIEnv* env, jobject thiz, jobject activity, jobject lang, int version, jobject display_density_string, jobject path)
{
	char *strings[]={NULL,NULL};
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
	strings[0]=(*env)->GetStringUTFChars(env, path, NULL);
	main_real(1, strings);
	(*env)->ReleaseStringUTFChars(env, path, strings[0]);
	
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
	struct attr attr;
	const char *search_string =(*env)->GetStringUTFChars(env, str, NULL);
	dbg(0,"search '%s'\n", search_string);

	config_get_attr(config_get(), attr_navit, &attr, NULL);

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
	struct attr attr;
	const char *s;
	dbg(0,"enter %d %p\n",channel,str);

	config_get_attr(config_get(), attr_navit, &attr, NULL);

	switch(channel)
	{
	case 1:
		// zoom in
		navit_zoom_in_cursor(attr.u.navit, 2);
		break;
	case 2:
		// zoom out
		navit_zoom_out_cursor(attr.u.navit, 2);
		break;
	case 6:
		// this procedure is not used at the moment!!

		// hopefully use a newly downloaded map, we just hope its set in navit.xml (make nicer soon)
		// remove all curents maps
		// ******global_navit->mapsets=NULL; // is this the correct way to clear the list?
		// now add the default /sdcard/navitmap.bin entry to the list
		// ******navit_add_mapset(global_navit,ms); // but how? please write me!!
		// now reload some stuff to make the change stick
		break;
	case 5:
		// call a command (like in gui)
		s=(*env)->GetStringUTFChars(env, str, NULL);
		dbg(0,"*****string=%s\n",s);
		command_evaluate(&attr,s);
		(*env)->ReleaseStringUTFChars(env, str, s);
		break;
	case 4:
	{
		char *pstr;
		struct point p;
		struct coord c;
		struct pcoord pc;
		
		struct transformation *transform=navit_get_trans(attr.u.navit);

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

		transform_reverse(transform, &p, &c);


		pc.x = c.x;
		pc.y = c.y;
		pc.pro = transform_get_projection(transform);

		dbg(0,"22x=%d\n",pc.x);
		dbg(0,"22y=%d\n",pc.y);

		// start navigation asynchronous
		navit_set_destination(attr.u.navit, &pc, parse_str, 1);
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
		navit_set_destination(attr.u.navit, &pc, name, 1);

	}
	break;
	default:
		dbg(0, "Unknown command");
	}
}

