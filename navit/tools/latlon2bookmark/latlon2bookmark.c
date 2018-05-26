/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

/**
 * this program is free software, please use it as you like.
 * i am not a programmer, so please have this code reviewed
 * to make sure it has nog bugs or ill side efects.
 * use at your own risk
 * compile it with: gcc -l m -o latlon2bookmark latlon2bookmark.c
 * Credits for this work goes to edje in #navit
**/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main( int argc, char **argv ) {
    char        description[256];
    char        lngsign[10];
    char        latsign[10];
    float       lat;
    float       lng;
    long        intlat;
    long        intlng;
    int         i;
    int         n;

    if ( argc < 4 ) {
        printf("\n");
        printf("This program converts a lat/lon coordinates pair\n");
        printf("into a bookmark you can use with navit.\n");
        printf("This program expects 3 arguments in the following order:\n");
        printf("Lat, Lon and a description\n");
        printf("and will print a bookmark.\n");
        printf("for example: latlon2bookmark 51.980344 4.358005 this is my house \n");
        printf("\n");
        return 1;
    }

    lat=atof(argv[1]);
    lng=atof(argv[2]);

    /* concatenate all parts of the description string */
    strcpy(description, argv[3]);
    n=0;
    for (i=4; i < argc; i++) {
        /* add spaces between the parts of the description */
        if ( i < argc ) {
            strcat(description, " ");
        }
        strcat(description, argv[i]);
        n=n+1;
    }

    if ( lat < -90 ) {
        printf("\n");
        printf("The first argument must be the lattitude\n");
        printf("and can't be smaller then -90 (southpole)\n");
        printf("\n");
        return 2;
    }

    if ( lat > 90 ) {
        printf("\n");
        printf("The first argument must be the lattitude\n");
        printf("and can't be bigger then 90 (northpole)\n");
        printf("\n");
        return 3;
    }

    if ( lng < -180 ) {
        printf("\n");
        printf("The second argument must be the longitude\n");
        printf("and can't be smaller then -180 (oposite the 0 meridian)\n");
        printf("\n");
        return 4;
    }

    if ( lng > 180 ) {
        printf("\n");
        printf("The first argument must be the longitude\n");
        printf("and can't be bigger then 180 (oposite the 0 meridian)\n");
        printf("\n");
        return 5;
    }

    /* convert the longitude to an integer */
    intlng=lng*6371000.0*M_PI/180;

    /* aparently if inlng < 0 , inlng needs to be inverted and a - sign used in the output */
    strcpy(lngsign, "0x");
    if ( intlng < 0) {
        intlng=(intlng ^ 0xffffffff);
        strcpy(lngsign, "-0x");
    }

    /* and the same for the latitude */
    intlat=log(tan(M_PI_4+lat*M_PI/360))*6371000.0;

    /* aparently if inlat < 0 , inlat needs to be inverted and a - sign used in the output */
    strcpy(latsign, "0x");
    if ( intlat < 0) {
        intlat=(intlat ^ 0xffffffff);
        strcpy(latsign, "-0x");
    }

    /* print the bookmark */
    fprintf(stderr,"\n");
    fprintf(stdout,"mg:%s%x %s%x type=bookmark label=\"%s\"\n",lngsign,intlng,latsign,intlat,description);
    fprintf(stderr,"\n");

    return 0;
}
