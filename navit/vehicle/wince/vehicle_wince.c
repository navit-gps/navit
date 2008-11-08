/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <winioctl.h>
#include <winbase.h>
#include "support/win32/ConvertUTF.h"

#define SwitchToThread() Sleep(0)

static void vehicle_wince_disable_watch(struct vehicle_priv *priv);
static void vehicle_wince_enable_watch(struct vehicle_priv *priv);
static int vehicle_wince_parse(struct vehicle_priv *priv, char *buffer);
static int vehicle_wince_open(struct vehicle_priv *priv);
static void vehicle_wince_close(struct vehicle_priv *priv);

static int buffer_size = 256;

struct vehicle_priv {
	char *source;
	struct callback_list *cbl;
	int is_running;
	int thread_up;
	int fd;
	HANDLE			m_hGPSDevice;		// Handle to the device
	HANDLE			m_hGPSThread;		// Handle to the thread
	DWORD			m_dwGPSThread;		// Thread id
	HANDLE			m_hEventPosition;	// Handle to the position event
	HANDLE			m_hEventState;		// Handle to the state event
	HANDLE			m_hEventStop;		// Handle to the stop event
	DWORD			m_dwError;			// Last error code

	char *buffer;
	int buffer_pos;
	char *nmea_data;
	char *nmea_data_buf;

	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	double hdop;
	double vdop;
	char fixtime[20];
	int fixyear;
	int fixmonth;
	int fixday;
	int status;
	int sats_used;
	int sats_visible;
	int time;
	int on_eof;
	int no_data_count;
	int baudrate;
	struct attr ** attrs;
	char fixiso8601[128];
};

static DWORD WINAPI wince_port_reader_thread (LPVOID lParam)
{
	struct vehicle_priv *pvt = lParam;
	COMMTIMEOUTS commTiming;
	BOOL status;
	static char buffer[10*82*4] = {0,};
	static int current_index = 0;
	const int chunk_size = 3*82;
	char* return_pos;
	DWORD dwBytes;
	char return_buffer[1024];
	int havedata;
	HANDLE hGPS;
	// FIXME
	wchar_t portname[] = TEXT("GPD1:");
	pvt->thread_up = 1;
reconnect_port:
	/* GPD0 is the control port for the GPS driver */
	hGPS = CreateFile(L"GPD0:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hGPS != INVALID_HANDLE_VALUE) {
		DeviceIoControl(hGPS,IOCTL_SERVICE_REFRESH,0,0,0,0,0,0);
		DeviceIoControl(hGPS,IOCTL_SERVICE_START,0,0,0,0,0,0);
		CloseHandle(hGPS);
	}

	while (pvt->is_running &&
		(pvt->m_hGPSDevice = CreateFile(portname, 
			GENERIC_READ, 0,
			NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
		Sleep(1000);
		dbg(0, "Waiting to connect to %s\n", pvt->source);
	}
	GetCommTimeouts (pvt->m_hGPSDevice, &commTiming);
	commTiming.ReadIntervalTimeout = 20;
	commTiming.ReadTotalTimeoutMultiplier = 0;
	commTiming.ReadTotalTimeoutConstant = 200;

	commTiming.WriteTotalTimeoutMultiplier=5;
	commTiming.WriteTotalTimeoutConstant=5;
	SetCommTimeouts (pvt->m_hGPSDevice, &commTiming);

	if (pvt->baudrate) {
		DCB portState;
		if (!GetCommState(pvt->m_hGPSDevice, &portState)) {
			MessageBox (NULL, TEXT ("GetCommState Error"), TEXT (""),
				MB_APPLMODAL|MB_OK);
			pvt->thread_up = 0;
			return(1);
		}
		portState.BaudRate = pvt->baudrate;
		if (!SetCommState(pvt->m_hGPSDevice, &portState)) {
			MessageBox (NULL, TEXT ("SetCommState Error"), TEXT (""),
				MB_APPLMODAL|MB_OK);
			pvt->thread_up = 0;
			return(1);
		}
	}
	while (pvt->is_running) {
		havedata = 0;
		if (current_index + chunk_size >= sizeof(buffer)) {
			// reset buffer
			dbg(0, "GPS buffer reset\n");
			current_index = 0;
			memset(buffer, 0, sizeof(buffer));
		}
		status = ReadFile(pvt->m_hGPSDevice, 
			&buffer[current_index], chunk_size,
			&dwBytes, NULL);
		if (!status) {
			CloseHandle(pvt->m_hGPSDevice);
			pvt->m_hGPSDevice = NULL;
			goto reconnect_port;
		}
		if (dwBytes > 0) {
			int pos;
			int consumed;
			return_pos = NULL;
			current_index += dwBytes;
			buffer[current_index+1] = '\0';
			//dbg(0, "buffer[%s]\n", buffer);
			while (buffer[0]) {
				pos = strcspn(buffer, "\r\n");
				if (pos >= sizeof(return_buffer)) {
					current_index = 0;
					break;
				}
				if (pos) {
					if (!buffer[pos])
						break;
					memcpy(return_buffer, buffer, pos);
					return_buffer[pos] = '\0';
					// dbg(0, "received %d : '%s' bytes to copy\n", bytes_to_copy, return_buffer );
					havedata += vehicle_wince_parse(pvt, return_buffer);
				}
				consumed = pos + strspn(buffer+pos, "\r\n");
				current_index -= consumed;
				memmove(buffer,&buffer[consumed] , 
					sizeof(buffer) - consumed);
				buffer[current_index] = '\0';
			}
			if (havedata) {
				// post a message so we can callback from
				// the ui thread
				event_call_callback(pvt->cbl);
			}
		} else {
			pvt->no_data_count++;
		}
	}
	CloseHandle(pvt->m_hGPSDevice);
	pvt->m_hGPSDevice = NULL;
	pvt->thread_up = 0;
	return 0;
}


static int
vehicle_wince_open(struct vehicle_priv *priv)
{
	dbg(1, "enter vehicle_wince_open, priv->source='%s'\n", priv->source);

	if (priv->source ) {
		char* raw_setting_str = g_strdup( priv->source );
		char* strport = strchr(raw_setting_str, ':' );
		char* strsettings = strchr(raw_setting_str, ' ' );

		if (raw_setting_str && strport&&strsettings ) {
			strport++;
			*strsettings = '\0';
			strsettings++;

			dbg(1, "serial('%s', '%s')\n", strport, strsettings );
		}
		if (raw_setting_str)
		g_free( raw_setting_str );
	}
	return 1;
}

static void
vehicle_wince_close(struct vehicle_priv *priv)
{
	vehicle_wince_disable_watch(priv);
}

static int
vehicle_wince_parse(struct vehicle_priv *priv, char *buffer)
{
	char *nmea_data_buf, *p, *item[16];
	double lat, lng;
	int i, bcsum;
	int len;
	unsigned char csum = 0;
	int ret = 0;
	int valid = 0;

	len = strlen(buffer);
	//dbg(0, "buffer='%s'\n", buffer);
	for (;;) {
		if (len < 4) {
			dbg(0, "'%s' too short\n", buffer);
			return ret;
		}
		if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')
			buffer[--len] = '\0';
		else
			break;
	}
	if (buffer[0] != '$') {
		dbg(0, "no leading $ in '%s'\n", buffer);
		return ret;
	}
	if (buffer[len - 3] != '*') {
		dbg(0, "no *XX in '%s'\n", buffer);
		return ret;
	}
	for (i = 1; i < len - 3; i++) {
		csum ^= (unsigned char) (buffer[i]);
	}
	if (!sscanf(buffer + len - 2, "%x", &bcsum)) {
		dbg(0, "no checksum in '%s'\n", buffer);
		return ret;
	}
	if (bcsum != csum) {
		dbg(0, "wrong checksum in '%s'\n", buffer);
		return ret;
	}
// RMC, RMB, VTG, and GLL in nmea 2.3 have status
// A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator. 
// Only A and D are valid
	if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
		nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, "\n", NULL);
		g_free(priv->nmea_data_buf);
		priv->nmea_data_buf=nmea_data_buf;
	} else {
		dbg(0, "nmea buffer overflow, discarding '%s'\n", buffer);
	}
	i = 0;
	p = buffer;
	while (i < 16) {
		item[i++] = p;
		while (*p && *p != ',')
			p++;
		if (!*p)
			break;
		*p++ = '\0';
	}
	if (0) {
		int j = 0;
		for (j=0; j < i; j++)
			dbg(0,"[%d] = %s\n", j, item[j]);
	}

	if (!strncmp(buffer, "$GPGGA", 6)) {
		/*                                                           1 1111
		   0      1          2         3 4          5 6 7  8   9     0 1234
		   $GPGGA,184424.505,4924.2811,N,01107.8846,E,1,05,2.5,408.6,M,,,,0000*0C
		   UTC of Fix[1],Latitude[2],N/S[3],Longitude[4],E/W[5],Quality(0=inv,1=gps,2=dgps)[6],Satelites used[7],
		   HDOP[8],Altitude[9],"M"[10],height of geoid[11], "M"[12], time since dgps update[13], dgps ref station [14]
		 */
		lat = strtod(item[2], NULL);
		priv->geo.lat = floor(lat / 100);
		lat -= priv->geo.lat * 100;
		priv->geo.lat += lat / 60;

		if (!strcasecmp(item[3],"S"))
			priv->geo.lat=-priv->geo.lat;

		lng = strtod(item[4], NULL);
		priv->geo.lng = floor(lng / 100);
		lng -= priv->geo.lng * 100;
		priv->geo.lng += lng / 60;

		if (!strcasecmp(item[5],"W"))
			priv->geo.lng=-priv->geo.lng;

		sscanf(item[6], "%d", &priv->status);
		sscanf(item[7], "%d", &priv->sats_used);
		// priv->fixtime = strdup(item[1]);//strtod(item[1], NULL);
		sscanf(item[8], "%f", &priv->hdop);
		strcpy(priv->fixtime, item[1]);
		sscanf(item[9], "%f", &priv->height);
		g_free(priv->nmea_data);
		priv->nmea_data=priv->nmea_data_buf;
		priv->nmea_data_buf=NULL;

		// callback_list_call_0(priv->cbl);
		return 1;
	}
	if (!strncmp(buffer, "$GPVTG", 6)) {
		
		/* 0      1      2 34 5    6 7   8
		   $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A
		   Course Over Ground Degrees True[1],"T"[2],Course Over Ground Degrees Magnetic[3],"M"[4],
		   Speed in Knots[5],"N"[6],"Speed in KM/H"[7],"K"[8]
		 */
		if (item[1] && item[7])
			valid = 1;
		if (i == 9 && (*item[9] == 'A' || *item[9] == 'D'))
			valid = 1;
		if (valid) {
			priv->direction = g_ascii_strtod( item[1], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
		}
	}
	if (!strncmp(buffer, "$GPRMC", 6)) {
		/*                                                           1     1
		   0      1      2 3        4 5         6 7     8     9      0     1
		   $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
		   Time[1],Active/Void[2],lat[3],N/S[4],long[5],W/E[6],speed in knots[7],track angle[8],date[9],
		   magnetic variation[10],magnetic variation direction[11]
		*/
		if (*item[2] == 'A')
			valid = 1;
		if (i == 12 && (*item[12] == 'A' || *item[12] == 'D'))
			valid = 1; 
		if (valid) {
			// FIXME speed is in knots .5144444444444444f
			priv->direction = g_ascii_strtod( item[8], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
			priv->speed *= 1.852;
			sscanf(item[9], "%02d%02d%02d",
				&priv->fixday,
				&priv->fixmonth,
				&priv->fixyear);
			priv->fixyear += 2000;
		}
	}
	if (!strncmp(buffer, "$GPGSA", 6)) {
	/*
		GSA      Satellite status
		A        Auto selection of 2D or 3D fix (M = manual) 
		3        3D fix - values include:	1 = no fix
							2 = 2D fix
							3 = 3D fix
		04,05... PRNs of satellites used for fix (space for 12) 
		2.5      PDOP (dilution of precision) 
		1.3      Horizontal dilution of precision (HDOP) 
		2.1      Vertical dilution of precision (VDOP)
		*39      the checksum data, always begins with *
	*/
	}
	if (!strncmp(buffer, "$GPGSV", 6)) {
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
	if (!strncmp(buffer, "$GPZDA", 6)) {
	/*
		0        1        2  3  4    5  6
		$GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC
			hhmmss    HrMinSec(UTC)
			dd,mm,yyy Day,Month,Year
			xx        local zone hours -13..13
			yy        local zone minutes 0..59
	*/
		if (item[1] && item[2] && item[3] && item[4]) {
			// priv->fixtime = atof(item[1]);
			strcpy(priv->fixtime, item[1]);
			priv->fixday = atoi(item[2]);
			priv->fixmonth = atoi(item[3]);
			priv->fixyear = atoi(item[4]);
		}
	}
	return ret;
}

static void
vehicle_wince_enable_watch(struct vehicle_priv *priv)
{
	vehicle_wince_disable_watch(priv);
	priv->is_running = 1;
	priv->m_hGPSThread = CreateThread(NULL, 0, wince_port_reader_thread,
			priv, 0, &priv->m_dwGPSThread);
	if (!priv->m_hGPSThread) {
		priv->is_running = 0;
		// error creating thread
		MessageBox (NULL, TEXT ("Can not create GPS reader thread"), TEXT (""),
			MB_APPLMODAL|MB_OK);
	}
}

static void
vehicle_wince_disable_watch(struct vehicle_priv *priv)
{
	int wait = 5000;
	priv->is_running = 0;
//	DWORD res;
//	res = WaitForSingleObject(priv->m_hGPSThread, 2000);
//	
	while (wait-- > 0 && priv->thread_up) {
		SwitchToThread();
	}
	if (priv->m_hGPSThread) {
		// Terminate reader, sorry
		TerminateThread(priv->m_hGPSThread, -1);
	}
}


static void
vehicle_wince_destroy(struct vehicle_priv *priv)
{
	vehicle_wince_close(priv);
	if (priv->source)
		g_free(priv->source);
	if (priv->buffer)
		g_free(priv->buffer);
	g_free(priv);
}

static int
vehicle_wince_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	struct attr * active=NULL;
	int check_status = 0;

	switch (type) {
	case attr_position_fix_type:
		attr->u.num = priv->status;
		break;
	case attr_position_height:
		check_status = 1;
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		check_status = 1;
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		check_status = 1;
		attr->u.numd = &priv->direction;
		break;
	case attr_position_hdop:
		attr->u.numd = &priv->hdop;
		break;
	case attr_position_qual:
		attr->u.num = priv->sats_visible;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		check_status = 1;
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_nmea:
		attr->u.str=priv->nmea_data;
		if (!attr->u.str)
			return 0;
		break;
	case attr_position_time_iso8601:
		if (!priv->fixyear || !priv->fixtime[0])
			return 0;
		sprintf(priv->fixiso8601, "%04d-%02d-%02dT%sZ",
			priv->fixyear, priv->fixmonth, priv->fixday,
			priv->fixtime);
		attr->u.str=priv->fixiso8601;
		break;
	case attr_active:
		if (active != NULL && active->u.num == 1)
			return 1;
		else
			return 0;
		break;
	default:
		return 0;
	}
	attr->type = type;
	if (check_status && 0 == priv->status)
		return 0;
	return 1;
}

struct vehicle_methods vehicle_wince_methods = {
	vehicle_wince_destroy,
	vehicle_wince_position_attr_get,
};

static struct vehicle_priv *
vehicle_wince_new(struct vehicle_methods
		      *meth, struct callback_list
		      *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source;
	struct attr *time;
	struct attr *on_eof;
	struct attr *baudrate;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->fd = -1;
	ret->cbl = cbl;
	ret->source = g_strdup(source->u.str);
	ret->buffer = g_malloc(buffer_size);
	ret->time=1000;
	ret->baudrate=0;	// do not change the rate if not configured
	// ret->fixtime = 0.0f;
	time = attr_search(attrs, NULL, attr_time);
	if (time)
		ret->time=time->u.num;
	baudrate = attr_search(attrs, NULL, attr_baudrate);
	if (baudrate) {
		ret->baudrate = baudrate->u.num;
	}
	ret->attrs = attrs;
	on_eof = attr_search(attrs, NULL, attr_on_eof);
	if (on_eof && !strcasecmp(on_eof->u.str, "stop"))
		ret->on_eof=1;
	if (on_eof && !strcasecmp(on_eof->u.str, "exit"))
		ret->on_eof=2;
	dbg(0,"on_eof=%d\n", ret->on_eof);
	*meth = vehicle_wince_methods;
	if (vehicle_wince_open(ret)) {
		vehicle_wince_enable_watch(ret);
		return ret;
	}
	ret->no_data_count = 0;
	dbg(0, "Failed to open '%s'\n", ret->source);
	vehicle_wince_destroy(ret);
	return NULL;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("wince", vehicle_wince_new);
}
