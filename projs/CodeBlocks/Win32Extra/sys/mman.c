#include <windows.h>
#include "mman.h"

void * mmap_readonly_win32( const char* name, long* map_handle_ptr, long* map_file_ptr )
{
    void * mapped_ptr = NULL;

    OFSTRUCT   of;
    HFILE hFile = OpenFile (name, &of, OF_READ);

    *map_file_ptr = (long)hFile;
    *map_handle_ptr = 0;

    if ( hFile != HFILE_ERROR )
    {
        HANDLE hMapping = CreateFileMapping( (HANDLE)hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        mapped_ptr = MapViewOfFile(hMapping, FILE_MAP_READ, 0 , 0, 0 );
        *map_handle_ptr = (long)hMapping;
    }

    return mapped_ptr;
}

void mmap_unmap_win32( void* mem_ptr, long map_handle, long map_file )
{
    if ( mem_ptr != NULL )
    {
        UnmapViewOfFile( mem_ptr );
    }
    if ( map_handle != 0)
    {
        CloseHandle( (HANDLE)map_handle );
    }
    if ( map_file != 0  )
    {
        CloseHandle( (HANDLE)map_file );
    }
}
