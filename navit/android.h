#include <jni.h>
extern JNIEnv *jnienv;
extern jobject *android_activity;
extern int android_version;
int android_find_class_global(char *name, jclass *ret);
int android_find_method(jclass class, char *name, char *args, jmethodID *ret);
int android_find_static_method(jclass class, char *name, char *args, jmethodID *ret);

struct jni_object {
	JNIEnv* env;
	jobject jo;
	jmethodID jm;
};
