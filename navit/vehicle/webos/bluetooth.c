/* vim: sw=3 ts=3
 * */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <PDL.h>
#include "SDL.h"
#include "debug.h"
#include "callback.h"
#include "event.h"
#include "cJSON.h"
#include "vehicle_webos.h"
#include "bluetooth.h"

static int buffer_size = 128;
static void vehicle_webos_spp_init_read(struct vehicle_priv *priv, unsigned int length);

/********************************************************************/

static void mlPDL_ServiceCall_callback(struct callback_list *cbl, char *service,
                                       char *parameters/*, struct callback *fail_cb*/) {
    PDL_Err err;
    dbg(lvl_debug,"PDL_ServiceCall(%s) parameters(%s)",service,parameters);
    err = PDL_ServiceCall(service, parameters);
    if (err != PDL_NOERROR) {
        dbg(lvl_error,"PDL_ServiceCall to (%s) with (%s) failed with (%d): (%s)", service, parameters, err, PDL_GetError());
    }

    callback_list_destroy(cbl);
    g_free(service);
    g_free(parameters);
}

static void mlPDL_ServiceCall(const char *service, const char *parameters/*, struct callback *fail_cb = NULL*/) {
    struct callback *cb = NULL;
    struct callback_list *cbl = NULL;

    char *service2 = g_strdup(service);
    char *parameters2 = g_strdup(parameters);

    cbl = callback_list_new();
    cb	= callback_new_3(callback_cast(mlPDL_ServiceCall_callback),cbl,service2,parameters2);

    callback_list_add(cbl, cb);

    dbg(lvl_debug,"event_call_callback(%p)",cbl);
    event_call_callback(cbl);
}

/********************************************************************/

static void mlPDL_ServiceCallWithCallback_callback(struct callback_list *cbl,
        char *service,
        char *parameters,
        PDL_ServiceCallbackFunc callback,
        void *user,
        PDL_bool removeAfterResponse) {
    PDL_Err err;
    dbg(lvl_debug,"PDL_ServiceCallWithCallback(%s) parameters(%s)",service,parameters);
    err = PDL_ServiceCallWithCallback(service, parameters, callback, user, removeAfterResponse);
    if (err != PDL_NOERROR) {
        dbg(lvl_error,"PDL_ServiceCallWithCallback to (%s) with (%s) failed with (%d): (%s)", service, parameters, err,
            PDL_GetError());
    }

    callback_list_destroy(cbl);
    g_free(service);
    g_free(parameters);
}

static void mlPDL_ServiceCallWithCallback(const char *service,
        const char *parameters,
        PDL_ServiceCallbackFunc callback,
        void *user,
        PDL_bool removeAfterResponse) {
    struct callback *cb = NULL;
    struct callback_list *cbl = NULL;

    char *service2 = g_strdup(service);
    char *parameters2 = g_strdup(parameters);

    cbl = callback_list_new();
    cb	= callback_new_args(callback_cast(mlPDL_ServiceCallWithCallback_callback),6,cbl,service2,parameters2,callback,user,
                            removeAfterResponse);

    callback_list_add(cbl, cb);

    dbg(lvl_debug,"event_call_callback(%p)",cbl);
    event_call_callback(cbl);
}

/********************************************************************/

static void vehicle_webos_init_pdl_locationtracking_callback(struct vehicle_priv *priv, struct callback_list *cbl,
        int param) {
    PDL_Err err;

    priv->gps_type = param ? GPS_TYPE_INT: GPS_TYPE_NONE;

    dbg(lvl_debug,"Calling PDL_EnableLocationTracking(%i)",param);
    err = PDL_EnableLocationTracking(param);

    if (err != PDL_NOERROR) {
        dbg(lvl_error,"PDL_EnableLocationTracking failed with (%d): (%s)", err, PDL_GetError());
//		vehicle_webos_close(priv);
//		return 0;
    }

    callback_list_destroy(cbl);
}

static void vehicle_webos_init_pdl_locationtracking(struct vehicle_priv *priv, int param) {
    struct callback *cb = NULL;
    struct callback_list *cbl = NULL;

    cbl = callback_list_new();
    cb	= callback_new_3(callback_cast(vehicle_webos_init_pdl_locationtracking_callback),priv,cbl,param);

    callback_list_add(cbl, cb);

    event_call_callback(cbl);
}

/********************************************************************/

static int vehicle_webos_parse_nmea(struct vehicle_priv *priv, char *buffer) {
    char *nmea_data_buf, *p, *item[32];
    double lat, lng;
    int i, bcsum;
    int len = strlen(buffer);
    unsigned char csum = 0;
    int valid=0;
    int ret = 0;

    dbg(lvl_info, "enter: buffer='%s'", buffer);
    for (;;) {
        if (len < 4) {
            dbg(lvl_error, "'%s' too short", buffer);
            return ret;
        }
        if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n') {
            buffer[--len] = '\0';
            if (buffer[len - 1] == '\r')
                buffer[--len] = '\0';
        } else
            break;
    }
    if (buffer[0] != '$') {
        dbg(lvl_error, "no leading $ in '%s'", buffer);
        return ret;
    }
    if (buffer[len - 3] != '*') {
        dbg(lvl_error, "no *XX in '%s'", buffer);
        return ret;
    }
    for (i = 1; i < len - 3; i++) {
        csum ^= (unsigned char) (buffer[i]);
    }
    if (!sscanf(buffer + len - 2, "%x", &bcsum) /*&& priv->checksum_ignore != 2*/) {
        dbg(lvl_error, "no checksum in '%s'", buffer);
        return ret;
    }
    if (bcsum != csum /*&& priv->checksum_ignore == 0*/) {
        dbg(lvl_error, "wrong checksum in '%s was %x should be %x'", buffer,bcsum,csum);
        return ret;
    }

    if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
        nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, "\n", NULL);
        g_free(priv->nmea_data_buf);
        priv->nmea_data_buf=nmea_data_buf;
    } else {
        dbg(lvl_error, "nmea buffer overflow, discarding '%s'", buffer);
    }
    i = 0;
    p = buffer;
    while (i < 31) {
        item[i++] = p;
        while (*p && *p != ',')
            p++;
        if (!*p)
            break;
        *p++ = '\0';
    }

//	if (buffer[0] == '$') {
//		struct timeval tv;
//		gettimeofday(&tv,NULL);

    priv->delta = 0; //			(unsigned int)difftime(tv.tv_sec, priv->fix_time);
//		priv->fix_time = tv.tv_sec;
//		dbg(lvl_info,"delta(%i)",priv->delta);
//	}

    if (!strncmp(&buffer[3], "GGA", 3)) {
        /*                                                           1 1111
           0      1          2         3 4          5 6 7  8   9     0 1234
           $GPGGA,184424.505,4924.2811,N,01107.8846,E,1,05,2.5,408.6,M,,,,0000*0C
           UTC of Fix[1],Latitude[2],N/S[3],Longitude[4],E/W[5],Quality(0=inv,1=gps,2=dgps)[6],Satelites used[7],
           HDOP[8],Altitude[9],"M"[10],height of geoid[11], "M"[12], time since dgps update[13], dgps ref station [14]
         */
        if (*item[2] && *item[3] && *item[4] && *item[5]) {
            lat = g_ascii_strtod(item[2], NULL);
            priv->geo.lat = floor(lat / 100);
            lat -= priv->geo.lat * 100;
            priv->geo.lat += lat / 60;

            if (!g_strcasecmp(item[3],"S"))
                priv->geo.lat=-priv->geo.lat;

            lng = g_ascii_strtod(item[4], NULL);
            priv->geo.lng = floor(lng / 100);
            lng -= priv->geo.lng * 100;
            priv->geo.lng += lng / 60;

            if (!g_strcasecmp(item[5],"W"))
                priv->geo.lng=-priv->geo.lng;
            priv->valid=attr_position_valid_valid;
            dbg(lvl_info, "latitude '%2.4f' longitude %2.4f", priv->geo.lat, priv->geo.lng);

        } else
            priv->valid=attr_position_valid_invalid;
        if (*item[6])
            sscanf(item[6], "%d", &priv->status);
        if (*item[7])
            sscanf(item[7], "%d", &priv->sats_used);
        if (*item[8])
            sscanf(item[8], "%lf", &priv->hdop);
        if (*item[1]) {
            struct tm tm;
            strptime(item[1],"%H%M%S",&tm);
            priv->fix_time = mktime(&tm);
        }

        if (*item[9])
            sscanf(item[9], "%lf", &priv->altitude);

        g_free(priv->nmea_data);
        priv->nmea_data=priv->nmea_data_buf;
        priv->nmea_data_buf=NULL;
        ret = 1;
    }
    if (!strncmp(&buffer[3], "VTG", 3)) {
        /* 0      1      2 34 5    6 7   8
           $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A
           Course Over Ground Degrees True[1],"T"[2],Course Over Ground Degrees Magnetic[3],"M"[4],
           Speed in Knots[5],"N"[6],"Speed in KM/H"[7],"K"[8]
         */
        if (item[1] && item[7])
            valid = 1;
        if (i >= 10 && (*item[9] == 'A' || *item[9] == 'D'))
            valid = 1;
        if (valid) {
            priv->track = g_ascii_strtod( item[1], NULL );
            priv->speed = g_ascii_strtod( item[7], NULL );
            dbg(lvl_info,"direction %lf, speed %2.1lf", priv->track, priv->speed);
        }
    }
    if (!strncmp(&buffer[3], "RMC", 3)) {
        /*                                                           1     1
           0      1      2 3        4 5         6 7     8     9      0     1
           $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
           Time[1],Active/Void[2],lat[3],N/S[4],long[5],W/E[6],speed in knots[7],track angle[8],date[9],
           magnetic variation[10],magnetic variation direction[11]
         */
        if (*item[2] == 'A')
            valid = 1;
        if (i >= 13 && (*item[12] == 'A' || *item[12] == 'D'))
            valid = 1;
        if (valid) {
            priv->track = g_ascii_strtod( item[8], NULL );
            priv->speed = g_ascii_strtod( item[7], NULL );
            priv->speed *= 1.852;

            struct tm tm;
            char time[13];

            sprintf(time,"%s%s",item[1],item[9]);

            strptime(time,"%H%M%S%d%m%y",&tm);

            priv->fix_time = mktime(&tm);
        }
        ret = 1;
    }
    if (!strncmp(buffer, "$GPGSV", 6) && i >= 4) {
        /*
        	0 GSV	   Satellites in view
        	1 2 	   Number of sentences for full data
        	2 1 	   sentence 1 of 2
        	3 08	   Number of satellites in view

        	4 01	   Satellite PRN number
        	5 40	   Elevation, degrees
        	6 083	   Azimuth, degrees
        	7 46	   SNR - higher is better
        		   for up to 4 satellites per sentence
        	*75	   the checksum data, always begins with *
        */
        if (item[3]) {
            sscanf(item[3], "%d", &priv->sats_visible);
        }
    }
    if (!strncmp(buffer, "$IISMD", 6)) {
        /*
        	0      1   2     3      4
        	$IISMD,dir,press,height,temp*CC"
        		dir 	  Direction (0-359)
        		press	  Pressure (hpa, i.e. 1032)
        		height    Barometric height above ground (meter)
        		temp      Temperature (Degree Celsius)
        */
        if (item[1]) {
            priv->magnetic_direction = g_ascii_strtod( item[1], NULL );
            dbg(lvl_debug,"magnetic %d", priv->magnetic_direction);
        }
    }
    return ret;
}

static void vehicle_webos_spp_handle_read(PDL_ServiceParameters *params, void *user) {
    struct vehicle_priv *priv = user;
    int size, rc = 0;
    char *str, *tok;

    //PDL_Err err;
    size = PDL_GetParamInt(params, "dataLength");
    if (size > buffer_size) {
        dbg(lvl_error, "read returned too much data");
        return;
    }

    char buffer[buffer_size];

    PDL_GetParamString(params,"data",buffer,buffer_size);
    dbg(lvl_debug,"data(%s) dataLength(%i)",buffer,size);

    memmove(priv->buffer + priv->buffer_pos, buffer, size);



    priv->buffer_pos += size;
    priv->buffer[priv->buffer_pos] = '\0';
    dbg(lvl_debug, "size=%d pos=%d buffer='%s'", size,
        priv->buffer_pos, priv->buffer);
    str = priv->buffer;
    while ((tok = strchr(str, '\n'))) {
        *tok++ = '\0';
        dbg(lvl_debug, "line='%s'", str);
        rc += vehicle_webos_parse_nmea(priv, str);
        str = tok;
//		if (priv->file_type == file_type_file && rc)
//			break;
    }

    if (str != priv->buffer) {
        size = priv->buffer + priv->buffer_pos - str;
        memmove(priv->buffer, str, size + 1);
        priv->buffer_pos = size;
        dbg(lvl_debug,"now pos=%d buffer='%s'",
            priv->buffer_pos, priv->buffer);
    } else if (priv->buffer_pos == buffer_size - 1) {
        dbg(lvl_error,"Overflow. Most likely wrong baud rate or no nmea protocol");
        priv->buffer_pos = 0;
    }
    if (rc) {
        SDL_Event event;
        SDL_UserEvent userevent;

        userevent.type = SDL_USEREVENT;
        userevent.code = PDL_GPS_UPDATE;
        userevent.data1 = NULL;
        userevent.data2 = NULL;

        event.type = SDL_USEREVENT;
        event.user = userevent;

        SDL_PushEvent(&event);
    }

    vehicle_webos_spp_init_read(priv, buffer_size - priv->buffer_pos - 1);
}

static void vehicle_webos_spp_init_read(struct vehicle_priv *priv, unsigned int length) {
    //PDL_Err err;
    char parameters[128];

    snprintf(parameters, sizeof(parameters), "{\"instanceId\":%i, \"dataLength\":%i}", priv->spp_instance_id, length);
    mlPDL_ServiceCallWithCallback("palm://com.palm.service.bluetooth.spp/read",
                                  parameters,
                                  (PDL_ServiceCallbackFunc)vehicle_webos_spp_handle_read,
                                  priv,
                                  PDL_FALSE
                                 );
}

static void vehicle_webos_spp_handle_open(PDL_ServiceParameters *params, void *user) {
    struct vehicle_priv *priv = (struct vehicle_priv *)user;

    if (!priv->buffer)
        priv->buffer = g_malloc(buffer_size);

    dbg(lvl_debug,"instanceId(%i)",priv->spp_instance_id);

    priv->gps_type = GPS_TYPE_BT;

    vehicle_webos_spp_init_read(priv, buffer_size-1);
}

static void vehicle_webos_spp_notify(PDL_ServiceParameters *params, void *user) {
    struct vehicle_priv *priv = user;

    char notification[128];
    char parameters[128];

    const char *params_json = PDL_GetParamJson(params);
    dbg(lvl_info,"params_json(%s)", params_json);

    if (PDL_ParamExists(params, "errorText")) {
        PDL_GetParamString(params, "errorText", notification, sizeof(notification));
        dbg(lvl_error,"errorText(%s)",notification);
        return;
    }

    PDL_GetParamString(params, "notification", notification, sizeof(notification));
    notification[sizeof(notification)-1] = '\0';

    dbg(lvl_warning,"notification(%s) %i",notification,PDL_ParamExists(params, "notification"));

    if(strcmp(notification,"notifnservicenames") == 0) {
        int instance_id = PDL_GetParamInt(params, "instanceId");

        dbg(lvl_debug,"instanceId(%i)", instance_id);

        cJSON *root = cJSON_Parse(params_json);
        if (!root) {
            dbg(lvl_error,"parsing json failed");
            return;
        }

        cJSON *services = cJSON_GetObjectItem(root, "services");

        char *service_name = cJSON_GetArrayItem(services, 0)->valuestring;

        snprintf(parameters, sizeof(parameters), "{\"instanceId\":%i, \"servicename\":\"%s\"}",instance_id, service_name);
        mlPDL_ServiceCall("palm://com.palm.bluetooth/spp/selectservice", parameters);

        cJSON_Delete(root);
    } else if(strcmp(notification,"notifnconnected") == 0) {
        if (PDL_GetParamInt(params,"error") == 0) {
            vehicle_webos_init_pdl_locationtracking(priv, 0);

            int instance_id = PDL_GetParamInt(params, "instanceId");
            priv->spp_instance_id = instance_id;
            snprintf(parameters, sizeof(parameters), "{\"instanceId\":%i}", instance_id);
            mlPDL_ServiceCallWithCallback("palm://com.palm.service.bluetooth.spp/open",
                                          parameters,
                                          (PDL_ServiceCallbackFunc)vehicle_webos_spp_handle_open,
                                          priv,
                                          PDL_TRUE);
        } else {
            dbg(lvl_error,"notifnconnected error(%i)",PDL_GetParamInt(params,"error"));
        }
    } else if(strcmp(notification,"notifndisconnected") == 0) {
        priv->gps_type = GPS_TYPE_NONE;
        snprintf(parameters, sizeof(parameters), "{\"instanceId\":%i}",priv->spp_instance_id);
        mlPDL_ServiceCall("palm://com.palm.service.bluetooth.spp/close", parameters);
        priv->spp_instance_id = 0;
        vehicle_webos_init_pdl_locationtracking(priv, 1);
    }


}

static void vehicle_webos_init_bt_gps(struct vehicle_priv *priv, char *addr) {
    char parameters[128];

    dbg(lvl_debug,"subscribeNotifications");
    mlPDL_ServiceCallWithCallback("palm://com.palm.bluetooth/spp/subscribenotifications",
                                  "{\"subscribe\":true}",
                                  (PDL_ServiceCallbackFunc)vehicle_webos_spp_notify,
                                  priv,
                                  PDL_FALSE);

    snprintf(parameters, sizeof(parameters), "{\"address\":\"%s\"}", addr);
    mlPDL_ServiceCall("palm://com.palm.bluetooth/spp/connect", parameters);

    priv->spp_address = addr;
}

static void vehicle_webos_bt_gap_callback(PDL_ServiceParameters *params, void *param) {
    const char *params_json;
    struct vehicle_priv *priv = (struct vehicle_priv *)param;
    char *device_addr = NULL;
    cJSON *root;

    dbg(lvl_debug,"enter");

    PDL_Err err;
    err = PDL_GetParamInt(params, "errorCode");
    if (err != PDL_NOERROR) {
        dbg(lvl_error,"BT GAP Callback errorCode %d", err);
        return /*PDL_EOTHER*/;
    }

    params_json = PDL_GetParamJson(params);
    dbg(lvl_info,"params_json(%s)",params_json);

    root = cJSON_Parse(params_json);
    if (!root) {
        dbg(lvl_error,"parsing json failed");
        return;
    }

    cJSON *trusted_devices = cJSON_GetObjectItem(root, "trusteddevices");

    unsigned int i,c = cJSON_GetArraySize(trusted_devices);
    dbg(lvl_debug, "trusted_devices(%i)",c);
    for(i=0; i < c && !device_addr; i++) {
        cJSON *device = cJSON_GetArrayItem(trusted_devices,i);
        char *name = cJSON_GetObjectItem(device, "name")->valuestring;
        char *address = cJSON_GetObjectItem(device, "address")->valuestring;
        char *status = cJSON_GetObjectItem(device, "status")->valuestring;

        dbg(lvl_debug,"i(%i) name(%s) address(%s) status(%s)",i,name,address,status);

        if (/*strncmp(status, "connected",9) == 0 && */strstr(name, "GPS") != NULL) {
            dbg(lvl_debug,"choose name(%s) address(%s)",name,address);
            device_addr = g_strdup(address);
            break;
        }
    }

    cJSON_Delete(root);

    if (device_addr) {
        vehicle_webos_init_bt_gps(priv, device_addr);
    }

    g_free(device_addr);
}

int vehicle_webos_bt_open(struct vehicle_priv *priv) {
    // Try to connect to BT GPS, or use PDL method

    dbg(lvl_debug,"enter");

    PDL_Err err;
    err = PDL_ServiceCallWithCallback("palm://com.palm.bluetooth/gap/gettrusteddevices",
                                      "{}",
                                      (PDL_ServiceCallbackFunc)vehicle_webos_bt_gap_callback,
                                      priv,
                                      PDL_TRUE);
    if (err != PDL_NOERROR) {
        dbg(lvl_error,"PDL_ServiceCallWithCallback failed with (%d): (%s)", err, PDL_GetError());
        vehicle_webos_close(priv);
        return 0;
    }
    return 1;
}

void vehicle_webos_bt_close(struct vehicle_priv *priv) {
    dbg(lvl_debug,"XXX");
    char parameters[128];
    if (priv->spp_instance_id) {
        snprintf(parameters, sizeof(parameters), "{\"instanceId\":%i}", priv->spp_instance_id);
        PDL_ServiceCall("palm://com.palm.service.bluetooth.spp/close", parameters);
    }
    if (priv->spp_address) {
        snprintf(parameters, sizeof(parameters), "{\"address\":\"%s\"}", priv->spp_address);
        PDL_ServiceCall("palm://com.palm.bluetooth.spp/disconnect", parameters);
        g_free(priv->spp_address);
        priv->spp_address = NULL;
    }
    //g_free(priv->buffer);
    //priv->buffer = NULL;
    //		g_free(priv->nmea_data);
    //		priv->nmea_data = NULL;
    //		g_free(priv->nmea_data_buf);
    //		priv->nmea_data_buf = NULL;
}
