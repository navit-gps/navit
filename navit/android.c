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
jobject *android_activity = NULL;

struct android_search_priv {
    struct jni_object search_result_obj;
    struct event_idle *idle_ev;
    struct callback *idle_clb;
    struct search_list *search_list;
    struct attr search_attr;
    gchar **phrases;
    int current_phrase_per_level[4];
    int partial;
    int found;
};


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *aVm, void *aReserved) {
    if ((*aVm)->GetEnv(aVm,(void**)&jnienv, JNI_VERSION_1_6) != JNI_OK) {
        dbg(lvl_error,"Failed to get the environment");
        return -1;
    }
    dbg(lvl_debug,"Found the environment");
    return JNI_VERSION_1_6;
}

int android_find_class_global(char *name, jclass *ret) {
    *ret=(*jnienv)->FindClass(jnienv, name);
    if (! *ret) {
        dbg(lvl_error,"Failed to get Class %s",name);
        return 0;
    }
    *ret = (*jnienv)->NewGlobalRef(jnienv, *ret);
    return 1;
}

int android_find_method(jclass class, char *name, char *args, jmethodID *ret) {
    *ret = (*jnienv)->GetMethodID(jnienv, class, name, args);
    if (*ret == NULL) {
        dbg(lvl_error,"Failed to get Method %s with signature %s",name,args);
        return 0;
    }
    return 1;
}


int android_find_static_method(jclass class, char *name, char *args, jmethodID *ret) {
    *ret = (*jnienv)->GetStaticMethodID(jnienv, class, name, args);
    if (*ret == NULL) {
        dbg(lvl_error,"Failed to get static Method %s with signature %s",name,args);
        return 0;
    }
    return 1;
}


/**
 * @brief Starts the Navitlib for Android
 *
 * @param env provided by JVM
 * @param thiz the calling Navit instance
 * @param lang a string describing the language
 * @param path relates to NAVIT_DATA_DIR on linux
 * @param map_path where the binfiles are stored
 */
JNIEXPORT void JNICALL Java_org_navitproject_navit_Navit_navitMain( JNIEnv* env, jobject thiz,
        jstring lang, jstring path, jstring map_path) {
    const char *langstr;
    const char *map_file_path;
    jnienv=env;

    android_activity = (*jnienv)->NewGlobalRef(jnienv, thiz);

    langstr=(*env)->GetStringUTFChars(env, lang, NULL);
    dbg(lvl_debug,"enter env=%p thiz=%p activity=%p lang=%s",env,thiz,android_activity,langstr);
    setenv("LANG",langstr,1);
    (*env)->ReleaseStringUTFChars(env, lang, langstr);

    map_file_path=(*env)->GetStringUTFChars(env, map_path, NULL);
    setenv("NAVIT_USER_DATADIR",map_file_path,1);
    (*env)->ReleaseStringUTFChars(env, map_path, map_file_path);

    const char *strings=(*env)->GetStringUTFChars(env, path, NULL);
    main_real(1, &strings);
    (*env)->ReleaseStringUTFChars(env, path, strings);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_Navit_navitDestroy( JNIEnv* env, jobject thiz) {
    dbg(lvl_debug, "shutdown navit");
    exit(0);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitGraphics_sizeChangedCallback( JNIEnv* env, jobject thiz,
        jlong id, jint w, jint h) {
    dbg(lvl_debug,"enter %p %d %d",(struct callback *)(intptr_t)id,w,h);
    if (id) {
        callback_call_2((struct callback *)(intptr_t)id, w, h);
    }
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitGraphics_paddingChangedCallback(JNIEnv* env, jobject thiz,
        jlong id, jint left, jint top, jint right, jint bottom) {
    dbg(lvl_debug,"enter %p %d %d %d %d",(struct callback *)(intptr_t)id, left, top, right, bottom);
    if (id)
        callback_call_4((struct callback *)(intptr_t)id, left, top, right, bottom);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitGraphics_buttonCallback( JNIEnv* env, jobject thiz,
        jlong id, jint pressed, jint button, jint x, jint y) {
    dbg(lvl_debug,"enter %p %d %d",(struct callback *)(intptr_t)id,pressed,button);
    if (id)
        callback_call_4((struct callback *)(intptr_t)id,pressed,button,x,y);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitGraphics_motionCallback( JNIEnv* env, jobject thiz,
        jlong id, jint x, jint y) {
    dbg(lvl_debug,"enter %p %d %d",(struct callback *)(intptr_t)id,x,y);
    if (id)
        callback_call_2((struct callback *)(intptr_t)id,x,y);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitGraphics_keypressCallback( JNIEnv* env, jobject thiz,
        jlong id, jstring str) {
    const char *s;
    dbg(lvl_debug,"enter %p %p",(struct callback *)(intptr_t)id,str);
    s=(*env)->GetStringUTFChars(env, str, NULL);
    dbg(lvl_debug,"key=%s",s);
    if (id)
        callback_call_1((struct callback *)(intptr_t)id,s);
    (*env)->ReleaseStringUTFChars(env, str, s);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitTimeout_timeoutCallback( JNIEnv* env, jobject thiz,
        jlong id) {
    dbg(lvl_debug,"enter %p %p %p",thiz,(void *)id, (void *)(intptr_t)id);
    void (*event_handler)(void *) = *((void **)(intptr_t)id);
    event_handler((void*)(intptr_t)id);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitVehicle_vehicleCallback( JNIEnv * env, jobject thiz,
        jlong id, jobject location) {
    callback_call_1((struct callback *)(intptr_t)id, (void *)location);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitWatch_poll( JNIEnv* env, jobject thiz, jlong func, jint fd,
        jint cond) {
    void (*pollfunc)(JNIEnv *env, int fd, int cond)=(void *)func;

    pollfunc(env, fd, cond);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitWatch_watchCallback( JNIEnv* env, jobject thiz, jlong id) {
    dbg(lvl_debug,"enter %p %p",thiz, (void *)(intptr_t)id);
    callback_call_0((struct callback *)(intptr_t)id);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitSensors_sensorCallback( JNIEnv* env, jobject thiz,
        jlong id, jint sensor, jfloat x, jfloat y, jfloat z) {
    dbg(lvl_debug,"enter %p %p %f %f %f",thiz, (void *)(intptr_t)id,x,y,z);
    callback_call_4((struct callback *)(intptr_t)id, sensor, &x, &y, &z);
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitTraff_onFeedReceived(JNIEnv * env, jobject thiz,
        jlong id, jstring feed) {
    const char *s;
    s = (*env)->GetStringUTFChars(env, feed, NULL);
    if (id)
        callback_call_1((struct callback *)(intptr_t) id, s);
    (*env)->ReleaseStringUTFChars(env, feed, s);
}

// type: 0=town, 1=street, 2=House#
void android_return_search_result(struct jni_object *jni_o, int type, struct pcoord *location, const char *address) {
    struct coord_geo geo_location;
    struct coord c;
    jstring jaddress = NULL;
    JNIEnv* env;
    env=jni_o->env;
    jaddress = (*env)->NewStringUTF(jni_o->env,address);

    c.x=location->x;
    c.y=location->y;
    transform_to_geo(location->pro, &c, &geo_location);

    (*env)->CallVoidMethod(jni_o->env, jni_o->jo, jni_o->jm, type, geo_location.lat, geo_location.lng, jaddress);
    (*env)->DeleteLocalRef(jni_o->env, jaddress);
}

JNIEXPORT jstring JNICALL Java_org_navitproject_navit_NavitAppConfig_callbackLocalizedString( JNIEnv* env, jclass thiz,
        jstring str) {
    const char *s;
    const char *localized_str;

    s=(*env)->GetStringUTFChars(env, str, NULL);
    //dbg(lvl_debug,"*****string=%s",s);

    localized_str=navit_nls_gettext(s);
    //dbg(lvl_debug,"localized string=%s",localized_str);

    // jstring dataStringValue = (jstring) localized_str;
    jstring js = (*env)->NewStringUTF(env,localized_str);
    (*env)->ReleaseStringUTFChars(env, str, s);
    return js;
}


JNIEXPORT jstring JNICALL Java_org_navitproject_navit_NavitGraphics_getDefaultCountry( JNIEnv* env, jobject thiz,
        jint channel, jstring str) {
    struct attr search_attr, country_name, country_iso2, *country_attr;
    struct tracking *tracking;
    struct search_list_result *res;
    jstring return_string = NULL;

    struct attr attr;
    dbg(lvl_debug,"enter %d %p",channel,str);

    config_get_attr(config_get(), attr_navit, &attr, NULL);

    country_attr=country_default();
    tracking=navit_get_tracking(attr.u.navit);
    if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL)) {
        country_attr = &search_attr;
    }
    if (country_attr) {
        struct country_search *cs=country_search_new(country_attr, 0);
        struct item *item=country_search_get_item(cs);
        if (item && item_attr_get(item, attr_country_name, &country_name)) {
            struct mapset *ms=navit_get_mapset(attr.u.navit);
            struct search_list *search_list = search_list_new(ms);
            search_attr.type=attr_country_all;
            dbg(lvl_debug,"country %s", country_name.u.str);
            search_attr.u.str=country_name.u.str;
            search_list_search(search_list, &search_attr, 0);
            while((res=search_list_get_result(search_list))) {
                dbg(lvl_debug,"Get result: %s", res->country->iso2);
            }
            if (item_attr_get(item, attr_country_iso2, &country_iso2)) {
                return_string = (*env)->NewStringUTF(env, country_iso2.u.str);
            }
        }
        country_search_destroy(cs);
    }

    return return_string;
}


JNIEXPORT jobjectArray JNICALL Java_org_navitproject_navit_NavitGraphics_getAllCountries( JNIEnv* env,
        jclass thiz) {
    struct attr search_attr;
    struct search_list_result *res;
    GList* countries = NULL;
    int country_count = 0;
    jobjectArray all_countries;

    struct attr attr;
    dbg(lvl_debug,"enter");

    config_get_attr(config_get(), attr_navit, &attr, NULL);

    struct mapset *ms=navit_get_mapset(attr.u.navit);
    struct search_list *search_list = search_list_new(ms);
    jobjectArray current_country = NULL;
    search_attr.type=attr_country_all;
    //dbg(lvl_debug,"country %s", country_name.u.str);
    search_attr.u.str=g_strdup("");//country_name.u.str;
    search_list_search(search_list, &search_attr, 1);
    while((res=search_list_get_result(search_list))) {
        dbg(lvl_debug,"Get result: %s", res->country->iso2);

        if (strlen(res->country->iso2)==2) {
            jstring j_iso2 = (*env)->NewStringUTF(env, res->country->iso2);
            jstring j_name = (*env)->NewStringUTF(env, navit_nls_gettext(res->country->name));

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
    all_countries = (jobjectArray)(*env)->NewObjectArray(env, country_count, (*env)->GetObjectClass(env,current_country),
                    NULL);

    while(countries) {
        (*env)->SetObjectArrayElement(env, all_countries, --country_count, countries->data);
        countries = g_list_delete_link( countries, countries);
    }

    return all_countries;
}


JNIEXPORT jstring JNICALL Java_org_navitproject_navit_NavitGraphics_getCoordForPoint( JNIEnv* env,
        jobject thiz, jint x, jint y, jboolean absolute_coord) {

    jstring return_string = NULL;

    struct attr attr;
    config_get_attr(config_get(), attr_navit, &attr, NULL);

    struct transformation *transform=navit_get_trans(attr.u.navit);
    struct point p;
    struct coord c;
    struct pcoord pc;

    p.x = x;
    p.y = y;

    transform_reverse(transform, &p, &c);

    pc.x = c.x;
    pc.y = c.y;
    pc.pro = transform_get_projection(transform);

    char coord_str[32];
    if (absolute_coord) {
        pcoord_format_absolute(&pc, coord_str, sizeof(coord_str), ",");
    } else {
        pcoord_format_degree_short(&pc, coord_str, sizeof(coord_str), " ");
    }

    dbg(lvl_error,"Display point x=%d y=%d is \"%s\"",x,y,coord_str);
    return_string = (*env)->NewStringUTF(env,coord_str);

    return return_string;
}

JNIEXPORT jint JNICALL Java_org_navitproject_navit_NavitGraphics_callbackMessageChannel( JNIEnv* env, jclass thiz,
        jint channel, jstring str) {
    struct attr attr;
    const char *s;
    jint ret = 0;
    dbg(lvl_debug,"enter %d %p",channel,str);
    config_get_attr(config_get(), attr_navit, &attr, NULL);

    switch(channel) {
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
    case 6: {// add a map to the current mapset, return 1 on success
        struct mapset *ms = navit_get_mapset(attr.u.navit);
        struct attr type, name, data, *attrs[4];
        const char *map_location = (*env)->GetStringUTFChars(env, str, NULL);
        dbg(lvl_debug, "*****string=%s", map_location);
        type.type = attr_type;
        type.u.str = "binfile";

        data.type = attr_data;
        data.u.str = g_strdup(map_location);

        name.type = attr_name;
        name.u.str = g_strdup(map_location);

        attrs[0] = &type;
        attrs[1] = &data;
        attrs[2] = &name;
        attrs[3] = NULL;

        struct map *new_map = map_new(NULL, attrs);
        if (new_map) {
            struct attr map_a;
            map_a.type = attr_map;
            map_a.u.map = new_map;
            ret = mapset_add_attr(ms, &map_a);
            navit_draw(attr.u.navit);
        }
        (*env)->ReleaseStringUTFChars(env, str, map_location);
        break;
    }
    case 7: { // remove a map from the current mapset, return 1 on success
        struct mapset *ms = navit_get_mapset(attr.u.navit);
        struct attr map_r;
        const char *map_location = (*env)->GetStringUTFChars(env, str, NULL);
        struct map *delete_map = mapset_get_map_by_name(ms, map_location);

        if (delete_map) {
            dbg(lvl_debug, "delete map %s (%p)", map_location, delete_map);
            map_r.type = attr_map;
            map_r.u.map = delete_map;
            ret = mapset_remove_attr(ms, &map_r);
            navit_draw(attr.u.navit);
        }
        (*env)->ReleaseStringUTFChars(env, str, map_location);
        break;
    }
    case 5:
        // call a command (like in gui)
        s = (*env)->GetStringUTFChars(env, str, NULL);
        dbg(lvl_debug, "*****string=%s", s);
        command_evaluate(&attr, s);
        (*env)->ReleaseStringUTFChars(env, str, s);
        break;
    case 4: { // navigate to display position
        char *pstr;
        struct point p;
        struct coord c;
        struct pcoord pc;
        struct transformation *transform = navit_get_trans(attr.u.navit);

        s = (*env)->GetStringUTFChars(env, str, NULL);
        char parse_str[strlen(s) + 1];
        strcpy(parse_str, s);
        (*env)->ReleaseStringUTFChars(env, str, s);
        dbg(lvl_debug, "*****string=%s", parse_str);

        // set destination to (pixel-x#pixel-y)
        // pixel-x
        pstr = strtok(parse_str, "#");
        p.x = atoi(pstr);
        // pixel-y
        pstr = strtok(NULL, "#");
        p.y = atoi(pstr);

        dbg(lvl_debug, "11x=%d", p.x);
        dbg(lvl_debug, "11y=%d", p.y);

        transform_reverse(transform, &p, &c);

        pc.x = c.x;
        pc.y = c.y;
        pc.pro = transform_get_projection(transform);

        char coord_str[32];
        //pcoord_format_short(&pc, coord_str, sizeof(coord_str), " ");
        pcoord_format_degree_short(&pc, coord_str, sizeof(coord_str), " ");
        dbg(lvl_debug,"Setting destination to %s",coord_str);
        // start navigation asynchronous
        navit_set_destination(attr.u.navit, &pc, coord_str, 1);
    }
    case 3: {
        // navigate to geo position
        char *name;
        s = (*env)->GetStringUTFChars(env, str, NULL);
        char parse_str[strlen(s) + 1];
        strcpy(parse_str, s);
        (*env)->ReleaseStringUTFChars(env, str, s);
        dbg(lvl_debug, "*****string=%s", s);

        // set destination to (lat#lon#title)
        struct coord_geo g;
        char *p;
        char *stopstring;

        // lat
        p = strtok(parse_str, "#");
        g.lat = strtof(p, &stopstring);
        // lon
        p = strtok(NULL, "#");
        g.lng = strtof(p, &stopstring);
        // description
        name = strtok(NULL, "#");

        dbg(lvl_debug, "lat=%f", g.lat);
        dbg(lvl_debug, "lng=%f", g.lng);
        dbg(lvl_debug, "str1=%s", name);

        struct coord c;
        transform_from_geo(projection_mg, &g, &c);

        struct pcoord pc;
        pc.x = c.x;
        pc.y = c.y;
        pc.pro = projection_mg;
        char coord_str[32];
        if (!name || *name == '\0') {
            pcoord_format_degree_short(&pc, coord_str, sizeof(coord_str), " ");
            name = coord_str;
        }
        // start navigation asynchronous
        navit_set_destination(attr.u.navit, &pc, name, 1);
        break;
    }
    default:
        dbg(lvl_error, "Unknown command: %d", channel);
    }

    return ret;
}


static char *postal_str(struct search_list_result *res, int level) {
    char *ret=NULL;
    if (res->town->common.postal)
        ret=res->town->common.postal;
    if (res->town->common.postal_mask)
        ret=res->town->common.postal_mask;
    if (level == 1)
        return ret;
    if (res->street->common.postal)
        ret=res->street->common.postal;
    if (res->street->common.postal_mask)
        ret=res->street->common.postal_mask;
    if (level == 2)
        return ret;
    if (res->house_number->common.postal)
        ret=res->house_number->common.postal;
    if (res->house_number->common.postal_mask)
        ret=res->house_number->common.postal_mask;
    return ret;
}

static char *district_str(struct search_list_result *res, int level) {
    char *ret=NULL;
    if (res->town->common.district_name)
        ret=res->town->common.district_name;
    if (level == 1)
        return ret;
    if (res->street->common.district_name)
        ret=res->street->common.district_name;
    if (level == 2)
        return ret;
    if (res->house_number->common.district_name)
        ret=res->house_number->common.district_name;
    return ret;
}

static char *town_str(struct search_list_result *res, int level) {
    char *town=res->town->common.town_name;
    char *district=district_str(res, level);
    char *postal=postal_str(res, level);
    char *postal_sep=" ";
    char *district_begin=" (";
    char *district_end=")";
    char *county_sep = ", Co. ";
    char *county = res->town->common.county_name;
    if (!postal)
        postal_sep=postal="";
    if (!district)
        district_begin=district_end=district="";
    if (!county)
        county_sep=county="";

    return g_strdup_printf("%s%s%s%s%s%s%s%s", postal, postal_sep, town, district_begin, district,
                           district_end, county_sep, county);
}

static void android_search_end(struct android_search_priv *search_priv) {
    dbg(lvl_debug, "End search");
    JNIEnv* env = search_priv->search_result_obj.env;
    if (search_priv->idle_ev) {
        event_remove_idle(search_priv->idle_ev);
        search_priv->idle_ev=NULL;
    }
    if (search_priv->idle_clb) {
        callback_destroy(search_priv->idle_clb);
        search_priv->idle_clb=NULL;
    }
    jclass cls = (*env)->GetObjectClass(env,search_priv->search_result_obj.jo);
    jmethodID finish_MethodID = (*env)->GetMethodID(env, cls, "finishAddressSearch", "()V");
    if(finish_MethodID != 0) {
        (*env)->CallVoidMethod(env, search_priv->search_result_obj.jo, finish_MethodID);
    } else {
        dbg(lvl_error, "Error method finishAddressSearch not found");
    }

    search_list_destroy(search_priv->search_list);
    g_strfreev(search_priv->phrases);
    g_free(search_priv);
}

static enum attr_type android_search_level[] = {
    attr_town_or_district_name,
    attr_street_name,
    attr_house_number
};

static void android_search_idle_result(struct android_search_priv *search_priv, struct search_list_result *res) {
//    commented out because otherwise cyclomatic complexity needleslly reported as too high
//    dbg(lvl_debug, "Town: %s, Street: %s",res->town ? res->town->common.town_name : "no town",
//        res->street ? res->street->name : "no street");
    search_priv->found = 1;
    switch (search_priv->search_attr.type) {
    case attr_town_or_district_name: {
        gchar *town = town_str(res, 1);
        android_return_search_result(&search_priv->search_result_obj, 0, res->town->common.c, town);
        g_free(town);
    }
    break;
    case attr_street_name: {
        gchar *town = town_str(res, 2);
        gchar *address = g_strdup_printf("%.101s,%.101s, %.101s", res->country->name, town, res->street->name);
        android_return_search_result(&search_priv->search_result_obj, 1, res->street->common.c, address);
        g_free(address);
        g_free(town);
    }
    break;
    case attr_house_number: {
        gchar *town = town_str(res, 3);
        gchar *address = g_strdup_printf("%.101s, %.101s, %.101s %.15s", res->country->name, town,
                                         res->street->name, res->house_number->house_number);
        android_return_search_result(&search_priv->search_result_obj, 2, res->house_number->common.c, address);
        g_free(address);
        g_free(town);
    }
    break;
    default:
        dbg(lvl_error, "Unhandled search type %d", search_priv->search_attr.type);
    }
}

static void android_search_idle(struct android_search_priv *search_priv) {
    dbg(lvl_debug, "enter android_search_idle");

    struct search_list_result *res = search_list_get_result(search_priv->search_list);
    if (res) {
        android_search_idle_result(search_priv, res);
    } else {
        int level = search_list_level(search_priv->search_attr.type) - 1;

        if (search_priv->found) {
            search_priv->found = 0;
            if (search_priv->search_attr.type != attr_house_number) {
                level++;
            }
        }
        dbg(lvl_info, "test phrase: %d,%d, %d, level %d", search_priv->current_phrase_per_level[0],
            search_priv->current_phrase_per_level[1], search_priv->current_phrase_per_level[2], level)
        do {
            while (!search_priv->phrases[++search_priv->current_phrase_per_level[level]]) {
                dbg(lvl_info, "next phrase: %d,%d, %d, level %d", search_priv->current_phrase_per_level[0],
                    search_priv->current_phrase_per_level[1], search_priv->current_phrase_per_level[2], level)
                if (level > 0) {
                    search_priv->current_phrase_per_level[level] = -1;
                    level--;
                } else {
                    android_search_end(search_priv);
                    return;
                }
            }
        } while (level > 0 ? search_priv->current_phrase_per_level[level] == search_priv->current_phrase_per_level[level-1] :
                 0);
        dbg(lvl_info, "used phrase: %d,%d, %d, level %d, '%s'", search_priv->current_phrase_per_level[0],
            search_priv->current_phrase_per_level[1], search_priv->current_phrase_per_level[2], level,
            attr_to_name(android_search_level[level]))
        dbg(lvl_debug, "Search for '%s'", search_priv->phrases[search_priv->current_phrase_per_level[level]]);
        search_priv->search_attr.type = android_search_level[level];
        search_priv->search_attr.u.str = search_priv->phrases[search_priv->current_phrase_per_level[level]];
        struct attr test;
        test.type = android_search_level[level];
        test.u.str = search_priv->phrases[search_priv->current_phrase_per_level[level]];
        search_list_search(search_priv->search_list, &test, search_priv->partial);
    }
    dbg(lvl_info, "leave");
}

static char *search_fix_spaces(const char *str) {
    int i;
    int len=strlen(str);
    char c,*s,*d,*ret=g_strdup(str);

    for (i = 0 ; i < len ; i++) {
        if (ret[i] == ',' || ret[i] == '/')
            ret[i]=' ';
    }
    s=ret;
    d=ret;
    len=0;
    do {
        c=*s++;
        if (c != ' ' || len != 0) {
            *d++=c;
            len++;
        }
        while (c == ' ' && *s == ' ')
            s++;
        if (c == ' ' && *s == '\0') {
            d--;
            len--;
        }
    } while (c);

    return ret;
}

static void start_search(struct android_search_priv *search_priv, const char *search_string) {
    dbg(lvl_debug,"enter %s", search_string);
    char *str=search_fix_spaces(search_string);
    search_priv->phrases = g_strsplit(str, " ", 0);
    //ret=search_address_town(ret, sl, phrases, NULL, partial, jni);

    dbg(lvl_debug,"First search phrase %s", search_priv->phrases[0]);
    search_priv->search_attr.u.str= search_priv->phrases[0];
    search_priv->search_attr.type=attr_town_or_district_name;
    search_list_search(search_priv->search_list, &search_priv->search_attr, search_priv->partial);

    search_priv->idle_clb = callback_new_1(callback_cast(android_search_idle), search_priv);
    search_priv->idle_ev = event_add_idle(50,search_priv->idle_clb);
    //callback_call_0(search_priv->idle_clb);

    g_free(str);
    dbg(lvl_debug,"leave");
}

JNIEXPORT jlong JNICALL Java_org_navitproject_navit_NavitAddressSearchActivity_callbackStartAddressSearch( JNIEnv* env,
        jobject thiz, jint partial, jstring country, jstring str) {
    struct attr attr;
    const char *search_string =(*env)->GetStringUTFChars(env, str, NULL);
    dbg(lvl_debug,"search '%s'", search_string);

    config_get_attr(config_get(), attr_navit, &attr, NULL);

    jclass cls = (*env)->GetObjectClass(env,thiz);
    jmethodID aMethodID = (*env)->GetMethodID(env, cls, "receiveAddress", "(IFFLjava/lang/String;)V");
    struct android_search_priv *search_priv = NULL;

    if(aMethodID != 0) {
        struct mapset *ms4=navit_get_mapset(attr.u.navit);
        struct attr country_attr;
        const char *str_country=(*env)->GetStringUTFChars(env, country, NULL);
        struct search_list_result *slr;
        int count = 0;

        search_priv = g_new0( struct android_search_priv, 1);
        search_priv->search_list = search_list_new(ms4);
        search_priv->partial = partial;
        search_priv->current_phrase_per_level[1] = -1;
        search_priv->current_phrase_per_level[2] = -1;

        country_attr.type=attr_country_iso2;
        country_attr.u.str=g_strdup(str_country);
        search_list_search(search_priv->search_list, &country_attr, 0);

        while ((slr=search_list_get_result(search_priv->search_list))) {
            count++;
        }
        if (!count)
            dbg(lvl_error,"Country not found");

        dbg(lvl_debug,"search in country '%s'", str_country);
        (*env)->ReleaseStringUTFChars(env, country, str_country);

        search_priv->search_result_obj.env = env;
        search_priv->search_result_obj.jo = (*env)->NewGlobalRef(env, thiz);
        search_priv->search_result_obj.jm = aMethodID;

        start_search(search_priv, search_string);
    } else
        dbg(lvl_error,"**** Unable to get methodID: fillStringArray");

    (*env)->ReleaseStringUTFChars(env, str, search_string);

    return (jlong)(long)search_priv;
}

JNIEXPORT void JNICALL Java_org_navitproject_navit_NavitAddressSearchActivity_callbackCancelAddressSearch( JNIEnv* env,
        jobject thiz, jlong handle) {
    struct android_search_priv *priv = (void*)(long)handle;

    if (priv)
        android_search_end(priv);
    else
        dbg(lvl_error, "Error: Cancel search failed");
}
