#ifndef _WIN32_MMAN_H_INCLUDED
#define	_WIN32_MMAN_H_INCLUDED

void * mmap_readonly_win32( const char* name, long* map_handle_ptr, long* map_file_ptr );
void mmap_unmap_win32( void* mem_ptr, long map_handle, long map_file );

#endif /* !_WIN32_MMAN_H_INCLUDED */

