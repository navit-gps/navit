/* raster.c -- line/rect/circle/poly rasterization

   copyright (c) 2008 bryan rittmeyer <bryanr@bryanr.org>
   license: LGPLv2

   based on SDL_gfx (c) A. Schiffler
   and SGE (c)1999-2003 Anders Lindström

   + added filled anti-aliased polygons
   + fixed some bugs and improved performance

   revision history
   2008-07-05 initial
   2008-07-06 aacircle
*/


#include <math.h>

#include "raster.h"

#undef DEBUG
#undef PARANOID


/* raster_rect */

static inline void raster_rect_inline(SDL_Surface *dst, int16_t x1, int16_t y1, int16_t w, int16_t h, uint32_t color)
{
    /* sge */
    SDL_Rect rect;

#if 1
    if((w <= 0) || (h <= 0))
    {
        return;
    }

#if 0
    if(x1 < 0)
    {
        if((x1 + w) < 0)
        {
            return;
        }
        else
        {
            w = w + x1;
            x1 = 0;
        }
    }
    if(y1 < 0)
    {
        if((y1 + h) < 0)
        {
            return;
        }
        else
        {
            h = h + y1;
            y1 = 0;
        }
    }

    if(x1 + w >= dst->w)
    {
        w = dst->w - x1;
    }
    if(y1 + h >= dst->h)
    {
        h = dst->h - y1;
    }
#endif

    rect.x = x1;
    rect.y = y1;
    rect.w = w;
    rect.h = h;

    /* will clip internally */
    SDL_FillRect(dst, &rect, color);

#else

    x2 = x1 + w;
    y2 = y1 + h;

    /*
     * Order coordinates to ensure that
     * x1<=x2 and y1<=y2 
     */
    if (x1 > x2) {
	tmp = x1;
	x1 = x2;
	x2 = tmp;
    }
    if (y1 > y2) {
	tmp = y1;
	y1 = y2;
	y2 = tmp;
    }

    /* 
     * Get clipping boundary and 
     * check visibility 
     */
    left = dst->clip_rect.x;
    if (x2<left) {
     return(0);
    }
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    if (x1>right) {
     return(0);
    }
    top = dst->clip_rect.y;
    if (y2<top) {
     return(0);
    }
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
    if (y1>bottom) {
     return(0);
    }
     
    /* Clip all points */
    if (x1<left) { 
     x1=left; 
    } else if (x1>right) {
     x1=right;
    }
    if (x2<left) { 
     x2=left; 
    } else if (x2>right) {
     x2=right;
    }
    if (y1<top) { 
     y1=top; 
    } else if (y1>bottom) {
     y1=bottom;
    }
    if (y2<top) { 
     y2=top; 
    } else if (y2>bottom) {
     y2=bottom;
    }

#if 0
    /*
     * Test for special cases of straight line or single point 
     */
    if (x1 == x2) {
	if (y1 == y2) {
	    return (pixelColor(dst, x1, y1, color));
	} else { 
	    return (vlineColor(dst, x1, y1, y2, color));
	}
    }
    if (y1 == y2) {
	return (hlineColor(dst, x1, x2, y1, color));
    }
#endif

    /*
     * Calculate width&height 
     */
    w = x2 - x1;
    h = y2 - y1;

	/*
	 * No alpha-blending required 
	 */

#if 0
	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}
#endif

	/*
	 * Lock surface 
	 */
	SDL_LockSurface(dst);

	/*
	 * More variable setup 
	 */
	dx = w;
	dy = h;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y1;
	pixellast = pixel + pixx * dx + pixy * dy;
	dx++;
	
	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    for (; pixel <= pixellast; pixel += pixy) {
		memset(pixel, (Uint8) color, dx);
	    }
	    break;
	case 2:
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    *(Uint16*) pixel = color;
		    pixel += pixx;
		}
	    }
	    break;
	case 3:
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			pixel[0] = (color >> 16) & 0xff;
			pixel[1] = (color >> 8) & 0xff;
			pixel[2] = color & 0xff;
		    } else {
			pixel[0] = color & 0xff;
			pixel[1] = (color >> 8) & 0xff;
			pixel[2] = (color >> 16) & 0xff;
		    }
		    pixel += pixx;
		}
	    }
	    break;
	default:		/* case 4 */
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    *(Uint32 *) pixel = color;
		    pixel += pixx;
		}
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

#endif
}

void raster_rect(SDL_Surface *s, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t col)
{
    raster_rect_inline(s, x, y, w, h, col);
}

#define raster_rect_inline raster_rect


/* raster :: pixel */

#define MODIFIED_ALPHA_PIXEL_ROUTINE


#define clip_xmin(surface) surface->clip_rect.x
#define clip_xmax(surface) surface->clip_rect.x+surface->clip_rect.w-1
#define clip_ymin(surface) surface->clip_rect.y
#define clip_ymax(surface) surface->clip_rect.y+surface->clip_rect.h-1


static void raster_PutPixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	if(x>=clip_xmin(surface) && x<=clip_xmax(surface) && y>=clip_ymin(surface) && y<=clip_ymax(surface)){
		switch (surface->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				*((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */
				*((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

  				/* Gack - slow, but endian correct */
				*(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
  				*(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
  				*(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
				*(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
			}
			break;

			case 4: { /* Probably 32-bpp */
				*((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
			}
			break;
		}
	}
}

/* PutPixel routine with alpha blending, input color in destination format */

/* New, faster routine - default blending pixel */

static void raster_PutPixelAlpha(SDL_Surface * surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha)
{
    /* sdl-gfx */
    Uint32 Rmask = surface->format->Rmask, Gmask =
	surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
    Uint32 R = 0, G = 0, B = 0, A = 0;

    if (x >= clip_xmin(surface) && x <= clip_xmax(surface)
	&& y >= clip_ymin(surface) && y <= clip_ymax(surface)) {

	switch (surface->format->BytesPerPixel) {
	case 1:{		/* Assuming 8-bpp */
		if (alpha == 255) {
		    *((Uint8 *) surface->pixels + y * surface->pitch + x) = color;
		} else {
		    Uint8 *pixel = (Uint8 *) surface->pixels + y * surface->pitch + x;

		    Uint8 dR = surface->format->palette->colors[*pixel].r;
		    Uint8 dG = surface->format->palette->colors[*pixel].g;
		    Uint8 dB = surface->format->palette->colors[*pixel].b;
		    Uint8 sR = surface->format->palette->colors[color].r;
		    Uint8 sG = surface->format->palette->colors[color].g;
		    Uint8 sB = surface->format->palette->colors[color].b;

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);

		    *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
		}
	    }
	    break;

	case 2:{		/* Probably 15-bpp or 16-bpp */
		if (alpha == 255) {
		    *((Uint16 *) surface->pixels + y * surface->pitch / 2 + x) = color;
		} else {
		    Uint16 *pixel = (Uint16 *) surface->pixels + y * surface->pitch / 2 + x;
		    Uint32 dc = *pixel;

		    R = ((dc & Rmask) + (((color & Rmask) - (dc & Rmask)) * alpha >> 8)) & Rmask;
		    G = ((dc & Gmask) + (((color & Gmask) - (dc & Gmask)) * alpha >> 8)) & Gmask;
		    B = ((dc & Bmask) + (((color & Bmask) - (dc & Bmask)) * alpha >> 8)) & Bmask;
		    if (Amask)
			A = ((dc & Amask) + (((color & Amask) - (dc & Amask)) * alpha >> 8)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	    break;

	case 3:{		/* Slow 24-bpp mode, usually not used */
		Uint8 *pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;
		Uint8 rshift8 = surface->format->Rshift / 8;
		Uint8 gshift8 = surface->format->Gshift / 8;
		Uint8 bshift8 = surface->format->Bshift / 8;
		Uint8 ashift8 = surface->format->Ashift / 8;


		if (alpha == 255) {
		    *(pix + rshift8) = color >> surface->format->Rshift;
		    *(pix + gshift8) = color >> surface->format->Gshift;
		    *(pix + bshift8) = color >> surface->format->Bshift;
		    *(pix + ashift8) = color >> surface->format->Ashift;
		} else {
		    Uint8 dR, dG, dB, dA = 0;
		    Uint8 sR, sG, sB, sA = 0;

		    pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;

		    dR = *((pix) + rshift8);
		    dG = *((pix) + gshift8);
		    dB = *((pix) + bshift8);
		    dA = *((pix) + ashift8);

		    sR = (color >> surface->format->Rshift) & 0xff;
		    sG = (color >> surface->format->Gshift) & 0xff;
		    sB = (color >> surface->format->Bshift) & 0xff;
		    sA = (color >> surface->format->Ashift) & 0xff;

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);
		    dA = dA + ((sA - dA) * alpha >> 8);

		    *((pix) + rshift8) = dR;
		    *((pix) + gshift8) = dG;
		    *((pix) + bshift8) = dB;
		    *((pix) + ashift8) = dA;
		}
	    }
	    break;

#ifdef ORIGINAL_ALPHA_PIXEL_ROUTINE

	case 4:{		/* Probably :-) 32-bpp */
		if (alpha == 255) {
		    *((Uint32 *) surface->pixels + y * surface->pitch / 4 + x) = color;
		} else {
		    Uint32 Rshift, Gshift, Bshift, Ashift;
		    Uint32 *pixel = (Uint32 *) surface->pixels + y * surface->pitch / 4 + x;
		    Uint32 dc = *pixel;

		    Rshift = surface->format->Rshift;
		    Gshift = surface->format->Gshift;
		    Bshift = surface->format->Bshift;
		    Ashift = surface->format->Ashift;

		    R = ((dc & Rmask) + (((((color & Rmask) - (dc & Rmask)) >> Rshift) * alpha >> 8) << Rshift)) & Rmask;
		    G = ((dc & Gmask) + (((((color & Gmask) - (dc & Gmask)) >> Gshift) * alpha >> 8) << Gshift)) & Gmask;
		    B = ((dc & Bmask) + (((((color & Bmask) - (dc & Bmask)) >> Bshift) * alpha >> 8) << Bshift)) & Bmask;
		    if (Amask)
			A = ((dc & Amask) + (((((color & Amask) - (dc & Amask)) >> Ashift) * alpha >> 8) << Ashift)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	    break;
#endif

#ifdef MODIFIED_ALPHA_PIXEL_ROUTINE

	case 4:{		/* Probably :-) 32-bpp */
		if (alpha == 255) {
		    *((Uint32 *) surface->pixels + y * surface->pitch / 4 + x) = color;
		} else {
		    Uint32 Rshift, Gshift, Bshift, Ashift;
		    Uint32 *pixel = (Uint32 *) surface->pixels + y * surface->pitch / 4 + x;
		    Uint32 dc = *pixel;
		    Uint32 dR = (color & Rmask), dG = (color & Gmask), dB = (color & Bmask);
		    Uint32 surfaceAlpha, preMultR, preMultG, preMultB;
		    Uint32 aTmp;

		    Rshift = surface->format->Rshift;
		    Gshift = surface->format->Gshift;
		    Bshift = surface->format->Bshift;
		    Ashift = surface->format->Ashift;

                    preMultR = (alpha * (dR>>Rshift));
                    preMultG = (alpha * (dG>>Gshift));
                    preMultB = (alpha * (dB>>Bshift));

                    surfaceAlpha = ((dc & Amask) >> Ashift);
                    aTmp = (255 - alpha);
                    if ((A = 255 - ((aTmp * (255 - surfaceAlpha)) >> 8 ))) {
                      aTmp *= surfaceAlpha;
                      R = (preMultR + ((aTmp * ((dc & Rmask) >> Rshift)) >> 8)) / A << Rshift & Rmask;
                      G = (preMultG + ((aTmp * ((dc & Gmask) >> Gshift)) >> 8)) / A << Gshift & Gmask;
                      B = (preMultB + ((aTmp * ((dc & Bmask) >> Bshift)) >> 8)) / A << Bshift & Bmask;
                    }
		    *pixel = R | G | B | (A << Ashift & Amask);

		}
	    }
	    break;
#endif
	}
    }
}


/* FIXME: eliminate these 2 functions */

static int raster_pixelColorNolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    int result = 0;

#if 0
    /*
     * Setup color 
     */
    alpha = color & 0x000000ff;
    mcolor =
	SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24,
		    (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);
#endif

    /*
     * Draw 
     */
    raster_PutPixel(dst, x, y, color);

    return (result);
}


/* Pixel - using alpha weight on color for AA-drawing - no locking */

static int raster_pixelColorWeightNolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color, Uint32 weight)
{
#if 0
    Uint32 a;

    /*
     * Get alpha 
     */
    a = (color & (Uint32) 0x000000ff);

    /*
     * Modify Alpha by weight 
     */
    a = ((a * weight) >> 8);
#endif

    raster_PutPixelAlpha(dst, x, y, color, weight);

    return 0;
}


/* raster :: line */



static inline void raster_hline(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
#if 1
	SDL_Rect l;

    /* sge */
	if(x1>x2){Sint16 tmp=x1; x1=x2; x2=tmp;}
	
	l.x=x1; l.y=y; l.w=x2-x1+1; l.h=1;
	
	SDL_FillRect(dst, &l, color);

#else
    /* sdl_gfx */
    Sint16 left, right, top, bottom;
    Uint8 *pixel, *pixellast;
    int dx;
    int pixx, pixy;
    Sint16 w;
    Sint16 xtmp;
    int result = -1;
#if 0
    int i;
    union
    {
        double d;
        uint16_t col[4];
    } doub;

    for(i = 0; i < 4; i++)
    {
        doub.col[i] = color;
    }
#endif

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return(0);
    }
    
    /*
     * Swap x1, x2 if required to ensure x1<=x2
     */
    if (x1 > x2) {
	xtmp = x1;
	x1 = x2;
	x2 = xtmp;
    }

    /*
     * Get clipping boundary and
     * check visibility of hline 
     */
    left = dst->clip_rect.x;
    if (x2<left) {
     return(0);
    }
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    if (x1>right) {
     return(0);
    }
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
    if ((y<top) || (y>bottom)) {
     return (0);
    }

    /*
     * Clip x 
     */
    if (x1 < left) {
	x1 = left;
    }
    if (x2 > right) {
	x2 = right;
    }

    /*
     * Calculate width 
     */
    w = x2 - x1;

#if 0
    printf("raster_hline %d %d %d %d\n", x1, x2, y, w);
#endif

	/*
	 * Lock surface 
	 */
    if (SDL_MUSTLOCK(dst)) {
	SDL_LockSurface(dst);
    }

	/*
	 * More variable setup 
	 */
	dx = w;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y;

	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    memset(pixel, color, dx);
	    break;
	case 2:
	    pixellast = pixel + dx + dx;
#if 0
        for (; (pixel+3) <= pixellast; pixel += 4*pixx)
        {
            *(double *)pixel = doub.d; 
        }
#endif
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint16 *) pixel = color;
	    }
	    break;
	case 3:
	    pixellast = pixel + dx + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
	    }
	    break;
	default:		/* case 4 */
	    dx = dx + dx;
	    pixellast = pixel + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint32 *) pixel = color;
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

	/*
	 * Set result code 
	 */
	result = 0;

    return (result);
#endif
}

static void raster_vline(SDL_Surface *dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color)
{
	SDL_Rect l;

	if(y1>y2){Sint16 tmp=y1; y1=y2; y2=tmp;}
	
	l.x=x; l.y=y1; l.w=1; l.h=y2-y1+1;
	
	SDL_FillRect(dst, &l, color);
}



/* raster_line */
#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

static int clipEncode(Sint16 x, Sint16 y, Sint16 left, Sint16 top, Sint16 right, Sint16 bottom)
{
    int code = 0;

    if (x < left) {
	code |= CLIP_LEFT_EDGE;
    } else if (x > right) {
	code |= CLIP_RIGHT_EDGE;
    }
    if (y < top) {
	code |= CLIP_TOP_EDGE;
    } else if (y > bottom) {
	code |= CLIP_BOTTOM_EDGE;
    }
    return code;
}

static int clipLine(SDL_Surface * dst, Sint16 * x1, Sint16 * y1, Sint16 * x2, Sint16 * y2)
{
    Sint16 left, right, top, bottom;
    int code1, code2;
    int draw = 0;
    Sint16 swaptmp;
    float m;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    while (1) {
	code1 = clipEncode(*x1, *y1, left, top, right, bottom);
	code2 = clipEncode(*x2, *y2, left, top, right, bottom);
	if (CLIP_ACCEPT(code1, code2)) {
	    draw = 1;
	    break;
	} else if (CLIP_REJECT(code1, code2))
	    break;
	else {
	    if (CLIP_INSIDE(code1)) {
		swaptmp = *x2;
		*x2 = *x1;
		*x1 = swaptmp;
		swaptmp = *y2;
		*y2 = *y1;
		*y1 = swaptmp;
		swaptmp = code2;
		code2 = code1;
		code1 = swaptmp;
	    }
	    if (*x2 != *x1) {
		m = (*y2 - *y1) / (float) (*x2 - *x1);
	    } else {
		m = 1.0f;
	    }
	    if (code1 & CLIP_LEFT_EDGE) {
		*y1 += (Sint16) ((left - *x1) * m);
		*x1 = left;
	    } else if (code1 & CLIP_RIGHT_EDGE) {
		*y1 += (Sint16) ((right - *x1) * m);
		*x1 = right;
	    } else if (code1 & CLIP_BOTTOM_EDGE) {
		if (*x2 != *x1) {
		    *x1 += (Sint16) ((bottom - *y1) / m);
		}
		*y1 = bottom;
	    } else if (code1 & CLIP_TOP_EDGE) {
		if (*x2 != *x1) {
		    *x1 += (Sint16) ((top - *y1) / m);
		}
		*y1 = top;
	    }
	}
    }

    return draw;
}


void raster_line(SDL_Surface *dst, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
    /* sdl-gfx */
    int pixx, pixy;
    int x, y;
    int dx, dy;
    int sx, sy;
    int swaptmp;
    Uint8 *pixel;

    /*
     * Clip line and test if we have to draw 
     */
    if (!(clipLine(dst, &x1, &y1, &x2, &y2))) {
	return;
    }

    /*
     * Test for special cases of straight lines or single point 
     */
    if (x1 == x2) {
	if (y1 < y2) {
	    raster_vline(dst, x1, y1, y2, color);
        return;
	} else if (y1 > y2) {
	    raster_vline(dst, x1, y2, y1, color);
        return;
	} else {
	    raster_PutPixel(dst, x1, y1, color);
        return;
	}
    }
    if (y1 == y2) {
	if (x1 < x2) {
	    raster_hline(dst, x1, x2, y1, color);
        return;
	} else if (x1 > x2) {
	    raster_hline(dst, x2, x1, y1, color);
        return;
	}
    }


    /*
     * Variable setup 
     */
    dx = x2 - x1;
    dy = y2 - y1;
    sx = (dx >= 0) ? 1 : -1;
    sy = (dy >= 0) ? 1 : -1;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return;
	}
    }

	/*
	 * No alpha blending - use fast pixel routines 
	 */

#if 0
	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}
#endif

	/*
	 * More variable setup 
	 */
	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
	    swaptmp = dx;
	    dx = dy;
	    dy = swaptmp;
	    swaptmp = pixx;
	    pixx = pixy;
	    pixy = swaptmp;
	}

	/*
	 * Draw 
	 */
	x = 0;
	y = 0;
	switch (dst->format->BytesPerPixel) {
	case 1:
	    for (; x < dx; x++, pixel += pixx) {
		*pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	case 2:
	    for (; x < dx; x++, pixel += pixx) {
		*(Uint16 *) pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	case 3:
	    for (; x < dx; x++, pixel += pixx) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	default:		/* case 4 */
	    for (; x < dx; x++, pixel += pixx) {
		*(Uint32 *) pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	}

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return;
}


#define AAlevels 256
#define AAbits 8

static void raster_aalineColorInt(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, int draw_endpoint)
{
    Sint32 xx0, yy0, xx1, yy1;
    int result;
    Uint32 intshift, erracc, erradj;
    Uint32 erracctmp, wgt, wgtcompmask;
    int dx, dy, tmp, xdir, y0p1, x0pxdir;

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return;
    }

    /*
     * Clip line and test if we have to draw 
     */
    if (!(clipLine(dst, &x1, &y1, &x2, &y2))) {
	return;
    }

    /*
     * Keep on working with 32bit numbers 
     */
    xx0 = x1;
    yy0 = y1;
    xx1 = x2;
    yy1 = y2;

    /*
     * Reorder points if required 
     */
    if (yy0 > yy1) {
	tmp = yy0;
	yy0 = yy1;
	yy1 = tmp;
	tmp = xx0;
	xx0 = xx1;
	xx1 = tmp;
    }

    /*
     * Calculate distance 
     */
    dx = xx1 - xx0;
    dy = yy1 - yy0;

    /*
     * Adjust for negative dx and set xdir 
     */
    if (dx >= 0) {
	xdir = 1;
    } else {
	xdir = -1;
	dx = (-dx);
    }

    /*
     * Check for special cases 
     */
    if (dx == 0) {
	/*
	 * Vertical line 
	 */
    raster_vline(dst, x1, y1, y2, color);
	return;
    } else if (dy == 0) {
	/*
	 * Horizontal line 
	 */
    raster_hline(dst, x1, x2, y1, color);
	return;
    } else if (dx == dy) {
	/*
	 * Diagonal line 
	 */
	raster_line(dst, x1, y1, x2, y2, color);
    return;
    }

    /*
     * Line is not horizontal, vertical or diagonal 
     */
    result = 0;

    /*
     * Zero accumulator 
     */
    erracc = 0;

    /*
     * # of bits by which to shift erracc to get intensity level 
     */
    intshift = 32 - AAbits;
    /*
     * Mask used to flip all bits in an intensity weighting 
     */
    wgtcompmask = AAlevels - 1;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return;
	}
    }

    /*
     * Draw the initial pixel in the foreground color 
     */
    raster_pixelColorNolock(dst, x1, y1, color);

    /*
     * x-major or y-major? 
     */
    if (dy > dx) {

	/*
	 * y-major.  Calculate 16-bit fixed point fractional part of a pixel that
	 * X advances every time Y advances 1 pixel, truncating the result so that
	 * we won't overrun the endpoint along the X axis 
	 */
	/*
	 * Not-so-portable version: erradj = ((Uint64)dx << 32) / (Uint64)dy; 
	 */
	erradj = ((dx << 16) / dy) << 16;

	/*
	 * draw all pixels other than the first and last 
	 */
	x0pxdir = xx0 + xdir;
	while (--dy) {
	    erracctmp = erracc;
	    erracc += erradj;
	    if (erracc <= erracctmp) {
		/*
		 * rollover in error accumulator, x coord advances 
		 */
		xx0 = x0pxdir;
		x0pxdir += xdir;
	    }
	    yy0++;		/* y-major so always advance Y */

	    /*
	     * the AAbits most significant bits of erracc give us the intensity
	     * weighting for this pixel, and the complement of the weighting for
	     * the paired pixel. 
	     */
	    wgt = (erracc >> intshift) & 255;
	    raster_pixelColorWeightNolock (dst, xx0, yy0, color, 255 - wgt);
	    raster_pixelColorWeightNolock (dst, x0pxdir, yy0, color, wgt);
	}

    } else {

	/*
	 * x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
	 * that Y advances each time X advances 1 pixel, truncating the result so
	 * that we won't overrun the endpoint along the X axis. 
	 */
	/*
	 * Not-so-portable version: erradj = ((Uint64)dy << 32) / (Uint64)dx; 
	 */
	erradj = ((dy << 16) / dx) << 16;

	/*
	 * draw all pixels other than the first and last 
	 */
	y0p1 = yy0 + 1;
	while (--dx) {

	    erracctmp = erracc;
	    erracc += erradj;
	    if (erracc <= erracctmp) {
		/*
		 * Accumulator turned over, advance y 
		 */
		yy0 = y0p1;
		y0p1++;
	    }
	    xx0 += xdir;	/* x-major so always advance X */
	    /*
	     * the AAbits most significant bits of erracc give us the intensity
	     * weighting for this pixel, and the complement of the weighting for
	     * the paired pixel. 
	     */
	    wgt = (erracc >> intshift) & 255;
	    raster_pixelColorWeightNolock (dst, xx0, yy0, color, 255 - wgt);
	    raster_pixelColorWeightNolock (dst, xx0, y0p1, color, wgt);
	}
    }

    /*
     * Do we have to draw the endpoint 
     */
    if (draw_endpoint) {
	/*
	 * Draw final pixel, always exactly intersected by the line and doesn't
	 * need to be weighted. 
	 */
	raster_pixelColorNolock (dst, x2, y2, color);
    }

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }
}

void raster_aaline(SDL_Surface *s, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t col)
{
    raster_aalineColorInt(s, x1, y1, x2, y2, col, 1);
}



/* raster :: circle */

void raster_circle(SDL_Surface *dst, int16_t x, int16_t y, int16_t r, uint32_t color)
{
    /* sdl-gfx */
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    Sint16 cx = 0;
    Sint16 cy = r;
    Sint16 ocx = (Sint16) 0xffff;
    Sint16 ocy = (Sint16) 0xffff;
    Sint16 df = 1 - r;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * r + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return;
    }

    /*
     * Sanity check radius 
     */
    if (r < 0) {
	return;
    }

    /*
     * Special case for r=0 - draw a point 
     */
    if (r == 0) {
	return (raster_PutPixel(dst, x, y, color));
    }

    /*
     * Get circle and clipping boundary and 
     * test if bounding box of circle is visible 
     */
    x2 = x + r;
    left = dst->clip_rect.x;
    if (x2<left) {
     return;
    } 
    x1 = x - r;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    if (x1>right) {
     return;
    } 
    y2 = y + r;
    top = dst->clip_rect.y;
    if (y2<top) {
     return;
    } 
    y1 = y - r;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
    if (y1>bottom) {
     return;
    } 

    /*
     * Draw 
     */
    result = 0;
    do {
	xpcx = x + cx;
	xmcx = x - cx;
	xpcy = x + cy;
	xmcy = x - cy;
	if (ocy != cy) {
	    if (cy > 0) {
		ypcy = y + cy;
		ymcy = y - cy;
		raster_hline(dst, xmcx, xpcx, ypcy, color);
		raster_hline(dst, xmcx, xpcx, ymcy, color);
//        raster_rect_inline(dst, xmcx, ypcy, 2*cx, 1, color);
//        raster_rect_inline(dst, xmcx, ymcy, 2*cx, 1, color);

	    } else {
          raster_hline(dst, xmcx, xpcx, y, color);
//        raster_rect_inline(dst, xmcx, y, 2*cx, 1, color);
	    }
	    ocy = cy;
	}
	if (ocx != cx) {
	    if (cx != cy) {
		if (cx > 0) {
		    ypcx = y + cx;
		    ymcx = y - cx;
		    raster_hline(dst, xmcy, xpcy, ymcx, color);
		    raster_hline(dst, xmcy, xpcy, ypcx, color);
            //raster_rect_inline(dst, xmcy, ymcx, 2*cy, 1, color);
            //raster_rect_inline(dst, xmcy, ypcx, 2*cy, 1, color);
		} else {
            raster_hline(dst, xmcy, xpcy, y, color); 
            //raster_rect_inline(dst, xmcy, y, 2*cy, 1, color);
		}
	    }
	    ocx = cx;
	}
	/*
	 * Update 
	 */
	if (df < 0) {
	    df += d_e;
	    d_e += 2;
	    d_se += 2;
	} else {
	    df += d_se;
	    d_e += 2;
	    d_se += 4;
	    cy--;
	}
	cx++;
    } while (cx <= cy);
}


/* FIXME: convert to fixed pt */
static void raster_AAFilledEllipse(SDL_Surface *surface, Sint16 xc, Sint16 yc, Sint16 rx, Sint16 ry, Uint32 color)
{
    /* sge */
	/* Sanity check */
	if (rx < 1)
		rx = 1;
	if (ry < 1)
		ry = 1;
	
	int a2 = rx * rx;
	int b2 = ry * ry;

	int ds = 2 * a2;
	int dt = 2 * b2;

	int dxt = (int)(a2 / sqrt(a2 + b2));

	int t = 0;
	int s = -2 * a2 * ry;
	int d = 0;

	Sint16 x = xc;
	Sint16 y = yc - ry;
	
	Sint16 xs, ys, dyt;
	float cp, is, ip, imax = 1.0;
	
	/* Lock surface */
	if ( SDL_MUSTLOCK(surface) )
		if ( SDL_LockSurface(surface) < 0 )
			return;

	/* "End points" */
	raster_PutPixel(surface, x, y, color);
	raster_PutPixel(surface, 2*xc-x, y, color);
	
	raster_PutPixel(surface, x, 2*yc-y, color);
	raster_PutPixel(surface, 2*xc-x, 2*yc-y, color);
	
	/* unlock surface */
	if (SDL_MUSTLOCK(surface) )
		SDL_UnlockSurface(surface);
	
	raster_vline(surface, x, y+1, 2*yc-y-1, color);

	int i;

	for (i = 1; i <= dxt; i++)
	{
		x--;
		d += t - b2;

		if (d >= 0)
			ys = y - 1;
		else if ((d - s - a2) > 0)
		{
			if ((2 * d - s - a2) >= 0)
				ys = y + 1;
			else
			{
				ys = y;
				y++;
				d -= s + a2;
				s += ds;
			}
		}
		else
		{
			y++;
			ys = y + 1;
			d -= s + a2;
			s += ds;
		}

		t -= dt;
		
		/* Calculate alpha */
		cp = (float) abs(d) / abs(s);
		is = cp * imax;
		ip = imax - is;


		/* Lock surface */
		if ( SDL_MUSTLOCK(surface) )
			if ( SDL_LockSurface(surface) < 0 )
				return;

		/* Upper half */
		raster_PutPixelAlpha(surface, x, y, color, (Uint8)(ip*255));
		raster_PutPixelAlpha(surface, 2*xc-x, y, color, (Uint8)(ip*255));
		
		raster_PutPixelAlpha(surface, x, ys, color, (Uint8)(is*255));
		raster_PutPixelAlpha(surface, 2*xc-x, ys, color, (Uint8)(is*255));
		
		
		/* Lower half */
		raster_PutPixelAlpha(surface, x, 2*yc-y, color, (Uint8)(ip*255));
		raster_PutPixelAlpha(surface, 2*xc-x, 2*yc-y, color, (Uint8)(ip*255));
		
		raster_PutPixelAlpha(surface, x, 2*yc-ys, color, (Uint8)(is*255));
		raster_PutPixelAlpha(surface, 2*xc-x, 2*yc-ys, color, (Uint8)(is*255));
		
		/* unlock surface */
		if (SDL_MUSTLOCK(surface) )
			SDL_UnlockSurface(surface);
		
		
		/* Fill */
		raster_vline(surface, x, y+1, 2*yc-y-1, color);
		raster_vline(surface, 2*xc-x, y+1, 2*yc-y-1, color);
		raster_vline(surface, x, ys+1, 2*yc-ys-1, color);
		raster_vline(surface, 2*xc-x, ys+1, 2*yc-ys-1, color);
	}

	dyt = abs(y - yc);

	for (i = 1; i <= dyt; i++)
	{
		y++;
		d -= s + a2;

		if (d <= 0)
			xs = x + 1;
		else if ((d + t - b2) < 0)
		{
			if ((2 * d + t - b2) <= 0)
				xs = x - 1;
			else
			{
				xs = x;
				x--;
				d += t - b2;
				t -= dt;
			}
		}
		else
		{
			x--;
			xs = x - 1;
			d += t - b2;
			t -= dt;
		}

		s += ds;

		/* Calculate alpha */
		cp = (float) abs(d) / abs(t);
		is = cp * imax;
		ip = imax - is;
		

		/* Lock surface */
		if ( SDL_MUSTLOCK(surface) )
			if ( SDL_LockSurface(surface) < 0 )
				return;

		/* Upper half */
		raster_PutPixelAlpha(surface, x, y, color, (Uint8)(ip*255));
		raster_PutPixelAlpha(surface, 2*xc-x, y, color, (Uint8)(ip*255));
		
		raster_PutPixelAlpha(surface, xs, y, color, (Uint8)(is*255));
		raster_PutPixelAlpha(surface, 2*xc-xs, y, color, (Uint8)(is*255));
		
		
		/* Lower half*/
		raster_PutPixelAlpha(surface, x, 2*yc-y, color, (Uint8)(ip*255));
		raster_PutPixelAlpha(surface, 2*xc-x, 2*yc-y, color, (Uint8)(ip*255));
		
		raster_PutPixelAlpha(surface, xs, 2*yc-y, color, (Uint8)(is*255));
		raster_PutPixelAlpha(surface, 2*xc-xs, 2*yc-y, color, (Uint8)(is*255));

		/* unlock surface */
		if (SDL_MUSTLOCK(surface) )
			SDL_UnlockSurface(surface);
		
		/* Fill */
		raster_hline(surface, x+1, 2*xc-x-1, y, color);
		raster_hline(surface, xs+1, 2*xc-xs-1, y, color);
		raster_hline(surface, x+1, 2*xc-x-1, 2*yc-y, color);
		raster_hline(surface, xs+1, 2*xc-xs-1, 2*yc-y, color);
	}
}

void raster_aacircle(SDL_Surface *s, int16_t x, int16_t y, int16_t r, uint32_t col)
{
    raster_AAFilledEllipse(s, x, y, r, r, col);
}

#if 0
void raster_aacircle(SDL_Surface *s, int16_t x, int16_t y, int16_t r, uint32_t col)
{
    /* sdl-gfx */
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    Sint16 cx = 0;
    Sint16 cy = r;
    Sint16 ocx = (Sint16) 0xffff;
    Sint16 ocy = (Sint16) 0xffff;
    Sint16 df = 1 - r;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * r + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return;
    }

    /*
     * Sanity check radius 
     */
    if (r < 0) {
	return;
    }

#if 0
    /*
     * Special case for r=0 - draw a point 
     */
    if (r == 0) {
	return (pixelColor(dst, x, y, color));
    }
#endif

    /*
     * Get circle and clipping boundary and 
     * test if bounding box of circle is visible 
     */
    x2 = x + r;
    left = dst->clip_rect.x;
    if (x2<left) {
     return;
    } 
    x1 = x - r;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    if (x1>right) {
     return;
    } 
    y2 = y + r;
    top = dst->clip_rect.y;
    if (y2<top) {
     return;
    } 
    y1 = y - r;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
    if (y1>bottom) {
     return;
    } 

    /*
     * Draw 
     */
    result = 0;
    do {
	xpcx = x + cx;
	xmcx = x - cx;
	xpcy = x + cy;
	xmcy = x - cy;
	if (ocy != cy) {
	    if (cy > 0) {
		ypcy = y + cy;
		ymcy = y - cy;
		raster_hlineColor(dst, xmcx, xpcx, ypcy, color);
		raster_hlineColor(dst, xmcx, xpcx, ymcy, color);
//        raster_rect_inline(dst, xmcx, ypcy, 2*cx, 1, color);
//        raster_rect_inline(dst, xmcx, ymcy, 2*cx, 1, color);

	    } else {
          raster_hlineColor(dst, xmcx, xpcx, y, color);
//        raster_rect_inline(dst, xmcx, y, 2*cx, 1, color);
	    }
	    ocy = cy;
	}
	if (ocx != cx) {
	    if (cx != cy) {
		if (cx > 0) {
		    ypcx = y + cx;
		    ymcx = y - cx;
		    raster_hlineColor(dst, xmcy, xpcy, ymcx, color);
		    raster_hlineColor(dst, xmcy, xpcy, ypcx, color);
            //raster_rect_inline(dst, xmcy, ymcx, 2*cy, 1, color);
            //raster_rect_inline(dst, xmcy, ypcx, 2*cy, 1, color);
		} else {
            raster_hlineColor(dst, xmcy, xpcy, y, color); 
            //raster_rect_inline(dst, xmcy, y, 2*cy, 1, color);
		}
	    }
	    ocx = cx;
	}
	/*
	 * Update 
	 */
	if (df < 0) {
	    df += d_e;
	    d_e += 2;
	    d_se += 2;
	} else {
	    df += d_se;
	    d_e += 2;
	    d_se += 4;
	    cy--;
	}
	cx++;
    } while (cx <= cy);
#if 0
    /* sge */
	Sint16 cx = 0;
 	Sint16 cy = r;
	int draw=1;
 	Sint16 df = 1 - r;
 	Sint16 d_e = 3;
 	Sint16 d_se = -2 * r + 5;

#ifdef DEBUG
    printf("raster_circle %d %d %d\n", x, y, r);
#endif

    if(r < 0)
    {
        return;
    }

 	do {
		if(draw)
        {
 			raster_rect_inline(s, x-cx, y+cy, 2*cx, 1, col);
			raster_rect_inline(s, x-cx, y-cy, 2*cx, 1, col);
			draw=0;
		}
		if(cx!=cy)
        {
			if(cx)
            {
                raster_rect_inline(s, x-cy, y-cx, 2*cy, 1, col);
                raster_rect_inline(s, x-cy, y+cx, 2*cy, 1, col);
			}
            else
            {
                raster_rect_inline(s, x-cy, y, 2*cy, 1, col);
            }
		}

		if (df < 0)
        {
	 		df += d_e;
	 		d_e += 2;
	 		d_se += 2;
 		}
   		else
        {
	 		df += d_se;
	 		d_e += 2;
	 		d_se += 4;
	 		cy--;
			draw=1;
   		}
  		cx++;
	} while(cx <= cy);
#endif
}
#endif



/* raster :: poly */


/* ---- Filled Polygon */

/* Helper qsort callback for polygon drawing */

static int gfxPrimitivesCompareInt(const void *a, const void *b)
{
    return (*(const int *) a) - (*(const int *) b);
}


/* Global vertex array to use if optional parameters are not given in polygon calls. */
static int *gfxPrimitivesPolyIntsGlobal = NULL;
static int gfxPrimitivesPolyAllocatedGlobal = 0;

/* (Note: The last two parameters are optional; but required for multithreaded operation.) */  

static inline int raster_filledPolygonColorMT(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color, int **polyInts, int *polyAllocated)
{
    /* sdl-gfx */
    int result;
    int i;
    int y, xa, xb;
    int miny, maxy;
    int x1, y1;
    int x2, y2;
    int ind1, ind2;
    int ints;
    int *gfxPrimitivesPolyInts = NULL;
    int gfxPrimitivesPolyAllocated = 0;

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return(0);
    }

    /*
     * Sanity check number of edges
     */
    if (n < 3) {
	return -1;
    }
     
    /*
     * Map polygon cache  
     */
    if ((polyInts==NULL) || (polyAllocated==NULL)) {
       /* Use global cache */
       gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsGlobal;
       gfxPrimitivesPolyAllocated = gfxPrimitivesPolyAllocatedGlobal;
    } else {
       /* Use local cache */
       gfxPrimitivesPolyInts = *polyInts;
       gfxPrimitivesPolyAllocated = *polyAllocated;
    }

    /*
     * Allocate temp array, only grow array 
     */
    if (!gfxPrimitivesPolyAllocated) {
	gfxPrimitivesPolyInts = (int *) malloc(sizeof(int) * n);
	gfxPrimitivesPolyAllocated = n;
    } else {
	if (gfxPrimitivesPolyAllocated < n) {
	    gfxPrimitivesPolyInts = (int *) realloc(gfxPrimitivesPolyInts, sizeof(int) * n);
	    gfxPrimitivesPolyAllocated = n;
	}
    }

    /*
     * Check temp array
     */
    if (gfxPrimitivesPolyInts==NULL) {        
      gfxPrimitivesPolyAllocated = 0;
    }

    /*
     * Update cache variables
     */
    if ((polyInts==NULL) || (polyAllocated==NULL)) { 
     gfxPrimitivesPolyIntsGlobal =  gfxPrimitivesPolyInts;
     gfxPrimitivesPolyAllocatedGlobal = gfxPrimitivesPolyAllocated;
    } else {
     *polyInts = gfxPrimitivesPolyInts;
     *polyAllocated = gfxPrimitivesPolyAllocated;
    }

    /*
     * Check temp array again
     */
    if (gfxPrimitivesPolyInts==NULL) {        
	return(-1);
    }

    /*
     * Determine Y maxima 
     */
    miny = vy[0];
    maxy = vy[0];
    for (i = 1; (i < n); i++) {
	if (vy[i] < miny) {
	    miny = vy[i];
	} else if (vy[i] > maxy) {
	    maxy = vy[i];
	}
    }

    /*
     * Draw, scanning y 
     */
    result = 0;
    for (y = miny; (y <= maxy); y++) {
	ints = 0;
	for (i = 0; (i < n); i++) {
	    if (!i) {
		ind1 = n - 1;
		ind2 = 0;
	    } else {
		ind1 = i - 1;
		ind2 = i;
	    }
	    y1 = vy[ind1];
	    y2 = vy[ind2];
	    if (y1 < y2) {
		x1 = vx[ind1];
		x2 = vx[ind2];
	    } else if (y1 > y2) {
		y2 = vy[ind1];
		y1 = vy[ind2];
		x2 = vx[ind1];
		x1 = vx[ind2];
	    } else {
		continue;
	    }
	    if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
		gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
	    } 	    
	}
	
	qsort(gfxPrimitivesPolyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

	for (i = 0; (i < ints); i += 2) {
	    xa = gfxPrimitivesPolyInts[i] + 1;
	    xa = (xa >> 16) + ((xa & 32768) >> 15);
	    xb = gfxPrimitivesPolyInts[i+1] - 1;
	    xb = (xb >> 16) + ((xb & 32768) >> 15);
	    raster_hline(dst, xa, xb, y, color);
//        raster_rect_inline(dst, xa, y, xb - xa, 1, color);
	}
    }

    return (result);
}

void raster_polygon(SDL_Surface *s, int16_t n, int16_t *vx, int16_t *vy, uint32_t col)
{
    raster_filledPolygonColorMT(s, vx, vy, n, col, NULL, NULL);
}


void raster_aapolygon(SDL_Surface *dst, int16_t n, int16_t *vx, int16_t *vy, uint32_t color)
{
    /* sdl-gfx + sge w/ rphlx changes: basically, draw aaline border,
       then fill.

       the output is not perfect yet but usually looks better than aliasing
    */
    int result;
    int i;
    int y, xa, xb;
    int miny, maxy;
    int x1, y1;
    int x2, y2;
    int ind1, ind2;
    int ints;
    int *gfxPrimitivesPolyInts = NULL;
    int gfxPrimitivesPolyAllocated = 0;
    const Sint16 *px1, *py1, *px2, *py2;
    int **polyInts;
    int *polyAllocated;

    polyInts = NULL;
    polyAllocated = NULL;

    /*
     * Check visibility of clipping rectangle
     */
    if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
     return;
    }

    /*
     * Sanity check number of edges
     */
    if (n < 3) {
	return;
    }


    /*
     * Pointer setup 
     */
    px1 = px2 = vx;
    py1 = py2 = vy;
    px2++;
    py2++;

    /*
     * Draw 
     */
    result = 0;
    for (i = 1; i < n; i++) {
	raster_aalineColorInt(dst, *px1, *py1, *px2, *py2, color, 0);
	px1 = px2;
	py1 = py2;
	px2++;
	py2++;
    }
    raster_aalineColorInt(dst, *px1, *py1, *vx, *vy, color, 0);

    /*
     * Map polygon cache  
     */
    if ((polyInts==NULL) || (polyAllocated==NULL)) {
       /* Use global cache */
       gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsGlobal;
       gfxPrimitivesPolyAllocated = gfxPrimitivesPolyAllocatedGlobal;
    } else {
       /* Use local cache */
       gfxPrimitivesPolyInts = *polyInts;
       gfxPrimitivesPolyAllocated = *polyAllocated;
    }

    /*
     * Allocate temp array, only grow array 
     */
    if (!gfxPrimitivesPolyAllocated) {
	gfxPrimitivesPolyInts = (int *) malloc(sizeof(int) * n);
	gfxPrimitivesPolyAllocated = n;
    } else {
	if (gfxPrimitivesPolyAllocated < n) {
	    gfxPrimitivesPolyInts = (int *) realloc(gfxPrimitivesPolyInts, sizeof(int) * n);
	    gfxPrimitivesPolyAllocated = n;
	}
    }

    /*
     * Check temp array
     */
    if (gfxPrimitivesPolyInts==NULL) {        
      gfxPrimitivesPolyAllocated = 0;
    }

    /*
     * Update cache variables
     */
    if ((polyInts==NULL) || (polyAllocated==NULL)) { 
     gfxPrimitivesPolyIntsGlobal =  gfxPrimitivesPolyInts;
     gfxPrimitivesPolyAllocatedGlobal = gfxPrimitivesPolyAllocated;
    } else {
     *polyInts = gfxPrimitivesPolyInts;
     *polyAllocated = gfxPrimitivesPolyAllocated;
    }

    /*
     * Check temp array again
     */
    if (gfxPrimitivesPolyInts==NULL) {        
	return;
    }

    /*
     * Determine Y maxima 
     */
    miny = vy[0];
    maxy = vy[0];
    for (i = 1; (i < n); i++) {
	if (vy[i] < miny) {
	    miny = vy[i];
	} else if (vy[i] > maxy) {
	    maxy = vy[i];
	}
    }

    /*
     * Draw, scanning y 
     */
    result = 0;
    for (y = miny; (y <= maxy); y++) {
	ints = 0;
	for (i = 0; (i < n); i++) {
	    if (!i) {
		ind1 = n - 1;
		ind2 = 0;
	    } else {
		ind1 = i - 1;
		ind2 = i;
	    }
	    y1 = vy[ind1];
	    y2 = vy[ind2];
	    if (y1 < y2) {
		x1 = vx[ind1];
		x2 = vx[ind2];
	    } else if (y1 > y2) {
		y2 = vy[ind1];
		y1 = vy[ind2];
		x2 = vx[ind1];
		x1 = vx[ind2];
	    } else {
		continue;
	    }
	    if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
		gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
	    } 	    
	}
	
	qsort(gfxPrimitivesPolyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

//    o = p = -1;
	for (i = 0; (i < ints); i +=2) {
#if 0
	    xa = gfxPrimitivesPolyInts[i] + 1;
	    xa = (xa >> 16) + ((xa & 32768) >> 15);
	    xb = gfxPrimitivesPolyInts[i+1] - 1;
	    xb = (xb >> 16) + ((xb & 32768) >> 15);
#else
        xa = (gfxPrimitivesPolyInts[i] >> 16);
        xb = (gfxPrimitivesPolyInts[i+1] >> 16);
#endif

#if 0
        if(o < 0)
        {
            o = xa+1;
        }
        else if(p < 0)
        {
            p = xa;
        }

        if( (o >= 0) && (p >= 0))
        {
            if(p-o < 0)
            {
                o = p = -1;
                continue;
            }

            raster_hlineColor(dst, o, p, y, color);

            o = p = -1;
        }
#else

        raster_hline(dst, xa+1, xb, y, color);
#endif

//        raster_rect_inline(dst, xa, y, xb - xa, 1, color);
	}
    }
}

