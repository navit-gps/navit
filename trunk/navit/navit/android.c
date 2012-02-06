#include <stdlib.h>
#include <string.h>
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
#include "track.h"


JNIEnv *jnienv;
jobject *android_activity;
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
Java_org_navitproject_navit_Navit_NavitDestroy( JNIEnv* env)
{
	dbg(0, "shutdown navit\n");
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
Java_org_navitproject_navit_NavitWatch_poll( JNIEnv* env, jobject thiz, int func, int fd, int cond)
{
	void (*pollfunc)(JNIEnv *env, int fd, int cond)=(void *)func;

	pollfunc(env, fd, cond);
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
Java_org_navitproject_navit_NavitGraphics_CallbackSearchResultList( JNIEnv* env, jobject thiz, int partial, jobject country, jobject str)
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
		GList *ret = NULL;
		struct mapset *ms4=navit_get_mapset(attr.u.navit);
		struct search_list *sl = search_list_new(ms4);
		struct attr country_attr;
		const char *str_country=(*env)->GetStringUTFChars(env, country, NULL);

		country_attr.type=attr_country_iso2;
		country_attr.u.str=str_country;

		search_list_search(sl, &country_attr, 0);
		(*env)->ReleaseStringUTFChars(env, country, str_country);

		my_jni_object.env=env;
		my_jni_object.jo=thiz;
		my_jni_object.jm=aMethodID;

		/* TODO (rikky#1#): does return nothing yet, also should use a generic callback instead of jni_object */		ret=search_by_address(sl,search_string,partial,&my_jni_object);
		search_list_destroy(sl);

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
		navit_draw(attr.u.navit);
		break;
	case 2:
		// zoom out
		navit_zoom_out_cursor(attr.u.navit, 2);
		navit_draw(attr.u.navit);
		break;
	case 6:
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
		// navigate to display position
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
		// navigate to geo position
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

JNIEXPORT jstring JNICALL
Java_org_navitproject_navit_NavitGraphics_GetDefaultCountry( JNIEnv* env, jobject thiz, int channel, jobject str)
{
	struct attr search_attr, country_name, country_iso2, *country_attr;
	struct tracking *tracking;
	struct search_list_result *res;
	jstring return_string = NULL;

	struct attr attr;
	dbg(0,"enter %d %p\n",channel,str);

	config_get_attr(config_get(), attr_navit, &attr, NULL);

	country_attr=country_default();
	tracking=navit_get_tracking(attr.u.navit);
	if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL))
		country_attr=&search_attr;
	if (country_attr) {
		struct country_search *cs=country_search_new(country_attr, 0);
		struct item *item=country_search_get_item(cs);
		if (item && item_attr_get(item, attr_country_name, &country_name)) {
			struct mapset *ms=navit_get_mapset(attr.u.navit);
			struct search_list *search_list = search_list_new(ms);
			search_attr.type=attr_country_all;
			dbg(0,"country %s\n", country_name.u.str);
			search_attr.u.str=country_name.u.str;
			search_list_search(search_list, &search_attr, 0);
			while((res=search_list_get_result(search_list)))
			{
				dbg(0,"Get result: %s\n", res->country->iso2);
			}
			if (item_attr_get(item, attr_country_iso2, &country_iso2))
				return_string = (*env)->NewStringUTF(env,country_iso2.u.str);
		}
		country_search_destroy(cs);
	}

	return return_string;
}

JNIEXPORT jobjectArray JNICALL
Java_org_navitproject_navit_NavitGraphics_GetAllCountries( JNIEnv* env, jobject thiz)
{
	struct attr search_attr;
	struct search_list_result *res;
	GList* countries = NULL;
	int country_count = 0;
	jobjectArray all_countries;

	struct attr attr;
	dbg(0,"enter\n");

	config_get_attr(config_get(), attr_navit, &attr, NULL);

	struct mapset *ms=navit_get_mapset(attr.u.navit);
	struct search_list *search_list = search_list_new(ms);
	jobjectArray current_country = NULL;
	search_attr.type=attr_country_all;
	//dbg(0,"country %s\n", country_name.u.str);
	search_attr.u.str=g_strdup("");//country_name.u.str;
	search_list_search(search_list, &search_attr, 1);
	while((res=search_list_get_result(search_list)))
	{
		dbg(0,"Get result: %s\n", res->country->iso2);

		if (strlen(res->country->iso2)==2)
		{
			jstring j_iso2 = (*env)->NewStringUTF(env, res->country->iso2);
			jstring j_name = (*env)->NewStringUTF(env, gettext(res->country->name));

			current_country = (jobjectArray)(*env)->NewObjectArray(env, 2, (*env)->FindClass(env, "java/lang/String"), NULL);

			(*env)->SetObjectArrayElement(env, current_country, 0,  j_iso2);
			(*env)->SetObjectArrayElement(env, current_country, 1,  j_name);

			(*env)->DeleteLocalRef(env, j_iso2);
			(*env)->DeleteLocalRef(env, j_name);

			countries = g_list_prepend(countries, current_country);
			country_count++;
		}
	}

	search_list_destroy(search_list);
	all_countries = (jobjectArray)(*env)->NewObjectArray(env, country_count, (*env)->GetObjectClass(env, current_country), NULL);

	while(countries)
	{
		(*env)->SetObjectArrayElement(env, all_countries, --country_count, countries->data);
		countries = g_list_delete_link( countries, countries);
	}
	return all_countries;
}
