#include <string.h>
#include "android.h"
#include <android/log.h>
#include "debug.h"
#include "callback.h"

JNIEnv *jnienv;
jobject *android_activity;


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
Java_org_navitproject_navit_Navit_NavitMain( JNIEnv* env, jobject thiz, jobject activity)
{
	char *strings[]={"/data/data/org.navitproject.navit/bin/navit",NULL};
	__android_log_print(ANDROID_LOG_ERROR,"test","called");
	jnienv=env;
	android_activity=activity;
	(*jnienv)->NewGlobalRef(jnienv, activity);
	dbg(0,"enter env=%p thiz=%p activity=%p\n",env,thiz,activity);
	main(1, strings);
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
	dbg(0,"enter %p %d %d\n",(struct callback *)id,pressed,button);
	if (id)
		callback_call_4((struct callback *)id,pressed,button,x,y);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navit_NavitGraphics_MotionCallback( JNIEnv* env, jobject thiz, int id, int x, int y)
{
	dbg(0,"enter %p %d %d\n",(struct callback *)id,x,y);
	if (id)
		callback_call_2((struct callback *)id,x,y);
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

