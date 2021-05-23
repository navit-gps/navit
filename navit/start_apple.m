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

#include "start_real.h"
#import <Foundation/Foundation.h>
#include <glib.h>
#include <stdlib.h>
#include "debug.h"

int main(int argc, char **argv) {
    int ret;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSString *appFolderPath = [[NSBundle mainBundle] resourcePath];
    /*NSString *locale = [[NSLocale currentLocale] localeIdentifier];*/
    NSString * locale = [[NSLocale preferredLanguages] objectAtIndex:0];
    char *lang=g_strdup_printf("%s.UTF-8",[locale UTF8String]);
    dbg(0,"lang %s",lang);
    setenv("LANG",lang,0);
    setlocale(LC_ALL, NULL);

    const char *s=[appFolderPath UTF8String];
    char *user=g_strdup_printf("%s/../Documents",s);
    chdir(s);
    argv[0]=g_strdup_printf("%s/bin/navit",s);
    char *home = getenv("HOME");
    home = g_strdup_printf("%s/.navit",home);
    setenv("NAVIT_USER_DATADIR", home,0);

    char *share=g_strdup_printf("%s/share",s);
    setenv("NAVIT_SHAREDIR", share,0);

    setenv("NAVIT_LIBDIR", s, 0);

    dbg(0,"calling main_real data: %s  share: %s", home, share);
    ret=main_real(argc, argv);
    g_free(argv[0]);
    g_free(user);
    [pool release];
    return ret;
}
