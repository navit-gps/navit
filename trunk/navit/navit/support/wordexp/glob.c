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

/*
 * @file glob.c
 */

#include <config.h>

#ifndef HAVE_GLOB
#if defined _WIN32 || defined _WIN32_WCE
#include <windows.h>
#include "glob.h"

/*
 * @brief searches for all the pathnames matching pattern according to the rules
 * which is similar to the rules used by common shells.
 * here: expanding of ´*´ and ´?´ only in filenames
 * @param pattern: no tilde expansion or parameter substitution is done.
 * @param flags: not supported here
 * @param errfunc: not supported here
 * @param pglob: struct with array containing the matched files/directories
 * @return FALSE on error.
 */
int glob(const char *pattern, int flags,
          int (*errfunc)(const char *epath, int eerrno),
          glob_t *pglob)
{
	char           *pathend,
	               *filename;
	int             pathlen;
	HANDLE          hFiles;
#ifndef UNICODE
	WIN32_FIND_DATA xFindData;
	hFiles = FindFirstFile (pattern, &xFindData);
#else
	int              len = strlen(pattern) * sizeof(wchar_t*);
	wchar_t         *pathname = malloc(len);
	WIN32_FIND_DATAW xFindData;
	mbstowcs (pathname, pattern, len);
	hFiles = FindFirstFile (pathname, &xFindData);
#endif

	if(hFiles == INVALID_HANDLE_VALUE)
	{
		return 1;
	}
	/* store the path information */
	if (NULL == (pathend = max (strrchr (pattern, '\\'), strrchr (pattern, '/'))))
			pathend = (char *) pattern;
	pathlen = pathend - pattern + 1;

	/* glob */
	pglob->gl_pathc = 0;    /* number of founded files */
	pglob->gl_offs = 0;     /* not needed */
	pglob->gl_pathv = malloc(sizeof(char*));	/* list of file names */

	do
	{
		pglob->gl_pathc++;
		pglob->gl_pathv = realloc (pglob->gl_pathv, pglob->gl_pathc * sizeof(char*));
#ifndef UNICODE
		filename = xFindData.cFileName;
#else
		len = wcslen(xFindData.cFileName) * sizeof(char*);
		filename = malloc (len);
		wcstombs (filename, xFindData.cFileName, len);
#endif
		pglob->gl_pathv[pglob->gl_pathc - 1] = malloc ((pathlen + strlen (filename) + 1) * sizeof(char*));
		strncpy (pglob->gl_pathv[pglob->gl_pathc - 1], pattern, pathlen);
		// strcpy (pglob->gl_pathv[pglob->gl_pathc - 1] + pathlen - 1, filename);
		// The above line should be uncommented later. Currently, the blow line needs to be in use.
		// If not, navit on WinCE / Win32 cannot "find" the maps and bookmarks folder
		strcpy (pglob->gl_pathv[pglob->gl_pathc - 1] + pathlen, filename);
	} while (FindNextFile (hFiles, &xFindData));

	FindClose(hFiles);
	return 0;
}

void globfree(glob_t *pglob)
{
	int i;

	for (i=0; i < pglob->gl_pathc; i++)
	{
		free (pglob->gl_pathv[i]);
	}
	free (pglob->gl_pathv);
	pglob->gl_pathc = 0;
}

#endif     /* _WIN32 || _WIN32_WCE */
#endif     /* HAVE_GLOB */
