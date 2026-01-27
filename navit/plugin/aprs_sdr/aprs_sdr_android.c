/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#ifdef ANDROID
#include "aprs_sdr_android.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>

/* RTL-SDR USB identifiers */
#define RTL_SDR_VENDOR_ID  0x0bda
#define RTL_SDR_PRODUCT_ID_2832U 0x2832
#define RTL_SDR_PRODUCT_ID_2838 0x2838

/* USB control request types */
#define USB_TYPE_VENDOR    (0x02 << 5)
#define USB_RECIP_DEVICE   0x00
#define USB_DIR_OUT        0x00
#define USB_DIR_IN         0x80

struct aprs_sdr_android {
    JNIEnv *env;
    jobject context;
    jobject usb_manager;
    jobject usb_device;
    jclass usb_manager_class;
    jclass usb_device_class;
    jclass usb_device_connection_class;
    jmethodID get_device_list_method;
    jmethodID has_permission_method;
    jmethodID request_permission_method;
    jmethodID open_device_method;
    jmethodID bulk_transfer_method;
    jmethodID control_transfer_method;
    jobject usb_connection;
    int device_fd;
    int vendor_id;
    int product_id;
};

static jmethodID get_method_id(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    jmethodID mid = (*env)->GetMethodID(env, clazz, name, sig);
    if (!mid) {
        dbg(lvl_error, "Failed to get method %s", name);
    }
    return mid;
}

static jclass find_class(JNIEnv *env, const char *name) {
    jclass clazz = (*env)->FindClass(env, name);
    if (!clazz) {
        dbg(lvl_error, "Failed to find class %s", name);
    }
    return clazz;
}

struct aprs_sdr_android *aprs_sdr_android_new(JNIEnv *env, jobject context) {
    struct aprs_sdr_android *android_usb;
    jclass context_class;
    jmethodID get_system_service_method;
    jstring usb_service_name;
    
    if (!env || !context) {
        dbg(lvl_error, "aprs_sdr_android_new: NULL parameters");
        return NULL;
    }
    
    android_usb = g_new0(struct aprs_sdr_android, 1);
    android_usb->env = env;
    android_usb->context = (*env)->NewGlobalRef(env, context);
    android_usb->device_fd = -1;
    
    /* Get USB manager from context */
    context_class = (*env)->GetObjectClass(env, context);
    get_system_service_method = get_method_id(env, context_class, 
                                             "getSystemService",
                                             "(Ljava/lang/String;)Ljava/lang/Object;");
    
    usb_service_name = (*env)->NewStringUTF(env, "usb");
    android_usb->usb_manager = (*env)->CallObjectMethod(env, context,
                                                        get_system_service_method,
                                                        usb_service_name);
    (*env)->DeleteLocalRef(env, usb_service_name);
    (*env)->DeleteLocalRef(env, context_class);
    
    if (!android_usb->usb_manager) {
        dbg(lvl_error, "Failed to get USB manager");
        aprs_sdr_android_destroy(android_usb);
        return NULL;
    }
    
    android_usb->usb_manager = (*env)->NewGlobalRef(env, android_usb->usb_manager);
    
    /* Get USB manager class and methods */
    android_usb->usb_manager_class = find_class(env, "android/hardware/usb/UsbManager");
    if (android_usb->usb_manager_class) {
        android_usb->usb_manager_class = (jclass)(*env)->NewGlobalRef(env, android_usb->usb_manager_class);
        android_usb->get_device_list_method = get_method_id(env, android_usb->usb_manager_class,
                                                           "getDeviceList",
                                                           "()Ljava/util/HashMap;");
        android_usb->has_permission_method = get_method_id(env, android_usb->usb_manager_class,
                                                           "hasPermission",
                                                           "(Landroid/hardware/usb/UsbDevice;)Z");
        android_usb->open_device_method = get_method_id(env, android_usb->usb_manager_class,
                                                        "openDevice",
                                                        "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;");
    }
    
    /* Get USB device class */
    android_usb->usb_device_class = find_class(env, "android/hardware/usb/UsbDevice");
    if (android_usb->usb_device_class) {
        android_usb->usb_device_class = (jclass)(*env)->NewGlobalRef(env, android_usb->usb_device_class);
    }
    
    /* Get USB device connection class and methods */
    android_usb->usb_device_connection_class = find_class(env, "android/hardware/usb/UsbDeviceConnection");
    if (android_usb->usb_device_connection_class) {
        android_usb->usb_device_connection_class = (jclass)(*env)->NewGlobalRef(env, android_usb->usb_device_connection_class);
        android_usb->bulk_transfer_method = get_method_id(env, android_usb->usb_device_connection_class,
                                                          "bulkTransfer",
                                                          "(Landroid/hardware/usb/UsbEndpoint;[BI)I");
        android_usb->control_transfer_method = get_method_id(env, android_usb->usb_device_connection_class,
                                                             "controlTransfer",
                                                             "(IIII[BII)I");
    }
    
    dbg(lvl_info, "Android USB manager initialized");
    return android_usb;
}

void aprs_sdr_android_destroy(struct aprs_sdr_android *android_usb) {
    if (!android_usb) return;
    
    aprs_sdr_android_close_device(android_usb);
    
    if (android_usb->usb_connection) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_connection);
    }
    if (android_usb->usb_device) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_device);
    }
    if (android_usb->usb_manager) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_manager);
    }
    if (android_usb->context) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->context);
    }
    if (android_usb->usb_manager_class) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_manager_class);
    }
    if (android_usb->usb_device_class) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_device_class);
    }
    if (android_usb->usb_device_connection_class) {
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_device_connection_class);
    }
    
    g_free(android_usb);
}

int aprs_sdr_android_request_permission(struct aprs_sdr_android *android_usb,
                                       int vendor_id, int product_id) {
    /* Permission requests must be handled by the Android activity */
    /* This function checks if permission is already granted */
    if (!android_usb || !android_usb->usb_device) {
        return 0;
    }
    
    jboolean has_permission = (*android_usb->env)->CallBooleanMethod(android_usb->env,
                                                                     android_usb->usb_manager,
                                                                     android_usb->has_permission_method,
                                                                     android_usb->usb_device);
    
    return has_permission ? 1 : 0;
}

int aprs_sdr_android_open_device(struct aprs_sdr_android *android_usb,
                                 int vendor_id, int product_id) {
    jobject device_list;
    jobject device_iterator;
    jclass hash_map_class;
    jclass iterator_class;
    jmethodID entry_set_method;
    jmethodID iterator_method;
    jmethodID has_next_method;
    jmethodID next_method;
    jmethodID get_key_method;
    jmethodID get_vendor_id_method;
    jmethodID get_product_id_method;
    
    if (!android_usb) {
        return -1;
    }
    
    android_usb->vendor_id = vendor_id;
    android_usb->product_id = product_id;
    
    /* Get device list from USB manager */
    device_list = (*android_usb->env)->CallObjectMethod(android_usb->env,
                                                       android_usb->usb_manager,
                                                       android_usb->get_device_list_method);
    
    if (!device_list) {
        dbg(lvl_warning, "No USB devices found");
        return -1;
    }
    
    /* Iterate through devices to find RTL-SDR */
    hash_map_class = (*android_usb->env)->GetObjectClass(android_usb->env, device_list);
    entry_set_method = get_method_id(android_usb->env, hash_map_class,
                                    "entrySet", "()Ljava/util/Set;");
    jobject entry_set = (*android_usb->env)->CallObjectMethod(android_usb->env,
                                                              device_list,
                                                              entry_set_method);
    
    iterator_class = find_class(android_usb->env, "java/util/Iterator");
    jmethodID iterator_method_id = get_method_id(android_usb->env, iterator_class,
                                                  "iterator", "()Ljava/util/Iterator;");
    device_iterator = (*android_usb->env)->CallObjectMethod(android_usb->env,
                                                             entry_set,
                                                             iterator_method_id);
    
    has_next_method = get_method_id(android_usb->env, iterator_class,
                                    "hasNext", "()Z");
    next_method = get_method_id(android_usb->env, iterator_class,
                               "next", "()Ljava/lang/Object;");
    
    get_vendor_id_method = get_method_id(android_usb->env, android_usb->usb_device_class,
                                         "getVendorId", "()I");
    get_product_id_method = get_method_id(android_usb->env, android_usb->usb_device_class,
                                          "getProductId", "()I");
    
    while ((*android_usb->env)->CallBooleanMethod(android_usb->env, device_iterator, has_next_method)) {
        jobject entry = (*android_usb->env)->CallObjectMethod(android_usb->env, device_iterator, next_method);
        jclass map_entry_class = find_class(android_usb->env, "java/util/Map$Entry");
        get_key_method = get_method_id(android_usb->env, map_entry_class,
                                      "getKey", "()Ljava/lang/Object;");
        jobject device = (*android_usb->env)->CallObjectMethod(android_usb->env, entry, get_key_method);
        
        jint dev_vendor_id = (*android_usb->env)->CallIntMethod(android_usb->env, device, get_vendor_id_method);
        jint dev_product_id = (*android_usb->env)->CallIntMethod(android_usb->env, device, get_product_id_method);
        
        if (dev_vendor_id == vendor_id && 
            (dev_product_id == product_id || product_id == 0)) {
            android_usb->usb_device = (*android_usb->env)->NewGlobalRef(android_usb->env, device);
            dbg(lvl_info, "Found RTL-SDR device: VID=0x%04x PID=0x%04x", dev_vendor_id, dev_product_id);
            break;
        }
    }
    
    (*android_usb->env)->DeleteLocalRef(android_usb->env, device_list);
    (*android_usb->env)->DeleteLocalRef(android_usb->env, entry_set);
    (*android_usb->env)->DeleteLocalRef(android_usb->env, device_iterator);
    
    if (!android_usb->usb_device) {
        dbg(lvl_warning, "RTL-SDR device not found");
        return -1;
    }
    
    /* Check permission */
    if (!aprs_sdr_android_request_permission(android_usb, vendor_id, product_id)) {
        dbg(lvl_error, "USB device permission not granted");
        return -1;
    }
    
    /* Open device */
    android_usb->usb_connection = (*android_usb->env)->CallObjectMethod(android_usb->env,
                                                                         android_usb->usb_manager,
                                                                         android_usb->open_device_method,
                                                                         android_usb->usb_device);
    
    if (!android_usb->usb_connection) {
        dbg(lvl_error, "Failed to open USB device");
        return -1;
    }
    
    android_usb->usb_connection = (*android_usb->env)->NewGlobalRef(android_usb->env, android_usb->usb_connection);
    
    /* Get file descriptor */
    jclass file_descriptor_class = find_class(android_usb->env, "java/io/FileDescriptor");
    jmethodID get_fd_method = get_method_id(android_usb->env, file_descriptor_class,
                                           "getInt$", "()I");
    jobject fd_obj = (*android_usb->env)->CallObjectMethod(android_usb->env,
                                                           android_usb->usb_connection,
                                                           get_method_id(android_usb->env,
                                                                        android_usb->usb_device_connection_class,
                                                                        "getFileDescriptor",
                                                                        "()Ljava/io/FileDescriptor;"));
    
    if (fd_obj) {
        android_usb->device_fd = (*android_usb->env)->CallIntMethod(android_usb->env, fd_obj, get_fd_method);
        dbg(lvl_info, "USB device opened, FD=%d", android_usb->device_fd);
    }
    
    return android_usb->device_fd;
}

void aprs_sdr_android_close_device(struct aprs_sdr_android *android_usb) {
    if (!android_usb) return;
    
    if (android_usb->usb_connection) {
        jmethodID close_method = get_method_id(android_usb->env,
                                               android_usb->usb_device_connection_class,
                                               "close", "()V");
        if (close_method) {
            (*android_usb->env)->CallVoidMethod(android_usb->env,
                                               android_usb->usb_connection,
                                               close_method);
        }
        (*android_usb->env)->DeleteGlobalRef(android_usb->env, android_usb->usb_connection);
        android_usb->usb_connection = NULL;
    }
    
    android_usb->device_fd = -1;
}

int aprs_sdr_android_bulk_read(struct aprs_sdr_android *android_usb,
                               unsigned char *buffer, int length) {
    /* For Android, we use the file descriptor directly with read() */
    /* The USB bulk transfer is handled by the kernel */
    if (!android_usb || android_usb->device_fd < 0 || !buffer) {
        return -1;
    }
    
    int n_read = read(android_usb->device_fd, buffer, length);
    if (n_read < 0) {
        dbg(lvl_debug, "USB bulk read error: %s", strerror(errno));
    }
    
    return n_read;
}

int aprs_sdr_android_control_transfer(struct aprs_sdr_android *android_usb,
                                     int request_type, int request,
                                     int value, int index,
                                     unsigned char *data, int length) {
    if (!android_usb || !android_usb->usb_connection) {
        return -1;
    }
    
    jbyteArray jdata = NULL;
    if (data && length > 0) {
        jdata = (*android_usb->env)->NewByteArray(android_usb->env, length);
        (*android_usb->env)->SetByteArrayRegion(android_usb->env, jdata, 0, length, (jbyte *)data);
    }
    
    jint result = (*android_usb->env)->CallIntMethod(android_usb->env,
                                                    android_usb->usb_connection,
                                                    android_usb->control_transfer_method,
                                                    request_type, request, value, index,
                                                    jdata, length, 0);
    
    if (jdata) {
        if (result > 0 && (request_type & 0x80)) {
            /* IN transfer - copy data back */
            (*android_usb->env)->GetByteArrayRegion(android_usb->env, jdata, 0, result, (jbyte *)data);
        }
        (*android_usb->env)->DeleteLocalRef(android_usb->env, jdata);
    }
    
    return (result >= 0) ? result : -1;
}

int aprs_sdr_android_is_device_connected(struct aprs_sdr_android *android_usb) {
    return (android_usb && android_usb->device_fd >= 0) ? 1 : 0;
}

#else /* ANDROID */

/* Stub implementations for non-Android platforms */
struct aprs_sdr_android *aprs_sdr_android_new(void *env, void *context) { return NULL; }
void aprs_sdr_android_destroy(struct aprs_sdr_android *android_usb) { }
int aprs_sdr_android_request_permission(struct aprs_sdr_android *android_usb, int vid, int pid) { return 0; }
int aprs_sdr_android_open_device(struct aprs_sdr_android *android_usb, int vid, int pid) { return -1; }
void aprs_sdr_android_close_device(struct aprs_sdr_android *android_usb) { }
int aprs_sdr_android_bulk_read(struct aprs_sdr_android *android_usb, unsigned char *buf, int len) { return -1; }
int aprs_sdr_android_control_transfer(struct aprs_sdr_android *android_usb, int rt, int r, int v, int i, unsigned char *d, int l) { return -1; }
int aprs_sdr_android_is_device_connected(struct aprs_sdr_android *android_usb) { return 0; }

#endif /* ANDROID */

