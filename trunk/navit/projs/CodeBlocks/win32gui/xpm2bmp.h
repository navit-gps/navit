#ifndef Xpm2BMP_H_INCLUDED
#define Xpm2BMP_H_INCLUDED

#include <windows.h>
#include "wingdi.h"

typedef struct XPMCOLORENTRY_TAG
{
	char* color_str;
	unsigned long r;
	unsigned long g;
	unsigned long b;
} XPMCOLORENTRY, *PXPMCOLORENTRY;

typedef struct XPM2BMP_TAG
{
	unsigned short size_x;
	unsigned short size_y;
	unsigned short colors;
	unsigned short pixels;
	unsigned short chars_per_pixel;
	unsigned short hotspot_x;
	unsigned short hotspot_y;

	int color_entires_size;
	PXPMCOLORENTRY color_entires;

	unsigned char *dib;
	unsigned char *wimage_data;
	BITMAPINFOHEADER *bmih;

	unsigned char *dib_trans;
	unsigned char *wimage_data_trans;
	BITMAPINFOHEADER *bmih_trans;

} XPM2BMP, *PXPM2BMP;


PXPM2BMP Xpm2bmp_new();
int Xpm2bmp_load( PXPM2BMP pXpm2bmp, const char* filename );
int Xpm2bmp_paint( PXPM2BMP pXpm2bmp, HDC hdc, int x1,int y1 );


#endif // Xpm2BMP_H_INCLUDED
