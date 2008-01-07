#include <windows.h>
#include "mman.h"

void * mmap_file_readonly( const char* name )
{
    void * mapped_ptr = NULL;

    OFSTRUCT   of;
    HFILE hFile = OpenFile (name, &of, OF_READ);

    if ( hFile != HFILE_ERROR )
    {
        HANDLE hMapping = CreateFileMapping ( (HANDLE)hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        mapped_ptr = MapViewOfFile(hMapping, FILE_MAP_READ, 0 , 0, 0 );
    }
    return mapped_ptr;
}

