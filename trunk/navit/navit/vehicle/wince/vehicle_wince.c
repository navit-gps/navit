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
#include "event.h"
#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <winioctl.h>
#include <winbase.h>
#include <wchar.h>
#include "support/win32/ConvertUTF.h"

#define SwitchToThread() Sleep(0)

typedef int (WINAPI *PFN_BthSetMode)(DWORD pBthMode);
typedef int (WINAPI *PFN_BthGetMode)(DWORD* pBthMode);

char *_convert = NULL;
wchar_t *_wconvert = NULL;
#define W2A(lpw) (\
    ((LPCSTR)lpw == NULL) ? NULL : (\
          _convert = alloca(wcslen(lpw)+1),  wcstombs(_convert, lpw, wcslen(lpw) + 1), _convert) )

#define A2W(lpa) (\
    ((LPCSTR)lpa == NULL) ? NULL : (\
          _wconvert = alloca(strlen(lpa)*2+1),  mbstowcs(_wconvert, lpa, strlen(lpa) * 2 + 1), _wconvert) )

static void vehicle_wince_disable_watch(struct vehicle_priv *priv);
static void vehicle_wince_enable_watch(struct vehicle_priv *priv);
static int vehicle_wince_parse(struct vehicle_priv *priv, char *buffer);
static int vehicle_wince_open(struct vehicle_priv *priv);
static void vehicle_wince_close(struct vehicle_priv *priv);

static const int buffer_size = 256;

struct gps_sat {
	int prn;
	int elevation;
	int azimuth;
	int snr;
};

struct vehicle_priv {
	char *source;
	struct callback_list *cbl;
	int is_running;
	int thread_up;
	int fd;
	HANDLE			m_hGPSDevice;		// Handle to the device
	HANDLE			m_hGPSThread;		// Handle to the thread
	DWORD			m_dwGPSThread;		// Thread id

	char *buffer;
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
	int sats_signal;
	int time;
	int on_eof;
	int baudrate;
	struct attr ** attrs;
	char fixiso8601[128];
	int checksum_ignore;
	int use_file;
	HMODULE hBthDll;
	PFN_BthSetMode BthSetMode;
	int magnetic_direction;
	int current_count;
	struct gps_sat current[24];
	int next_count;
	struct gps_sat next[24];
	struct item sat_item;
	int valid;
};

static void initBth(struct vehicle_priv *priv)
{

	BOOL succeeded = FALSE;
	priv->hBthDll = LoadLibrary(TEXT("bthutil.dll"));
	if ( priv->hBthDll )
	{
		DWORD bthMode;
		PFN_BthGetMode BthGetMode  = (PFN_BthGetMode)GetProcAddress(priv->hBthDll, TEXT("BthGetMode") );

		if ( BthGetMode && BthGetMode(&bthMode) == ERROR_SUCCESS && bthMode == 0 )
		{
			priv->BthSetMode  = (PFN_BthSetMode)GetProcAddress(priv->hBthDll, TEXT("BthSetMode") );
			if( priv->BthSetMode &&  priv->BthSetMode(1) == ERROR_SUCCESS )
			{
				dbg(1, "bluetooth activated\n");
				succeeded = TRUE;
			}
		}

	}
	else
	{
		dbg(0, "Bluetooth library notfound\n");
	}

	if ( !succeeded )
	{

		dbg(1, "Bluetooth already enabled or failed to enable it.\n");
		priv->BthSetMode = NULL;
		if ( priv->hBthDll )
		{
			FreeLibrary(priv->hBthDll);
		}
	}
}


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
	// Com ports above 9 should be prefixed - $device/COM123
	dbg(0, "GPS Port:[%s]\n", pvt->source);
	pvt->thread_up = 1;
reconnect_port:
	/* GPD0 is the control port for the GPS driver */
	hGPS = CreateFile(L"GPD0:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hGPS != INVALID_HANDLE_VALUE) {
#ifndef IOCTL_SERVICE_REFRESH
#define IOCTL_SERVICE_REFRESH 0x4100000C
#endif
		DeviceIoControl(hGPS,IOCTL_SERVICE_REFRESH,0,0,0,0,0,0);
#ifndef IOCTL_SERVICE_START
#define IOCTL_SERVICE_START 0x41000004
#endif
		DeviceIoControl(hGPS,IOCTL_SERVICE_START,0,0,0,0,0,0);
		CloseHandle(hGPS);
	}

	while (pvt->is_running &&
		(pvt->m_hGPSDevice = CreateFile(A2W(pvt->source),
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
		dbg(1,"readfile\n");
		status = ReadFile(pvt->m_hGPSDevice,
			&buffer[current_index], chunk_size,
			&dwBytes, NULL);
		dbg(1,"status=%d dwBytes=%d\n",status,dwBytes);
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
			dbg(1,"buffer[%s]\n",buffer);
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
			dbg(1,"havedata=%d\n",havedata);
			if (havedata) {
				// post a message so we can callback from
				// the ui thread
				event_call_callback(pvt->cbl);
			}
		}
	}
	CloseHandle(pvt->m_hGPSDevice);
	pvt->m_hGPSDevice = NULL;
	pvt->thread_up = 0;
	return 0;
}

static int
vehicle_wince_available_ports(void)
{
	DWORD regkey_length = 20;
	DWORD regdevtype_length = 100;
	HKEY hkResult;
	HKEY hkSubResult;
	wchar_t keyname[regkey_length];
	wchar_t devicename[regkey_length];
	wchar_t devicetype[regdevtype_length];
	int index = 0;

	RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("Drivers\\Active"), 0, 0, &hkResult);
	while (RegEnumKeyEx( hkResult, index++, keyname, &regkey_length, NULL, NULL, NULL, NULL) == ERROR_SUCCESS )
	{
		if (RegOpenKeyEx( hkResult, keyname, 0, 0, &hkSubResult) == ERROR_SUCCESS )
		{
			regkey_length = 20;
			if ( RegQueryValueEx( hkSubResult,  L"Name", NULL, NULL, devicename, &regkey_length) == ERROR_SUCCESS )
			{
				if ( RegQueryValueEx( hkSubResult, L"Key", NULL, NULL, devicetype, &regdevtype_length) == ERROR_SUCCESS )
				{
					dbg(0, "Found device '%s' (%s)\n", W2A(devicename), W2A(devicetype));
				}
				else
				{
					dbg(0, "Found device '%s'\n", W2A(devicename));
				}
			}
			RegCloseKey(hkSubResult);
		}
		regkey_length = 20;
	}

	RegCloseKey(hkResult);
	return 0;
}


static int
vehicle_wince_open(struct vehicle_priv *priv)
{
	dbg(1, "enter vehicle_wince_open, priv->source='%s'\n", priv->source);

	if (priv->source ) {

		if ( strcmp(priv->source, "list") == 0 )
		{
			vehicle_wince_available_ports();
			return 0;
		}

		char* raw_setting_str = g_strdup( priv->source );
		char* strport = strchr(raw_setting_str, ':' );
		char* strsettings = strchr(raw_setting_str, ' ' );

		if (raw_setting_str && strport&&strsettings ) {
			strport++;
			*strsettings = '\0';
			strsettings++;

			dbg(0, "serial('%s', '%s')\n", strport, strsettings );
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
	if (priv->BthSetMode)
	{
		(void)priv->BthSetMode(0);
		FreeLibrary(priv->hBthDll);
	}
}

static int
vehicle_wince_parse(struct vehicle_priv *priv, char *buffer)
{
	char *nmea_data_buf, *p, *item[32];
	double lat, lng;
	int i, j, bcsum;
	int len = strlen(buffer);
	unsigned char csum = 0;
	int valid=0;
	int ret = 0;

	dbg(2, "enter: buffer='%s'\n", buffer);
	for (;;) {
		if (len < 4) {
			dbg(0, "'%s' too short\n", buffer);
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
	if (!sscanf(buffer + len - 2, "%x", &bcsum) && priv->checksum_ignore != 2) {
		dbg(0, "no checksum in '%s'\n", buffer);
		return ret;
	}
	if (bcsum != csum && priv->checksum_ignore == 0) {
		dbg(0, "wrong checksum in '%s'\n", buffer);
		return ret;
	}

	if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
		nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, "\n", NULL);
		g_free(priv->nmea_data_buf);
		priv->nmea_data_buf=nmea_data_buf;
	} else {
		dbg(0, "nmea buffer overflow, discarding '%s'\n", buffer);
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

	if (!strncmp(buffer, "$GPGGA", 6)) {
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

			if (!strcasecmp(item[3],"S"))
				priv->geo.lat=-priv->geo.lat;

			lng = g_ascii_strtod(item[4], NULL);
			priv->geo.lng = floor(lng / 100);
			lng -= priv->geo.lng * 100;
			priv->geo.lng += lng / 60;

			if (!strcasecmp(item[5],"W"))
				priv->geo.lng=-priv->geo.lng;
			priv->valid=attr_position_valid_valid;
            dbg(2, "latitude '%2.4f' longitude %2.4f\n", priv->geo.lat, priv->geo.lng);

		} else
			priv->valid=attr_position_valid_invalid;
		if (*item[6])
			sscanf(item[6], "%d", &priv->status);
		if (*item[7])
		sscanf(item[7], "%d", &priv->sats_used);
		if (*item[8])
			sscanf(item[8], "%lf", &priv->hdop);
		if (*item[1]) 
			strncpy(priv->fixtime, item[1], sizeof(priv->fixtime));
		if (*item[9])
			sscanf(item[9], "%lf", &priv->height);

		g_free(priv->nmea_data);
		priv->nmea_data=priv->nmea_data_buf;
		priv->nmea_data_buf=NULL;
	}
	if (!strncmp(buffer, "$GPVTG", 6)) {
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
			priv->direction = g_ascii_strtod( item[1], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
			dbg(2,"direction %lf, speed %2.1lf\n", priv->direction, priv->speed);
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
		if (i >= 13 && (*item[12] == 'A' || *item[12] == 'D'))
			valid = 1;
		if (valid) {
			priv->direction = g_ascii_strtod( item[8], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
			priv->speed *= 1.852;
			sscanf(item[9], "%02d%02d%02d",
				&priv->fixday,
				&priv->fixmonth,
				&priv->fixyear);
			priv->fixyear += 2000;
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
		j=4;
		while (j+4 <= i && priv->current_count < 24) {
			struct gps_sat *sat=&priv->next[priv->next_count++];
			sat->prn=atoi(item[j]);
			sat->elevation=atoi(item[j+1]);
			sat->azimuth=atoi(item[j+2]);
			sat->snr=atoi(item[j+3]);
			j+=4;
		}
		if (!strcmp(item[1], item[2])) {
			priv->sats_signal=0;
			for (i = 0 ; i < priv->next_count ; i++) {
				priv->current[i]=priv->next[i];
				if (priv->current[i].snr)
					priv->sats_signal++;
			}
			priv->current_count=priv->next_count;
			priv->next_count=0;
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
			strncpy(priv->fixtime, item[1], strlen(priv->fixtime));
			priv->fixday = atoi(item[2]);
			priv->fixmonth = atoi(item[3]);
			priv->fixyear = atoi(item[4]);
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
			dbg(1,"magnetic %d\n", priv->magnetic_direction);
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
	switch (type) {
	case attr_position_fix_type:
		attr->u.num = priv->status;
		break;
	case attr_position_height:
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_magnetic_direction:
		attr->u.num = priv->magnetic_direction;
		break;
	case attr_position_hdop:
		attr->u.numd = &priv->hdop;
		break;
	case attr_position_qual:
		attr->u.num = priv->sats_visible;
		break;
	case attr_position_sats_signal:
		attr->u.num = priv->sats_signal;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_nmea:
		attr->u.str=priv->nmea_data;
		if (! attr->u.str)
			return 0;
		break;
	case attr_position_time_iso8601:
		if (!priv->fixyear || !priv->fixtime[0])
			return 0;
		sprintf(priv->fixiso8601, "%04d-%02d-%02dT%.2s:%.2s:%sZ",
			priv->fixyear, priv->fixmonth, priv->fixday,
						priv->fixtime, (priv->fixtime+2), (priv->fixtime+4));
		attr->u.str=priv->fixiso8601;
		break;
	case attr_position_sat_item:
		dbg(0,"at here\n");
		priv->sat_item.id_lo++;
		if (priv->sat_item.id_lo > priv->current_count) {
			priv->sat_item.id_lo=0;
			return 0;
		}
		attr->u.item=&priv->sat_item;
		break;
	case attr_position_valid:
		attr->u.num=priv->valid;
		break;
	default:
		return 0;
	}
	if (type != attr_position_sat_item)
		priv->sat_item.id_lo=0;
	attr->type = type;
	return 1;
}

static int
vehicle_wince_sat_attr_get(void *priv_data, enum attr_type type, struct attr *attr)
{
	struct vehicle_priv *priv=priv_data;
	if (priv->sat_item.id_lo < 1)
		return 0;
	if (priv->sat_item.id_lo > priv->current_count)
		return 0;
	struct gps_sat *sat=&priv->current[priv->sat_item.id_lo-1];
	switch (type) {
	case attr_sat_prn:
		attr->u.num=sat->prn;
		break;
	case attr_sat_elevation:
		attr->u.num=sat->elevation;
		break;
	case attr_sat_azimuth:
		attr->u.num=sat->azimuth;
		break;
	case attr_sat_snr:
		attr->u.num=sat->snr;
		break;
	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

static struct item_methods vehicle_wince_sat_methods = {
	NULL,
	NULL,
	NULL,
	vehicle_wince_sat_attr_get,
};

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
	struct attr *checksum_ignore;
	struct attr *handle_bluetooth;
	char *cp;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->fd = -1;
	ret->cbl = cbl;
	cp = strchr(source->u.str,':');
	if (cp)
		cp++;
	else
		cp = source->u.str;
	ret->source = g_strdup(cp);
	ret->buffer = g_malloc(buffer_size);
	ret->time=1000;
	ret->baudrate=0;	// do not change the rate if not configured

	time = attr_search(attrs, NULL, attr_time);
	if (time)
		ret->time=time->u.num;
	baudrate = attr_search(attrs, NULL, attr_baudrate);
	if (baudrate) {
		ret->baudrate = baudrate->u.num;
	}
	checksum_ignore = attr_search(attrs, NULL, attr_checksum_ignore);
	if (checksum_ignore)
		ret->checksum_ignore=checksum_ignore->u.num;
	ret->attrs = attrs;
	on_eof = attr_search(attrs, NULL, attr_on_eof);
	if (on_eof && !strcasecmp(on_eof->u.str, "stop"))
		ret->on_eof=1;
	if (on_eof && !strcasecmp(on_eof->u.str, "exit"))
		ret->on_eof=2;
	dbg(0,"on_eof=%d\n", ret->on_eof);
	*meth = vehicle_wince_methods;
	ret->sat_item.type=type_position_sat;
	ret->sat_item.id_hi=ret->sat_item.id_lo=0;
	ret->sat_item.priv_data=ret;
	ret->sat_item.meth=&vehicle_wince_sat_methods;


	handle_bluetooth = attr_search(attrs, NULL, attr_bluetooth);
	if ( handle_bluetooth && handle_bluetooth->u.num == 1 )
		initBth(ret);

	if (vehicle_wince_open(ret)) {
		vehicle_wince_enable_watch(ret);
		return ret;
	}
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
