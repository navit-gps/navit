/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __ENDIANESS_HANDLER__

  /* The following is based on xorg/xserver/GL/glx/glxbyteorder.h 
   * which is (c) IBM Corp. 2006,2007 and originally licensed under the following
   * BSD-license. All modifications in navit are licensed under the GNU GPL as 
   * described in file "COPYRIGHT".
   * --------------------------
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   * 
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   * 
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, THE AUTHORS, AND/OR THEIR SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */

#if HAVE_BYTESWAP_H
  /* machine dependent versions of byte swapping functions.  GNU extension.*/
  #include <byteswap.h>
#elif defined(USE_SYS_ENDIAN_H)
  #include <sys/endian.h>
#elif defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  #define __bswap_16 OSSwapInt16
  #define __bswap_32 OSSwapInt32
  #define __bswap_64 OSSwapInt64
#elif if defined(_WIN32) || defined(__CEGCC__)
  #define __BIG_ENDIAN 4321
  #define __LITTLE_ENDIAN 1234
  #define __BYTE_ORDER __LITTLE_ENDIAN
#else
  #define __bswap_16(value)  \
      ((((value) & 0xff) << 8) | ((value) >> 8))
  #define __bswap_32(value) \
      (((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
      (uint32_t)bswap_16((uint16_t)((value) >> 16)))
  #define __bswap_64(value) \
      (((uint64_t)bswap_32((uint32_t)((value) & 0xffffffff)) \
         << 32) | \
      (uint64_t)bswap_32((uint32_t)((value) >> 32)))
#endif
#endif

#if  __BYTE_ORDER == __BIG_ENDIAN 
  #define le16_to_cpu(x)	__bswap_16 (x)
  #define le32_to_cpu(x)	__bswap_32 (x)
  #define le64_to_cpu(x)	__bswap_64 (x)
  #define cpu_to_le16(x)	__bswap_16 (x)
  #define cpu_to_le32(x)	__bswap_32 (x)
  #define cpu_to_le64(x)	__bswap_64 (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN 
  #define le16_to_cpu(x)	(x)
  #define le32_to_cpu(x)	(x)
  #define cpu_to_le16(x)	(x)
  #define cpu_to_le16(x)	(x)
#else
  #error "Unknown endianess"
#endif

#define __ENDIANESS_HANDLER__
#endif

