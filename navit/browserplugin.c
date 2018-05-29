/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-navit-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Josh Aas <josh@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "browserplugin.h"
#include <navit/navit.h>
#include <navit/item.h>
#include <navit/config_.h>
#include <navit/callback.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PLUGIN_NAME        "Navit Sample Plug-in"
#define PLUGIN_DESCRIPTION PLUGIN_NAME " (Mozilla SDK)"
#define PLUGIN_VERSION     "1.0.0.0"

static NPNetscapeFuncs *sBrowserFuncs = NULL;
extern struct NPClass navitclass,navitclass2;

typedef struct InstanceData {
    NPP npp;
    NPWindow window;
} InstanceData;


static void fillPluginFunctionTable(NPPluginFuncs * pFuncs) {
    pFuncs->version = 11;
    pFuncs->size = sizeof(*pFuncs);
    pFuncs->newp = NPP_New;
    pFuncs->destroy = NPP_Destroy;
    pFuncs->setwindow = NPP_SetWindow;
    pFuncs->newstream = NPP_NewStream;
    pFuncs->destroystream = NPP_DestroyStream;
    pFuncs->asfile = NPP_StreamAsFile;
    pFuncs->writeready = NPP_WriteReady;
    pFuncs->write = NPP_Write;
    pFuncs->print = NPP_Print;
    pFuncs->event = NPP_HandleEvent;
    pFuncs->urlnotify = NPP_URLNotify;
    pFuncs->getvalue = NPP_GetValue;
    pFuncs->setvalue = NPP_SetValue;
}

NP_EXPORT(NPError)
NP_Initialize(NPNetscapeFuncs * bFuncs, NPPluginFuncs * pFuncs) {
    NPError err = NPERR_NO_ERROR;
    NPBool supportsXEmbed = false;
    NPNToolkitType toolkit = 0;

    sBrowserFuncs = bFuncs;

    fillPluginFunctionTable(pFuncs);
    err =
        sBrowserFuncs->getvalue(NULL, NPNVSupportsXEmbedBool,
                                (void *) &supportsXEmbed);
    if (err != NPERR_NO_ERROR || supportsXEmbed != true)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    err =
        sBrowserFuncs->getvalue(NULL, NPNVToolkit, (void *) &toolkit);

    if (err != NPERR_NO_ERROR || toolkit != NPNVGtk2)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    return NPERR_NO_ERROR;
}

NP_EXPORT(char *)
NP_GetPluginVersion() {
    return PLUGIN_VERSION;
}

NP_EXPORT(char *)
NP_GetMIMEDescription() {
    return "application/navit-plugin:nsc:Navit plugin";
}

NP_EXPORT(NPError)
NP_GetValue(void *future, NPPVariable aVariable, void *aValue) {
    fprintf(stderr, "NP_GetValue %d\n", aVariable);
    switch (aVariable) {
    case NPPVpluginNameString:
        *((char **) aValue) = PLUGIN_NAME;
        break;
    case NPPVpluginDescriptionString:
        *((char **) aValue) = PLUGIN_DESCRIPTION;
        break;
    default:
        return NPERR_INVALID_PARAM;
        break;
    }
    return NPERR_NO_ERROR;
}

NP_EXPORT(NPError)
NP_Shutdown() {
    return NPERR_NO_ERROR;
}

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData * saved) {
    char *args[]= {"/usr/bin/navit",NULL};
    // Make sure we can render this plugin
    NPBool browserSupportsWindowless = false;
    sBrowserFuncs->getvalue(instance, NPNVSupportsWindowless,
                            &browserSupportsWindowless);
    if (!browserSupportsWindowless) {
        printf("Windowless mode not supported by the browser\n");
        return NPERR_GENERIC_ERROR;
    }
#if 0
    sBrowserFuncs->setvalue(instance, NPPVpluginWindowBool,
                            (void *) true);
#endif

    // set up our our instance data
    InstanceData *instanceData =
        (InstanceData *) malloc(sizeof(InstanceData));
    if (!instanceData)
        return NPERR_OUT_OF_MEMORY_ERROR;
    memset(instanceData, 0, sizeof(InstanceData));
    instanceData->npp = instance;
    instance->pdata = instanceData;
    fprintf(stderr, "npp=%p\n", instance);

    main_real(1, args);
    return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData ** save) {
    InstanceData *instanceData = (InstanceData *) (instance->pdata);
    free(instanceData);
    return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow * window) {
    struct attr navit,graphics,windowid;
    InstanceData *instanceData = (InstanceData *) (instance->pdata);
    if (window->window == instanceData->window.window)
        return;
    instanceData->window = *window;
    fprintf(stderr, "Window 0x%x\n", window->window);
    if (!config_get_attr(config, attr_navit, &navit, NULL)) {
        fprintf(stderr,"No navit\n");
        return NPERR_GENERIC_ERROR;
    }
    if (!navit_get_attr(navit.u.navit, attr_graphics, &graphics, NULL)) {
        fprintf(stderr,"No Graphics\n");
        return NPERR_GENERIC_ERROR;
    }
    windowid.type=attr_windowid;
    windowid.u.num=window->window;
    if (!graphics_set_attr(graphics.u.graphics, &windowid)) {
        fprintf(stderr,"Failed to set window\n");
        return NPERR_GENERIC_ERROR;
    }

    return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream * stream, NPBool seekable, uint16_t * stype) {
    return NPERR_GENERIC_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream * stream, NPReason reason) {
    return NPERR_GENERIC_ERROR;
}

int32_t NPP_WriteReady(NPP instance, NPStream * stream) {
    return 0;
}

int32_t NPP_Write(NPP instance, NPStream * stream, int32_t offset, int32_t len,
                  void *buffer) {
    return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream * stream, const char *fname) {

}

void NPP_Print(NPP instance, NPPrint * platformPrint) {

}

int16_t NPP_HandleEvent(NPP instance, void *event) {


    return 0;

#if 0
    InstanceData *instanceData = (InstanceData *) (instance->pdata);
    XEvent *nativeEvent = (XEvent *) event;

    if (nativeEvent->type != GraphicsExpose)
        return 0;

    XGraphicsExposeEvent *expose = &nativeEvent->xgraphicsexpose;
    instanceData->window.window = (void *) (expose->drawable);

    GdkNativeWindow nativeWinId = (XID) (instanceData->window.window);
    GdkDrawable *gdkWindow =
        GDK_DRAWABLE(gdk_window_foreign_new(nativeWinId));
    drawWindow(instanceData, gdkWindow);
    g_object_unref(gdkWindow);
#endif

    return 1;
}

void NPP_URLNotify(NPP instance, const char *URL, NPReason reason, void *notifyData) {

}

struct NavitObject {
    NPClass *class;
    uint32_t referenceCount;
    InstanceData *instanceData;
    int is_attr;
    struct attr attr;
};

void printIdentifier(NPIdentifier name) {
    NPUTF8 *str;
    str = sBrowserFuncs->utf8fromidentifier(name);
    fprintf(stderr, "%s\n", str);
    sBrowserFuncs->memfree(str);
}

NPObject *allocate(NPP npp, NPClass * aClass) {
    struct NavitObject *ret = calloc(sizeof(struct NavitObject), 1);
    if (ret) {
        ret->class = aClass;
        ret->instanceData = npp->pdata;
        fprintf(stderr, "instanceData for %p is %p\n", ret,
                ret->instanceData);
    }
    return (NPObject *) ret;
}


void invalidate(NPObject * npobj) {
    fprintf(stderr, "invalidate\n");
}


bool hasMethod(NPObject * npobj, NPIdentifier name) {
    fprintf(stderr, "hasMethod\n");
    printIdentifier(name);
    if (name == sBrowserFuncs->getstringidentifier("command"))
        return true;
    if (name == sBrowserFuncs->getstringidentifier("get_attr"))
        return true;
    if (name == sBrowserFuncs->getstringidentifier("toString"))
        return true;
    if (name == sBrowserFuncs->getstringidentifier("nativeMethod"))
        return true;
    if (name ==
            sBrowserFuncs->getstringidentifier("anotherNativeMethod"))
        return true;

    return false;
}

enum attr_type variant_to_attr_type(const NPVariant *variant) {
    if (NPVARIANT_IS_STRING(*variant))
        return attr_from_name(NPVARIANT_TO_STRING(*variant).utf8characters);
    return attr_none;
}



bool invoke(NPObject * npobj, NPIdentifier name, const NPVariant * args, uint32_t argCount, NPVariant * result) {
    struct NavitObject *obj = (struct NavitObject *) npobj;
    fprintf(stderr, "invoke\n");
    printIdentifier(name);
    if (name == sBrowserFuncs->getstringidentifier("get_attr")) {
        enum attr_type attr_type;
        struct attr attr;
        if (!argCount)
            return false;
        attr_type=variant_to_attr_type(&args[0]);
        if (attr_type == attr_none)
            return false;
        if (config_get_attr(config, attr_type, &attr, NULL)) {
            struct NavitObject *obj2 = (struct NavitObject *)sBrowserFuncs->createobject(obj->instanceData->npp, &navitclass);
            obj2->is_attr=1;
            obj2->attr=attr;
            OBJECT_TO_NPVARIANT((NPObject *)obj2, *result);
            return true;
        } else {
            VOID_TO_NPVARIANT(*result);
            return true;
        }
    }
    if (name == sBrowserFuncs->getstringidentifier("command")) {
        enum attr_type attr_type;
        struct attr attr;
        NPObject *window;
        NPError err;
        NPVariant value;
        if (!argCount || !NPVARIANT_IS_STRING(args[0]))
            return false;
        if (navit_get_attr(obj->attr.u.navit, attr_callback_list, &attr, NULL)) {
            int valid=0;
            callback_list_call_attr_4(attr.u.callback_list, attr_command, NPVARIANT_TO_STRING(args[0]), NULL, NULL, &valid);
        }
        err=sBrowserFuncs->getvalue(obj->instanceData->npp, NPNVWindowNPObject, (void *) &window);
        fprintf(stderr,"error1:%d\n",err);
        //OBJECT_TO_NPVARIANT(window, *result);
        err=sBrowserFuncs->invoke(obj->instanceData->npp, window, sBrowserFuncs->getstringidentifier("Array"), window, 0,
                                  result);
        fprintf(stderr,"error2:%d\n",err);
        INT32_TO_NPVARIANT(23, value);
        err=sBrowserFuncs->setproperty(obj->instanceData->npp, NPVARIANT_TO_OBJECT(*result), sBrowserFuncs->getintidentifier(0),
                                       &value);
        INT32_TO_NPVARIANT(42, value);
        err=sBrowserFuncs->setproperty(obj->instanceData->npp, NPVARIANT_TO_OBJECT(*result), sBrowserFuncs->getintidentifier(1),
                                       &value);
        fprintf(stderr,"error3:%d\n",err);


        //VOID_TO_NPVARIANT(*result);
        return true;
    }
    if (name == sBrowserFuncs->getstringidentifier("toString")) {
        char *s;
        if (obj->is_attr) {
            s="[NavitObject attribute]";
            STRINGZ_TO_NPVARIANT(strdup(s), *result);
            return true;
        }
        s=g_strdup_printf("[NavitObject %s]",attr_to_name(obj->attr.type));
        STRINGZ_TO_NPVARIANT(strdup(s), *result);
        g_free(s);
        return true;
    }
    if (name == sBrowserFuncs->getstringidentifier("nativeMethod")) {
        result->type = NPVariantType_Int32;
        result->value.intValue = 23;
        return true;
    }
    if (name ==
            sBrowserFuncs->getstringidentifier("anotherNativeMethod")) {
        result->type = NPVariantType_Int32;
        result->value.intValue = 42;
        return true;
    }
    return false;
}


bool invokeDefault(NPObject * npobj, const NPVariant * args, uint32_t argCount, NPVariant * result) {
    fprintf(stderr, "invokeDefault\n");
    return false;
}


bool hasProperty(NPObject * npobj, NPIdentifier name) {
    struct NavitObject *obj = (struct NavitObject *) npobj;
    fprintf(stderr, "hasProperty\n");
    printIdentifier(name);
    if (obj->is_attr && name == sBrowserFuncs->getstringidentifier("type"))
        return true;
    if (obj->is_attr && name == sBrowserFuncs->getstringidentifier("val"))
        return true;
    if (name == sBrowserFuncs->getstringidentifier("nativeProperty")) {
        return true;
    }
    return false;
}


bool getProperty(NPObject * npobj, NPIdentifier name, NPVariant * result) {
    struct NavitObject *obj = (struct NavitObject *) npobj;
    fprintf(stderr, "getProperty %p\n", obj);
    fprintf(stderr, "instanceData %p\n", obj->instanceData);
    if (obj->is_attr && name == sBrowserFuncs->getstringidentifier("type")) {
        STRINGZ_TO_NPVARIANT(strdup(attr_to_name(obj->attr.type)), *result);
        return true;
    }
    if (obj->is_attr && name == sBrowserFuncs->getstringidentifier("val")) {
        struct NavitObject *obj2 = (struct NavitObject *)sBrowserFuncs->createobject(obj->instanceData->npp, &navitclass);
        obj2->attr=obj->attr;
        OBJECT_TO_NPVARIANT((NPObject *)obj2, *result);
        return true;
    }
    if (name == sBrowserFuncs->getstringidentifier("nativeProperty")) {
        result->type = NPVariantType_Object;
        fprintf(stderr, "npp=%p\n", obj->instanceData->npp);
        result->value.objectValue = sBrowserFuncs->createobject(obj->instanceData->npp, &navitclass2);
        return true;
    }
    return false;
}


bool setProperty(NPObject * npobj, NPIdentifier name, const NPVariant * value) {
    fprintf(stderr, "setProperty\n");
    return false;
}


bool removeProperty(NPObject * npobj, NPIdentifier name) {
    fprintf(stderr, "removeProperty\n");
    return false;
}





struct NPClass navitclass = {
    1,
    allocate,
    NULL,			/* deallocate */
    invalidate,
    hasMethod,
    invoke,
    invokeDefault,
    hasProperty,
    getProperty,
    setProperty,
    removeProperty,
};

struct NPClass navitclass2 = {
    1,
    allocate,		/* allocate */
    NULL,			/* deallocate */
    invalidate,
    hasMethod,
    invoke,
    invokeDefault,
    hasProperty,
    getProperty,
    setProperty,
    removeProperty,
};


NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
    fprintf(stderr, "NPP_GetValue %d %d\n", variable,
            NPPVpluginScriptableNPObject);
    if (variable == NPPVpluginNeedsXEmbed) {
        *((NPBool *) value) = true;
        fprintf(stderr, "Xembedd\n");
        return NPERR_NO_ERROR;
    }

    if (variable == NPPVpluginScriptableNPObject) {
        *(NPObject **) value =
            sBrowserFuncs->createobject(instance, &navitclass);
        return NPERR_NO_ERROR;
    }
    return NPERR_GENERIC_ERROR;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
    return NPERR_GENERIC_ERROR;
}
