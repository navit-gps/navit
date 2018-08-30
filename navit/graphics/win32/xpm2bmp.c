#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wingdi.h>
#include <glib/glib.h>
#include "xpm2bmp.h"

/* #define _DBG */

// function prototypes
static int CreateBitmapFromXpm( const char* filename, PXPM2BMP pXpm2bmp );

// typedefs
static XPMCOLORENTRY theRGBRecords[] = {
    {"ALICEBLUE", 240, 248, 255},
    {"ANTIQUEWHITE", 250, 235, 215},
    {"AQUAMARINE", 50, 191, 193},
    {"AZURE", 240, 255, 255},
    {"BEIGE", 245, 245, 220},
    {"BISQUE", 255, 228, 196},
    {"BLACK", 0, 0, 0},
    {"BLANCHEDALMOND", 255, 235, 205},
    {"BLUE", 0, 0, 255},
    {"BLUEVIOLET", 138, 43, 226},
    {"BROWN", 165, 42, 42},
    {"BURLYWOOD", 222, 184, 135},
    {"CADETBLUE", 95, 146, 158},
    {"CHARTREUSE", 127, 255, 0},
    {"CHOCOLATE", 210, 105, 30},
    {"CORAL", 255, 114, 86},
    {"CORNFLOWERBLUE", 34, 34, 152},
    {"CORNSILK", 255, 248, 220},
    {"CYAN", 0, 255, 255},
    {"DARKGOLDENROD", 184, 134, 11},
    {"DARKGREEN", 0, 86, 45},
    {"DARKKHAKI", 189, 183, 107},
    {"DARKOLIVEGREEN", 85, 86, 47},
    {"DARKORANGE", 255, 140, 0},
    {"DARKORCHID", 139, 32, 139},
    {"DARKSALMON", 233, 150, 122},
    {"DARKSEAGREEN", 143, 188, 143},
    {"DARKSLATEBLUE", 56, 75, 102},
    {"DARKSLATEGRAY", 47, 79, 79},
    {"DARKTURQUOISE", 0, 166, 166},
    {"DARKVIOLET", 148, 0, 211},
    {"DEEPPINK", 255, 20, 147},
    {"DEEPSKYBLUE", 0, 191, 255},
    {"DIMGRAY", 84, 84, 84},
    {"DODGERBLUE", 30, 144, 255},
    {"FIREBRICK", 142, 35, 35},
    {"FLORALWHITE", 255, 250, 240},
    {"FORESTGREEN", 80, 159, 105},
    {"GAINSBORO", 220, 220, 220},
    {"GHOSTWHITE", 248, 248, 255},
    {"GOLD", 218, 170, 0},
    {"GOLDENROD", 239, 223, 132},
    {"GRAY", 126, 126, 126},
    {"GRAY0", 0, 0, 0},
    {"GRAY1", 3, 3, 3},
    {"GRAY10", 26, 26, 26},
    {"GRAY100", 255, 255, 255},
    {"GRAY11", 28, 28, 28},
    {"GRAY12", 31, 31, 31},
    {"GRAY13", 33, 33, 33},
    {"GRAY14", 36, 36, 36},
    {"GRAY15", 38, 38, 38},
    {"GRAY16", 41, 41, 41},
    {"GRAY17", 43, 43, 43},
    {"GRAY18", 46, 46, 46},
    {"GRAY19", 48, 48, 48},
    {"GRAY2", 5, 5, 5},
    {"GRAY20", 51, 51, 51},
    {"GRAY21", 54, 54, 54},
    {"GRAY22", 56, 56, 56},
    {"GRAY23", 59, 59, 59},
    {"GRAY24", 61, 61, 61},
    {"GRAY25", 64, 64, 64},
    {"GRAY26", 66, 66, 66},
    {"GRAY27", 69, 69, 69},
    {"GRAY28", 71, 71, 71},
    {"GRAY29", 74, 74, 74},
    {"GRAY3", 8, 8, 8},
    {"GRAY30", 77, 77, 77},
    {"GRAY31", 79, 79, 79},
    {"GRAY32", 82, 82, 82},
    {"GRAY33", 84, 84, 84},
    {"GRAY34", 87, 87, 87},
    {"GRAY35", 89, 89, 89},
    {"GRAY36", 92, 92, 92},
    {"GRAY37", 94, 94, 94},
    {"GRAY38", 97, 97, 97},
    {"GRAY39", 99, 99, 99},
    {"GRAY4", 10, 10, 10},
    {"GRAY40", 102, 102, 102},
    {"GRAY41", 105, 105, 105},
    {"GRAY42", 107, 107, 107},
    {"GRAY43", 110, 110, 110},
    {"GRAY44", 112, 112, 112},
    {"GRAY45", 115, 115, 115},
    {"GRAY46", 117, 117, 117},
    {"GRAY47", 120, 120, 120},
    {"GRAY48", 122, 122, 122},
    {"GRAY49", 125, 125, 125},
    {"GRAY5", 13, 13, 13},
    {"GRAY50", 127, 127, 127},
    {"GRAY51", 130, 130, 130},
    {"GRAY52", 133, 133, 133},
    {"GRAY53", 135, 135, 135},
    {"GRAY54", 138, 138, 138},
    {"GRAY55", 140, 140, 140},
    {"GRAY56", 143, 143, 143},
    {"GRAY57", 145, 145, 145},
    {"GRAY58", 148, 148, 148},
    {"GRAY59", 150, 150, 150},
    {"GRAY6", 15, 15, 15},
    {"GRAY60", 153, 153, 153},
    {"GRAY61", 156, 156, 156},
    {"GRAY62", 158, 158, 158},
    {"GRAY63", 161, 161, 161},
    {"GRAY64", 163, 163, 163},
    {"GRAY65", 166, 166, 166},
    {"GRAY66", 168, 168, 168},
    {"GRAY67", 171, 171, 171},
    {"GRAY68", 173, 173, 173},
    {"GRAY69", 176, 176, 176},
    {"GRAY7", 18, 18, 18},
    {"GRAY70", 179, 179, 179},
    {"GRAY71", 181, 181, 181},
    {"GRAY72", 184, 184, 184},
    {"GRAY73", 186, 186, 186},
    {"GRAY74", 189, 189, 189},
    {"GRAY75", 191, 191, 191},
    {"GRAY76", 194, 194, 194},
    {"GRAY77", 196, 196, 196},
    {"GRAY78", 199, 199, 199},
    {"GRAY79", 201, 201, 201},
    {"GRAY8", 20, 20, 20},
    {"GRAY80", 204, 204, 204},
    {"GRAY81", 207, 207, 207},
    {"GRAY82", 209, 209, 209},
    {"GRAY83", 212, 212, 212},
    {"GRAY84", 214, 214, 214},
    {"GRAY85", 217, 217, 217},
    {"GRAY86", 219, 219, 219},
    {"GRAY87", 222, 222, 222},
    {"GRAY88", 224, 224, 224},
    {"GRAY89", 227, 227, 227},
    {"GRAY9", 23, 23, 23},
    {"GRAY90", 229, 229, 229},
    {"GRAY91", 232, 232, 232},
    {"GRAY92", 235, 235, 235},
    {"GRAY93", 237, 237, 237},
    {"GRAY94", 240, 240, 240},
    {"GRAY95", 242, 242, 242},
    {"GRAY96", 245, 245, 245},
    {"GRAY97", 247, 247, 247},
    {"GRAY98", 250, 250, 250},
    {"GRAY99", 252, 252, 252},
    {"GREEN", 0, 255, 0},
    {"GREENYELLOW", 173, 255, 47},
    {"HONEYDEW", 240, 255, 240},
    {"HOTPINK", 255, 105, 180},
    {"INDIANRED", 107, 57, 57},
    {"IVORY", 255, 255, 240},
    {"KHAKI", 179, 179, 126},
    {"LAVENDER", 230, 230, 250},
    {"LAVENDERBLUSH", 255, 240, 245},
    {"LAWNGREEN", 124, 252, 0},
    {"LEMONCHIFFON", 255, 250, 205},
    {"LIGHTBLUE", 176, 226, 255},
    {"LIGHTCORAL", 240, 128, 128},
    {"LIGHTCYAN", 224, 255, 255},
    {"LIGHTGOLDENROD", 238, 221, 130},
    {"LIGHTGOLDENRODYELLOW", 250, 250, 210},
    {"LIGHTGRAY", 168, 168, 168},
    {"LIGHTPINK", 255, 182, 193},
    {"LIGHTSALMON", 255, 160, 122},
    {"LIGHTSEAGREEN", 32, 178, 170},
    {"LIGHTSKYBLUE", 135, 206, 250},
    {"LIGHTSLATEBLUE", 132, 112, 255},
    {"LIGHTSLATEGRAY", 119, 136, 153},
    {"LIGHTSTEELBLUE", 124, 152, 211},
    {"LIGHTYELLOW", 255, 255, 224},
    {"LIMEGREEN", 0, 175, 20},
    {"LINEN", 250, 240, 230},
    {"MAGENTA", 255, 0, 255},
    {"MAROON", 143, 0, 82},
    {"MEDIUMAQUAMARINE", 0, 147, 143},
    {"MEDIUMBLUE", 50, 50, 204},
    {"MEDIUMFORESTGREEN", 50, 129, 75},
    {"MEDIUMGOLDENROD", 209, 193, 102},
    {"MEDIUMORCHID", 189, 82, 189},
    {"MEDIUMPURPLE", 147, 112, 219},
    {"MEDIUMSEAGREEN", 52, 119, 102},
    {"MEDIUMSLATEBLUE", 106, 106, 141},
    {"MEDIUMSPRINGGREEN", 35, 142, 35},
    {"MEDIUMTURQUOISE", 0, 210, 210},
    {"MEDIUMVIOLETRED", 213, 32, 121},
    {"MIDNIGHTBLUE", 47, 47, 100},
    {"MINTCREAM", 245, 255, 250},
    {"MISTYROSE", 255, 228, 225},
    {"MOCCASIN", 255, 228, 181},
    {"NAVAJOWHITE", 255, 222, 173},
    {"NAVY", 35, 35, 117},
    {"NAVYBLUE", 35, 35, 117},
    {"OLDLACE", 253, 245, 230},
    {"OLIVEDRAB", 107, 142, 35},
    {"ORANGE", 255, 135, 0},
    {"ORANGERED", 255, 69, 0},
    {"ORCHID", 239, 132, 239},
    {"PALEGOLDENROD", 238, 232, 170},
    {"PALEGREEN", 115, 222, 120},
    {"PALETURQUOISE", 175, 238, 238},
    {"PALEVIOLETRED", 219, 112, 147},
    {"PAPAYAWHIP", 255, 239, 213},
    {"PEACHPUFF", 255, 218, 185},
    {"PERU", 205, 133, 63},
    {"PINK", 255, 181, 197},
    {"PLUM", 197, 72, 155},
    {"POWDERBLUE", 176, 224, 230},
    {"PURPLE", 160, 32, 240},
    {"RED", 255, 0, 0},
    {"ROSYBROWN", 188, 143, 143},
    {"ROYALBLUE", 65, 105, 225},
    {"SADDLEBROWN", 139, 69, 19},
    {"SALMON", 233, 150, 122},
    {"SANDYBROWN", 244, 164, 96},
    {"SEAGREEN", 82, 149, 132},
    {"SEASHELL", 255, 245, 238},
    {"SIENNA", 150, 82, 45},
    {"SKYBLUE", 114, 159, 255},
    {"SLATEBLUE", 126, 136, 171},
    {"SLATEGRAY", 112, 128, 144},
    {"SNOW", 255, 250, 250},
    {"SPRINGGREEN", 65, 172, 65},
    {"STEELBLUE", 84, 112, 170},
    {"TAN", 222, 184, 135},
    {"THISTLE", 216, 191, 216},
    {"TOMATO", 255, 99, 71},
    {"TRANSPARENT", 0, 0, 1},
    {"TURQUOISE", 25, 204, 223},
    {"VIOLET", 156, 62, 206},
    {"VIOLETRED", 243, 62, 150},
    {"WHEAT", 245, 222, 179},
    {"WHITE", 255, 255, 255},
    {"WHITESMOKE", 245, 245, 245},
    {"YELLOW", 255, 255, 0},
    {"YELLOWGREEN", 50, 216, 56}
};


PXPM2BMP Xpm2bmp_new(void) {
    PXPM2BMP preturn = g_malloc0( sizeof(XPM2BMP) );
    return preturn;
}


int Xpm2bmp_load( PXPM2BMP pXpm2bmp, const char* filename ) {
    return CreateBitmapFromXpm( filename, pXpm2bmp );
}

int Xpm2bmp_paint( PXPM2BMP pXpm2bmp, HDC hdc, int x1,int y1 ) {
    StretchDIBits(hdc,
                  x1, y1, pXpm2bmp->size_x, pXpm2bmp->size_y,
                  0, 0, pXpm2bmp->size_x, pXpm2bmp->size_y,
                  pXpm2bmp->wimage_data_trans,
                  (BITMAPINFO *)pXpm2bmp->bmih_trans,
                  DIB_RGB_COLORS,
                  SRCAND );

    StretchDIBits(hdc,
                  x1, y1, pXpm2bmp->size_x, pXpm2bmp->size_y,
                  0, 0, pXpm2bmp->size_x, pXpm2bmp->size_y,
                  pXpm2bmp->wimage_data,
                  (BITMAPINFO *)pXpm2bmp->bmih,
                  DIB_RGB_COLORS,
                  SRCPAINT );

    return 0;
}

static int parse_line_values( const char* line, PXPM2BMP pXpm2bmp ) {
    int return_value = 0;
    char* parse_line = (char*)line;
    char* tok;
    int value_pos = 0;

    parse_line = strchr( parse_line, '"' );
    parse_line++;

    tok = strtok( parse_line, " \t\n" );

    while ( tok != 0 ) {
        int val = atoi( tok );
        switch ( value_pos ) {
        case 0:
            pXpm2bmp->size_x = val;
            break;
        case 1:
            pXpm2bmp->size_y = val;
            break;
        case 2:
            pXpm2bmp->colors = val;
            break;
        case 3:
            pXpm2bmp->chars_per_pixel = val;
            break;
        case 4:
            pXpm2bmp->hotspot_x = val;
            break;
        case 5:
            pXpm2bmp->hotspot_y = val;
            break;
        }
        tok = strtok( NULL, " \t" );
        value_pos ++;

    }

    return return_value;
}

static int hex2int( char c ) {
    if ((c >= '0') && (c <='9' )) return c - '0';
    if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
    if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
    return -1;
}

static DWORD string2hex16( const char* str ) {
    int i1 = hex2int( str[0] );
    int i2 = hex2int( str[1] );
    if ( ( i1 >= 0 ) && ( i2 >= 0 ) )
        return i1*16+i2;
    return -1;
}

static int parse_color_values( const char* line, PXPM2BMP pXpm2bmp ) {
    int return_value = 0;
    char* cq    = strchr( line, '"' );
    char* cchar = strchr(  cq+pXpm2bmp->chars_per_pixel+1, 'c' );
    char* chash = strchr(  cq+pXpm2bmp->chars_per_pixel+1, '#' );
    char* qe    = strchr(  cq+pXpm2bmp->chars_per_pixel+1, '"' );

    cq++;

    if ( cq ) {
        memcpy( pXpm2bmp->color_entires[ pXpm2bmp-> color_entires_size].color_str, cq, pXpm2bmp->chars_per_pixel + 1 );
        pXpm2bmp->color_entires[ pXpm2bmp-> color_entires_size].color_str[ pXpm2bmp->chars_per_pixel ] = '\0';


        if ( cchar && chash && qe) {
            int len;
            chash++;
            *qe = 0;
            len = strlen( chash );

            pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].r = string2hex16( &chash[0] );
            pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].g = string2hex16( &chash[len / 3] );
            pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].b = string2hex16( &chash[len * 2 / 3] );
#ifdef _DBG
            printf( "adding color %s => %d RGB %x %x %x to index %d\n",
                    line,
                    pXpm2bmp->color_entires_size,
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].r,
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].g,
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].b,
                    pXpm2bmp->color_entires_size );
#endif
        } else {
            int q;
            char *start = cchar + 1;
            char *end = start;

            while ( *start != 0 ) {
                if ( ( *start != '\t' ) &&  ( *start != ' ' ) ) {
                    break;
                }
                start++;
            }

            end = start;
            while ( *end != 0 ) {
                if ( ( *end == '\t' ) ||  ( *end == ' ' ) ||  ( *end == '"' )) {
                    *end = 0;
                }
                end++;
            }

            start = _strupr( start );

            for ( q=0; q < sizeof( theRGBRecords ) / sizeof( theRGBRecords[0] ); q++ ) {

                if ( 0 == strcmp( start, theRGBRecords[q].color_str  ) ) {
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].r = theRGBRecords[q].r;
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].g = theRGBRecords[q].g;
                    pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].b = theRGBRecords[q].b;
                }
            }
            if ( 0 == strcmp( start, "NONE" ) ) {
                pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].r = 255;
                pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].g = 0;
                pXpm2bmp->color_entires[ pXpm2bmp->color_entires_size].b = 255;
            }
        }
    }
    pXpm2bmp->color_entires_size++;

    return return_value;
}

static int vv = 0;

static int parse_pixel_line_values( const char* line, PXPM2BMP pXpm2bmp, unsigned char* pixel_data,
                                    unsigned char* pixel_data_trans ) {
    int return_value = 0;
    int i,j;


    char* cq    = strchr( line, '"' );
    int pix_idx = 0;
    int size_x = pXpm2bmp->size_x;
    int len = strlen( cq );

    cq++;

    if ( len > pXpm2bmp->chars_per_pixel * size_x ) {
        for ( i=0; i< size_x; i++ ) {
            int found = 0;
            char* cmp = &cq[ i * pXpm2bmp->chars_per_pixel];

            for ( j=0; j< pXpm2bmp-> color_entires_size; j++ ) {
                if ( strncmp( cmp, pXpm2bmp->color_entires[ j ].color_str, pXpm2bmp->chars_per_pixel ) == 0 ) {
                    int r = pXpm2bmp->color_entires[ j ].r;
                    int g = pXpm2bmp->color_entires[ j ].g;
                    int b = pXpm2bmp->color_entires[ j ].b;

                    if ( ( r == 255 ) && ( g == 0 ) && ( r == 255 ) ) {
                        r=g=b=0;
                        pixel_data_trans[ pix_idx ] = 255;
                        pixel_data_trans[ pix_idx+1 ] = 255;
                        pixel_data_trans[ pix_idx+2 ] = 255;
                    } else {
                        pixel_data_trans[ pix_idx ] = 0;
                        pixel_data_trans[ pix_idx+1 ] = 0;
                        pixel_data_trans[ pix_idx+2 ] = 0;
                    }

                    // pixel_data[ pix_idx++ ] = pXpm2bmp->color_entires[ j ].r;
                    // pixel_data[ pix_idx++ ] = pXpm2bmp->color_entires[ j ].g;
                    // pixel_data[ pix_idx++ ] = pXpm2bmp->color_entires[ j ].b;
                    pixel_data[ pix_idx++ ] = b;
                    pixel_data[ pix_idx++ ] = g;
                    pixel_data[ pix_idx++ ] = r;
                    found = 1;
                    vv++;
                    break;
                }
            }
            if ( !found ) {
                fprintf( stderr, "XPMLIB: error color not found\n" );
            }

        }
    } else {
        return_value = -1;
        fprintf( stderr, "XPMLIB: invalid line length\n" );
    }
    return return_value;
}


static int CreateBitmapFromXpm( const char* filename, PXPM2BMP pXpm2bmp ) {
    int return_val = 0;
    unsigned char i, row;
    char line[ 1024   ];
    int nbytes ;
    int padding, rowsize = 0;
    FILE* file_xpm = fopen( filename, "r" );

    int phase = 0;
    row = 0;

    if ( file_xpm ) {
        while ( fgets(line, sizeof( line ), file_xpm ) ) {
#ifdef _DBG
            printf( "PARSING: %s\n", line );
#endif
            if ( line[ 0 ] != '"' )
                continue;

            switch ( phase ) {
            case 0:
                parse_line_values( line, pXpm2bmp );
#ifdef _DBG
                printf( "size_x %d\n", pXpm2bmp->size_x );
                printf( "size_y %d\n", pXpm2bmp->size_y );
#endif
                phase = 1;

                pXpm2bmp->color_entires_size = 0;
                nbytes = ( pXpm2bmp->chars_per_pixel + 1 ) * pXpm2bmp->colors;

                pXpm2bmp->color_entires = g_malloc0( sizeof(XPMCOLORENTRY) * (pXpm2bmp->colors + 100) );
                pXpm2bmp->color_entires[0].color_str = g_malloc0( nbytes * pXpm2bmp->colors );
                for ( i = 1; i< pXpm2bmp->colors; i++ ) {
                    pXpm2bmp->color_entires[i].color_str = pXpm2bmp->color_entires[0].color_str + ( pXpm2bmp->chars_per_pixel + 1 ) * i;
                }

                rowsize=pXpm2bmp->size_x * 3;
                padding=4-(rowsize%4);
                if(padding<4)
                    rowsize+=padding;


                if (!(pXpm2bmp->dib = (unsigned char *)g_malloc(sizeof(BITMAPINFOHEADER) + rowsize * pXpm2bmp->size_y ))) {
                    return 4;
                }
                if (!(pXpm2bmp->dib_trans = (unsigned char *)g_malloc0(sizeof(BITMAPINFOHEADER) + rowsize * pXpm2bmp->size_y ))) {
                    return 4;
                }

                memset(pXpm2bmp->dib, 0, sizeof(BITMAPINFOHEADER));
                pXpm2bmp->bmih = (BITMAPINFOHEADER *)pXpm2bmp->dib;
                pXpm2bmp->bmih->biSize = sizeof(BITMAPINFOHEADER);
                pXpm2bmp->bmih->biWidth = pXpm2bmp->size_x;
                pXpm2bmp->bmih->biHeight = -((long)pXpm2bmp->size_y);
                pXpm2bmp->bmih->biPlanes = 1;
                pXpm2bmp->bmih->biBitCount = 24;
                pXpm2bmp->bmih->biCompression = 0;
                pXpm2bmp->wimage_data = pXpm2bmp->dib + sizeof(BITMAPINFOHEADER);


                pXpm2bmp->bmih_trans = (BITMAPINFOHEADER *)pXpm2bmp->dib_trans;
                pXpm2bmp->bmih_trans->biSize = sizeof(BITMAPINFOHEADER);
                pXpm2bmp->bmih_trans->biWidth = pXpm2bmp->size_x;
                pXpm2bmp->bmih_trans->biHeight = -((long)pXpm2bmp->size_y);
                pXpm2bmp->bmih_trans->biPlanes = 1;
                pXpm2bmp->bmih_trans->biBitCount = 24;
                pXpm2bmp->bmih_trans->biCompression = 0;
                pXpm2bmp->wimage_data_trans = pXpm2bmp->dib_trans + sizeof(BITMAPINFOHEADER);
//					memset( pXpm2bmp->wimage_data_trans, 255, 5* 22 * 3 );

                break;
            case 1:
                parse_color_values( line, pXpm2bmp  );
                if ( pXpm2bmp->color_entires_size >= pXpm2bmp->colors ) {
                    phase = 2;
                }

                break;
            case 2:
                parse_pixel_line_values( line, pXpm2bmp,
                                         pXpm2bmp->wimage_data + row * rowsize,
                                         pXpm2bmp->wimage_data_trans + row * rowsize );

                row++;
                if ( row >= pXpm2bmp->size_y ) {
                    phase = 3;
                }
                break;
            }

        }

        fclose( file_xpm );
    }
    return return_val;
}
