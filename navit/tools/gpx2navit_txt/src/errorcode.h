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

#ifndef ERRORCODE_H
#define ERRORCODE_H

/* os level */
#define ERR_OUTOFMEMORY 	11
#define ERR_CANNOTOPEN 		12
#define ERR_CREATEFILE 		13
#define ERR_READERROR 		14
#define ERR_FREEFAILED 		15
/* option */
#define ERR_NOARGS 		21
#define ERR_WRONGOPTION 	22
#define ERR_OPTIONCONFRICT 	23
/* parser */
#define ERR_ISNOTGPX 		31
#define ERR_PARSEERROR 		32
/* unit */
#define ERR_ELLPSUNIT		41
#define ERR_LENGTHUNIT		42
#define ERR_TIMEUNIT		43

#endif /* ERRORCODE_H */
