#include <string.h>
#include "android.h"
#include <android/log.h>
#include "debug.h"
#include "callback.h"

JNIEnv *jnienv;
jobject *android_activity;

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/HelloJni/HelloJni.java
 */
JNIEXPORT void JNICALL
Java_org_navitproject_navit_Navit_NavitMain( JNIEnv* env, jobject thiz, jobject activity)
{
	char *strings[]={"/data/data/org.navitproject.navit/bin/navit",NULL};
	__android_log_print(ANDROID_LOG_ERROR,"test","called");
	jnienv=env;
	android_activity=activity;
	dbg(0,"enter env=%p thiz=%p activity=%p\n",env,thiz,activity);
	main(1, strings);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navitgraphics_NavitGraphics_SizeChangedCallback( JNIEnv* env, jobject thiz, int id, int w, int h)
{
	dbg(0,"enter %p %d %d\n",(struct callback *)id,w,h);
	if (id)
		callback_call_2((struct callback *)id,w,h);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navitgraphics_NavitGraphics_ButtonCallback( JNIEnv* env, jobject thiz, int id, int pressed, int button, int x, int y)
{
	dbg(0,"enter %p %d %d\n",(struct callback *)id,pressed,button);
	if (id)
		callback_call_4((struct callback *)id,pressed,button,x,y);
}

JNIEXPORT void JNICALL
Java_org_navitproject_navitgraphics_NavitGraphics_MotionCallback( JNIEnv* env, jobject thiz, int id, int x, int y)
{
	dbg(0,"enter %p %d %d\n",(struct callback *)id,x,y);
	if (id)
		callback_call_2((struct callback *)id,x,y);
}
