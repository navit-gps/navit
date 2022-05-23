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

/* Error message processing header file */
#ifndef EMESS_H
#define EMESS_H

#ifndef lint
/*static char EMESS_H_ID[] = "@(#)emess.h	4.1	93/03/08	GIE	REL";*/
#endif

struct EMESS {
	char	*File_name,	/* input file name */
			*Prog_name;	/* name of program */
	int		File_line;	/* approximate line read
							where error occured */
};

#ifdef EMESS_ROUTINE	/* use type */
/* for emess procedure */
struct EMESS emess_dat = { (char *)0, (char *)0, 0 };

#ifdef sun /* Archaic SunOs 4.1.1, etc. */
extern char *sys_errlist[];
#define strerror(n) (sys_errlist[n])
#endif

#else	/* for for calling procedures */

extern struct EMESS emess_dat;
void emess(int, char *, ...);

#endif /* use type */

#endif /* end EMESS_H */
