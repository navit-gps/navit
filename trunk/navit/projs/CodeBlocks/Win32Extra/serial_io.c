#include <stdio.h>
#include <windows.h>
#include "serial_io.h"

int serial_io_init( const char* port, const char* strsettings )
{
    HANDLE hCom = NULL;

	char strport[16];
	snprintf( strport, sizeof( strport ), "\\\\.\\%s", port );

	hCom = CreateFile(
			strport,
			GENERIC_WRITE | GENERIC_READ,
			0,
			0,
			OPEN_EXISTING,
			0,
			NULL);

	if (hCom == INVALID_HANDLE_VALUE)
	{
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);
		// g_strSerialError = strPort + wxT(": ") + (LPTSTR) lpMsgBuf;

		// Free the buffer.
		LocalFree( lpMsgBuf );
		return -1;
	}

	DCB dcb;

	ZeroMemory(&dcb, sizeof(DCB));

	GetCommState(hCom, &dcb);

//    char strsettings[255];
//    snprintf( strsettings, sizeof( strsettings ), "baud=%d parity=N data=8 stop=1", baudrate );
	BuildCommDCB( strsettings, &dcb);

	SetupComm(hCom, 4096, 4096);

	SetCommState(hCom, &dcb);

	COMMTIMEOUTS sCT;

	memset(&sCT, 0, sizeof(sCT));
	sCT.ReadTotalTimeoutConstant = 10;

	SetCommTimeouts(hCom, &sCT);

    return (int)hCom;
}

int serial_io_read( int fd, char * buffer, int buffer_size )
{
	DWORD dwBytesIn = 0;

	if (fd <= 0)
	{
		// Sleep(1000);
		*buffer = 0;
		return 0;
	}

	ReadFile( (HANDLE)fd, buffer, buffer_size - 1, &dwBytesIn, NULL);

	if (dwBytesIn > 0)
	{
	    printf( "GPS < %s\n",buffer );
	}
	if (dwBytesIn >= 0)
	{
		buffer[dwBytesIn] = 0;
	}
	else{
        buffer[0] = 0;
	}
		buffer[buffer_size - 1] = 0;


	if (dwBytesIn <= 0)
	{
		Sleep(50);
		dwBytesIn = 0;
	}

	return dwBytesIn;
}

int serial_io_write(int fd, const char * buffer)
{
	DWORD dwBytesOut = 0;

	WriteFile((HANDLE)fd, buffer, strlen(buffer), &dwBytesOut, NULL);

	return dwBytesOut;
}

void serial_io_shutdown(int fd )
{
	if (fd > 0)
	{
		CloseHandle((HANDLE)fd);
	}
}
