#ifndef __NAVIT_LFS_H__
/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2014 Navit Team
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

#ifdef BUFSIZ
#error "Don't #include stdio.h directly if you later #include navit_lfs.h"
#endif
#ifdef F_OK
#error "Don't #include unistd.h directly if you later #include navit_lfs.h"
#endif
#ifdef O_RDWR
#error "Don't #include unistd.h directly if you later #include navit_lfs.h"
#endif

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#ifdef __MSVCRT__
#define __USE_MINGW_FSEEK
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __MSVCRT__

#undef lseek
#define lseek(fd,offset,whence) _lseeki64(fd,offset,whence)

#undef ftello
#define ftello(f) ftello64(f)

#undef fseeko
#define fseeko(f,offset,whence) fseeko64(f,offset,whence)

#undef off_t
#define off_t long long

#endif

#ifdef HAVE_API_ANDROID
#undef lseek
#define lseek lseek64
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define __NAVIT_LFS_H__
#endif

