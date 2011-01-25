#include <stdio.h>
#include <windows.h>
#include <glib.h>
#include "serial_io.h"
#include "debug.h"

//***************************************************************************
/** @fn int serial_io_init( const char* port, const char* strsettings )
*****************************************************************************
* @b Description: initialise a serial port communication
*****************************************************************************
* @param      port : port name
*                 example 'COM7'
* @param      strsettings : port settings
*                 example ; 'baud=115200 parity=N data=8 stop=1'
*****************************************************************************
* @return     file descriptor
*             -1 if error
*****************************************************************************
**/
int serial_io_init( const char* port, const char* strsettings )
{
    HANDLE hCom = NULL;
    DCB dcb;
    COMMTIMEOUTS sCT;

        char strport[16];
        g_snprintf( strport, sizeof( strport ), "\\\\.\\%s", port );

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
                dbg(1, "return (fd) : '-1' : serial_io_init error : '%s'\n", lpMsgBuf);

                LocalFree( lpMsgBuf );       // Free the buffer.
                return -1;
        }

        ZeroMemory(&dcb, sizeof(DCB));

        GetCommState(hCom, &dcb);

        BuildCommDCB( strsettings, &dcb);

        SetupComm(hCom, 4096, 4096);

        SetCommState(hCom, &dcb);

        memset(&sCT, 0, sizeof(sCT));
        sCT.ReadTotalTimeoutConstant = 10;

        SetCommTimeouts(hCom, &sCT);

        dbg(1, "serial_io_init return (fd) : '%d'\n", (int)hCom);

   return (int)hCom;
}

//***************************************************************************
/** @fn int serial_io_read( int fd, char * buffer, int buffer_size )
*****************************************************************************
* @b Description: Read bytes on the serial port
*****************************************************************************
* @param      fd : file descriptor
* @param      buffer : buffer for data
* @param      buffer_size : size in byte of the buffer
*****************************************************************************
* @return     number of bytes read
*****************************************************************************
* @remarks buffer must be allocated by the caller
*****************************************************************************
**/
int serial_io_read( int fd, char * buffer, int buffer_size )
{
        DWORD dwBytesIn = 0;
        dbg(1, "serial_io_read fd = %d buffer_size = %d\n", fd, buffer_size);


        if (fd <= 0)
        {
               dbg(0, "serial_io_read return (dwBytesIn) : '0'\n");
               *buffer = 0;
                return 0;
        }

        ReadFile( (HANDLE)fd, buffer, buffer_size - 1, &dwBytesIn, NULL);

        if (dwBytesIn >= 0)
        {
                buffer[dwBytesIn] = 0;
        }
        else
        {
            dwBytesIn = 0;
			buffer[0] = 0;
        }
        if (dwBytesIn > 0)
        {
            dbg(1,"GPS < %s\n",buffer );
        }
        buffer[buffer_size - 1] = 0;

        dbg(2, "serial_io_read return (dwBytesIn) : '%d'\n", dwBytesIn);
        return dwBytesIn;
}

//***************************************************************************
/** @fn int serial_io_write(int fd, const char * buffer)
*****************************************************************************
* @b Description: Write bytes on the serial port
*****************************************************************************
* @param      fd : file descriptor
* @param      buffer : data buffer (null terminated)
*****************************************************************************
* @return     number of bytes written
*****************************************************************************
**/
int serial_io_write(int fd, const char * buffer)
{
        DWORD dwBytesOut = 0;
        dbg(1, "serial_io_write fd = %d buffer = '%s'\n",fd, buffer);


        WriteFile((HANDLE)fd, buffer, strlen(buffer), &dwBytesOut, NULL);

        return dwBytesOut;
}

//***************************************************************************
/** @fn void serial_io_shutdown(int fd )
*****************************************************************************
* @b Description: Terminate serial communication
*****************************************************************************
* @param      fd : file descriptor
*****************************************************************************
**/
void serial_io_shutdown(int fd )
{
       dbg(1, "serial_io_shutdown fd = %d\n",fd);

        if (fd > 0)
        {
                CloseHandle((HANDLE)fd);
        }
}
