#ifndef SERIAL_IO_H_INCLUDED
#define SERIAL_IO_H_INCLUDED

int serial_io_init( const char* port, const char* strsettings );
int serial_io_read(int fd, char * buffer, int buffer_size );
int WriteSerial(int fd, const char * buffer);
void serial_io_shutdown(int fd );

#endif
