#ifndef SERIAL_IO_H_INCLUDED
#define SERIAL_IO_H_INCLUDED

// wxString EnumSerialPort(int n);
int serial_io_init( int port, int baudrate );
int serio_io_read(int fd, char * buffer, int buffer_size );
int WriteSerial(int fd, const char * buffer);
void serial_io_shutdown(int fd );
// wxString GetLastSerialIOError();


#endif
