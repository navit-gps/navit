#include <windows.h>
#include <wingdi.h>
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "callback.h"
#include "plugin.h"
#include "item.h"
#include "win32_gui.h"
#include "xpm2bmp.h"
#ifdef _WIN32_WCE
#include "support/win32/ConvertUTF.h"
#endif

static HWND g_hwnd;

#ifndef GET_WHEEL_DELTA_WPARAM
	#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif


static GHashTable *image_cache_hash = NULL;


HFONT EzCreateFont (HDC hdc, TCHAR * szFaceName, int iDeciPtHeight,
                    int iDeciPtWidth, int iAttributes, BOOL fLogRes) ;

#define EZ_ATTR_BOLD          1
#define EZ_ATTR_ITALIC        2
#define EZ_ATTR_UNDERLINE     4
#define EZ_ATTR_STRIKEOUT     8

HFONT EzCreateFont (HDC hdc, TCHAR * szFaceName, int iDeciPtHeight,
                    int iDeciPtWidth, int iAttributes, BOOL fLogRes)
{
     FLOAT      cxDpi, cyDpi ;
     HFONT      hFont ;
     LOGFONT    lf ;
     POINT      pt ;
     TEXTMETRIC tm ;

     SaveDC (hdc) ;

#ifndef HAVE_API_WIN32_CE
     SetGraphicsMode (hdc, GM_ADVANCED) ;
     ModifyWorldTransform (hdc, NULL, MWT_IDENTITY) ;
#endif
     SetViewportOrgEx (hdc, 0, 0, NULL) ;
#ifndef HAVE_API_WIN32_CE
     SetWindowOrgEx   (hdc, 0, 0, NULL) ;
#endif

     if (fLogRes)
     {
          cxDpi = (FLOAT) GetDeviceCaps (hdc, LOGPIXELSX) ;
          cyDpi = (FLOAT) GetDeviceCaps (hdc, LOGPIXELSY) ;
     }
     else
     {
          cxDpi = (FLOAT) (25.4 * GetDeviceCaps (hdc, HORZRES) /
                                        GetDeviceCaps (hdc, HORZSIZE)) ;

          cyDpi = (FLOAT) (25.4 * GetDeviceCaps (hdc, VERTRES) /
                                        GetDeviceCaps (hdc, VERTSIZE)) ;
     }

     pt.x = (int) (iDeciPtWidth  * cxDpi / 72) ;
     pt.y = (int) (iDeciPtHeight * cyDpi / 72) ;

#ifndef HAVE_API_WIN32_CE
     DPtoLP (hdc, &pt, 1) ;
#endif
     lf.lfHeight         = - (int) (fabs (pt.y) / 10.0 + 0.5) ;
     lf.lfWidth          = 0 ;
     lf.lfEscapement     = 0 ;
     lf.lfOrientation    = 0 ;
     lf.lfWeight         = iAttributes & EZ_ATTR_BOLD      ? 700 : 0 ;
     lf.lfItalic         = iAttributes & EZ_ATTR_ITALIC    ?   1 : 0 ;
     lf.lfUnderline      = iAttributes & EZ_ATTR_UNDERLINE ?   1 : 0 ;
     lf.lfStrikeOut      = iAttributes & EZ_ATTR_STRIKEOUT ?   1 : 0 ;
     lf.lfCharSet        = DEFAULT_CHARSET ;
     lf.lfOutPrecision   = 0 ;
     lf.lfClipPrecision  = 0 ;
     lf.lfQuality        = 0 ;
     lf.lfPitchAndFamily = 0 ;

     lstrcpy (lf.lfFaceName, szFaceName) ;

     hFont = CreateFontIndirect (&lf) ;

     if (iDeciPtWidth != 0)
     {
          hFont = (HFONT) SelectObject (hdc, hFont) ;

          GetTextMetrics (hdc, &tm) ;

          DeleteObject (SelectObject (hdc, hFont)) ;

          lf.lfWidth = (int) (tm.tmAveCharWidth *
                                        fabs (pt.x) / fabs (pt.y) + 0.5) ;

          hFont = CreateFontIndirect (&lf) ;
     }

     RestoreDC (hdc, -1) ;
     return hFont ;
}

struct graphics_image_priv {
	PXPM2BMP pxpm;
};


static void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
    wprintf((LPTSTR)lpDisplayBuf, TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);

    dbg(0, "%s\n", lpDisplayBuf );
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}



struct graphics_gc_priv {
	HWND hwnd;
	int  line_width;
	COLORREF fg_color;
	COLORREF bg_color;
	struct graphics_priv *gr;
};


//struct graphics_priv *g_gra;

static HDC hMemDC;
static HBITMAP hBitmap;
static HBITMAP hOldBitmap;

// Fills the region 'rgn' in graded colours
static void MakeMemoryDC(HANDLE hWnd, HDC hdc )
{
	if ( hMemDC )
	{
		if ( hOldBitmap )
		{
			SelectObject( hMemDC, hOldBitmap );
			DeleteObject( hBitmap );
			hBitmap = NULL;
			hOldBitmap = NULL;
		}
	}

	// Creates memory DC
	hMemDC = CreateCompatibleDC(hdc);
	if ( hMemDC )
	{
		RECT rectRgn;
		GetClientRect( hWnd, &rectRgn );

		int Width = rectRgn.right - rectRgn.left;
		int Height = rectRgn.bottom - rectRgn.top;
		dbg(0, "resize memDC to: %d %d \n", Width, Height );

		hBitmap = CreateCompatibleBitmap(hdc, Width, Height );

		if ( hBitmap )
		{
			hOldBitmap = (HBITMAP) SelectObject( hMemDC, hBitmap);
		}
	}
}

static void HandleButtonClick( struct graphics_priv *gra_priv, int updown, int button, long lParam )
{
	POINTS p = MAKEPOINTS(lParam);
	struct point pt;
	pt.x = p.x;
	pt.y = p.y;
	callback_list_call_attr_3(gra_priv->cbl, attr_button, (void *)updown, (void *)button, (void *)&pt);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{

//if ( Message != 15 )
//printf( "CHILD %d %d %d \n", Message, wParam, lParam );

	struct graphics_priv* gra_priv = (struct graphics_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

	switch(Message)
	{
		case WM_CREATE:
		{
			HDC hdc;
			hdc = GetDC( hwnd );
			MakeMemoryDC(hwnd, hdc );
			ReleaseDC( hwnd, hdc );
		}
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case WM_USER + 1:
				break;
			}
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_USER+1:
			if ( gra_priv )
			{
				RECT rc ;
				HDC hdc;

				GetClientRect( hwnd, &rc );
				gra_priv->width = rc.right;
				gra_priv->height = rc.bottom;

				hdc = GetDC( hwnd );
				MakeMemoryDC(hwnd, hdc );
				ReleaseDC( hwnd, hdc );
				callback_list_call_attr_2(gra_priv->cbl, attr_resize, (void *)gra_priv->width, (void *)gra_priv->height);
			}
		break;
		case WM_USER+2:
			{
				struct callback_list *cbl = wParam;
#ifdef HAVE_API_WIN32_CE
				/* FIXME: Reset the idle timer  need a better place */
				SystemIdleTimerReset();
#endif
				callback_list_call_0(cbl);
			}
			break;

		case WM_SIZE:
		/*
			if ( gra_priv )
			{
				//graphics = GetWindowLong( hwnd, DWL_USER, 0 );


				{
					HDC hdc;
					hdc = GetDC( hwnd );
					MakeMemoryDC(hwnd, hdc );
					ReleaseDC( hwnd, hdc );
				}
				(*gra_priv->resize_callback)(gra_priv->resize_callback_data, gra_priv->width, gra_priv->height);


			}
			*/
			if ( gra_priv )
			{
				gra_priv->width = LOWORD( lParam );
				gra_priv->height  = HIWORD( lParam );
				dbg(0, "resize gfx to: %d %d \n", gra_priv->width, gra_priv->height );
			}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			//exit( 0 );
		break;
		case WM_PAINT:
			if ( gra_priv )
			{
				HDC hdc = GetDC(hwnd );
				if ( hMemDC )
				{
					BitBlt( hdc, 0, 0, gra_priv->width , gra_priv->height, hMemDC, 0, 0, SRCCOPY );
				}
				ReleaseDC( hwnd, hdc );
			}
		break;
		case WM_MOUSEMOVE:
		{
			POINTS p = MAKEPOINTS(lParam);
			struct point pt;
			pt.x = p.x;
			pt.y = p.y;

			//dbg(1, "WM_MOUSEMOVE: %d %d \n", p.x, p.y );
			callback_list_call_attr_1(gra_priv->cbl, attr_motion, (void *)&pt);
		}
		break;

		case WM_LBUTTONDOWN:
			dbg(1, "LBUTTONDOWN\n");
			HandleButtonClick( gra_priv, 1, 1, lParam);
		break;
		case WM_LBUTTONUP:
			dbg(1, "LBUTTONUP\n");
			HandleButtonClick( gra_priv, 0, 1, lParam);
		break;
		case WM_RBUTTONDOWN:
			HandleButtonClick( gra_priv, 1, 3,lParam );
		break;
		case WM_RBUTTONUP:
			HandleButtonClick( gra_priv, 0, 3,lParam );
		break;
		case WM_LBUTTONDBLCLK:
			dbg(1, "LBUTTONDBLCLK\n");
			HandleButtonClick( gra_priv, 1, 6,lParam );
		break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}


static const wchar_t g_szClassName[] = TEXT("NAVGRA");

static HANDLE CreateGraphicsWindows( struct graphics_priv* gr )
{
#ifdef HAVE_API_WIN32_CE
	WNDCLASS wc;
#else
	WNDCLASSEX wc;
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.hIconSm		 = NULL;
#endif
	HWND hwnd;
	RECT rcParent;

	wc.style	 = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc	= WndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 64;
	wc.hInstance	= GetModuleHandle(NULL);
	wc.hIcon	= NULL;
	wc.hCursor	= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;

	GetClientRect( gr->wnd_parent_handle,&rcParent);

#ifdef HAVE_API_WIN32_CE
	if(!RegisterClass(&wc))
#else
	if(!RegisterClassEx(&wc))
#endif
	{
		ErrorExit( TEXT("Window Registration Failed!") );
		return NULL;
	}

	gr->width = rcParent.right - rcParent.left;
	gr->height  = rcParent.bottom - rcParent.top;
#ifdef HAVE_API_WIN32_CE
	callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->width, (void *)gr->height);
#endif

	g_hwnd = hwnd = CreateWindow(g_szClassName,
				TEXT(""),
				WS_VISIBLE,
				0,
				0,
				gr->width,
				gr->height,
				gr->wnd_parent_handle,
				(HMENU)ID_CHILD_GFX,
				GetModuleHandle(NULL),
				NULL);

	if(hwnd == NULL)
	{
		ErrorExit( TEXT("Window Creation Failed!") );
		return NULL;
	}
	gr->wnd_handle = hwnd;

	SetWindowLongPtr( hwnd , DWLP_USER, gr );

	ShowWindow( hwnd, TRUE );
	UpdateWindow( hwnd );


	PostMessage( gr->wnd_parent_handle, WM_USER + 1, 0, 0 );

	return hwnd;
}



static void graphics_destroy(struct graphics_priv *gr)
{
	g_free( gr );
}


static void gc_destroy(struct graphics_gc_priv *gc)
{
	g_free( gc );
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->line_width = w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, int width, int offset, unsigned char dash_list[], int n)
{
//	gdk_gc_set_dashes(gc->gc, 0, (gint8 *)dash_list, n);
//	gdk_gc_set_line_attributes(gc->gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}



static void gc_set_color(struct graphics_gc_priv *gc, struct color *c, int fg)
{

	gc->fg_color = RGB( c->r, c->g, c->b );
}

static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->fg_color = RGB( c->r, c->g, c->b );
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc->bg_color = RGB( c->r, c->g, c->b );
	if ( hMemDC )
		SetBkColor( hMemDC, gc->bg_color );

}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,
	gc_set_foreground,
	gc_set_background
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);
	*meth=gc_methods;
	gc->hwnd = gr->wnd_handle;
	gc->line_width = 1;
	gc->fg_color = RGB( 0,0,0 );
	gc->bg_color = RGB( 255,255,255 );
	return gc;
}


static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
	HPEN holdpen;
	HPEN hpen;

#ifdef UNDER_CE
	/* user styles not supported under CE */
	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
#else
	hpen = ExtCreatePen(PS_GEOMETRIC|PS_ENDCAP_ROUND|PS_JOIN_ROUND
		, gc->line_width, NULL, 0, NULL);
#endif
	holdpen = SelectObject( hMemDC, hpen );

	SetBkColor( hMemDC, gc->bg_color );

	int first = 1;
	for ( i = 0; i< count; i++ )
	{
		if ( first )
		{
			first = 0;
			MoveToEx( hMemDC, p[0].x, p[0].y, NULL );
		}
		else
		{
			LineTo( hMemDC, p[i].x, p[i].y );
		}
	}

	SelectObject( hMemDC, holdpen );
	DeleteObject( hpen );
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{

	//if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
	{
		int i;
		POINT points[ count ];
		for ( i=0;i< count; i++ )
		{
			points[i].x = p[i].x;
			points[i].y = p[i].y;
		}
		HBRUSH holdbrush;
		HBRUSH hbrush;

		SetBkColor( hMemDC, gc->bg_color );

		hbrush = CreateSolidBrush( gc->fg_color );
		holdbrush = SelectObject( hMemDC, hbrush );
		Polygon( hMemDC, points,count );
		SelectObject( hMemDC, holdbrush );
		DeleteObject( hbrush );
	}
}


static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
//	gdk_draw_rectangle(gr->drawable, gc->gc, TRUE, p->x, p->y, w, h);
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	HDC dc = hMemDC;

	HPEN holdpen;
	HPEN hpen;

	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
	holdpen = SelectObject( dc, hpen );

	SetBkColor( hMemDC, gc->bg_color );

	Ellipse( dc, p->x - r, p->y -r, p->x + r, p->y + r );

	SelectObject( dc, holdpen );
	DeleteObject( hpen );

//	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
//		gdk_draw_arc(gr->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
//	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
//		gdk_draw_arc(gr->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
}



static void draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	InvalidateRect( gr->wnd_handle, NULL, FALSE );
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	// printf( "set draw_mode to %d\n", (int)mode );

	if ( mode == draw_mode_begin )
	{
		if ( gr->wnd_handle == NULL )
		{
			CreateGraphicsWindows( gr );
		}
		if ( gr->mode != draw_mode_begin )
		{
			if ( hMemDC )
			{
				RECT rcClient;
				HBRUSH bgBrush = CreateSolidBrush( gr->bg_color  );
				GetClientRect( gr->wnd_handle, &rcClient );
				FillRect( hMemDC, &rcClient, bgBrush );
				DeleteObject( bgBrush );
			}
		}
	}

	// force paint
	if (mode == draw_mode_end && gr->mode == draw_mode_begin)
	{
		InvalidateRect( gr->wnd_handle, NULL, FALSE );
	}

	gr->mode=mode;

}


static void * get_data(struct graphics_priv *this_, char *type)
{
	if ( strcmp( "wnd_parent_handle_ptr", type ) == 0 )
	{
		return &( this_->wnd_parent_handle );
	}
	if ( strcmp( "START_CLIENT", type ) == 0 )
	{
		CreateGraphicsWindows( this_ );
		return NULL;
	}
	return NULL;
}


static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	RECT rcClient;
	HBRUSH bgBrush = CreateSolidBrush( gc->bg_color  );
	GetClientRect( gr->wnd_handle, &rcClient );
	FillRect( hMemDC, &rcClient, bgBrush );
	DeleteObject( bgBrush );
	gr->bg_color = gc->bg_color;
}

struct graphics_font_priv {
	LOGFONT lf;
	HFONT hfont;
	int size;
};

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	RECT rcClient;
	GetClientRect( gr->wnd_handle, &rcClient );

	int prevBkMode = SetBkMode( hMemDC, TRANSPARENT );

	if ( NULL == font->hfont )
	{
#ifdef WIN_USE_SYSFONT
		font->hfont = (HFONT) GetStockObject (SYSTEM_FONT);
		GetObject (font->hfont, sizeof (LOGFONT), &font->lf);
#else
		font->hfont = EzCreateFont (hMemDC, TEXT ("Arial"), font->size/2, 0, 0, TRUE);
		GetObject ( font->hfont, sizeof (LOGFONT), &font->lf) ;
#endif
	}


	double angle = -atan2( dy, dx ) * 180 / 3.14159 ;
	if (angle < 0)
		angle += 360;

	SetTextAlign (hMemDC, TA_BASELINE) ;
	SetViewportOrgEx (hMemDC, p->x, p->y, NULL) ;
	font->lf.lfEscapement = font->lf.lfOrientation = ( angle * 10 ) ;
	DeleteObject (font->hfont) ;

	font->hfont = CreateFontIndirect (&font->lf);
	HFONT hOldFont = SelectObject(hMemDC, font->hfont );

#ifdef HAVE_GLIB
	gunichar2* utf16 = NULL;
	glong utf16_len = 0;

	utf16 = g_utf8_to_utf16( text, -1, NULL, &utf16_len, NULL );
	TextOutW(hMemDC, 0,0, utf16, (size_t)utf16_len );
	g_free( utf16 );
#endif
#ifdef _WIN32_WCE
	{
		wchar_t utf16[1024];
		const UTF8 *utf8 = (UTF8 *)text;
		UTF16 *utf16p = (UTF16 *) utf16;
		SetBkMode (hMemDC, TRANSPARENT);
		if (ConvertUTF8toUTF16(&utf8, utf8+strlen(text),
			&utf16p, utf16p+sizeof(utf16), 
			lenientConversion) == conversionOK) {
			ExtTextOut (hMemDC, 0, 0, 0, NULL,
				utf16, (wchar_t*) utf16p - utf16, NULL);
		}
	}
#endif


	SelectObject(hMemDC, hOldFont);
	DeleteObject (font->hfont) ;

	SetBkMode( hMemDC, prevBkMode );

	SetViewportOrgEx (hMemDC, 0, 0, NULL) ;
}

static void font_destroy(struct graphics_font_priv *font)
{
	if ( font->hfont )
	{
		DeleteObject(font->hfont);
	}
	g_free(font);
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *name, int size, int flags)
{
	struct graphics_font_priv *font=g_new(struct graphics_font_priv, 1);
	*meth = font_methods;

	font->hfont = NULL;
	font->size = size;
    // FontFamily fontFamily( "Liberation Mono");
//font( &fontFamily, size, FontStyleRegular, UnitPoint );
	return font;
}

static void image_cache_hash_add( const char* key, struct graphics_image_priv* val_ptr)
{
	if ( image_cache_hash == NULL ) {
		image_cache_hash = g_hash_table_new(g_str_hash, g_str_equal);
	}

	if ( g_hash_table_lookup(image_cache_hash, key ) == NULL )
	{
		g_hash_table_insert(image_cache_hash, g_strdup( key ),  (gpointer)val_ptr );
	}

}

static struct graphics_image_priv* image_cache_hash_lookup( const char* key )
{
	struct graphics_image_priv* val_ptr = NULL;

	if ( image_cache_hash != NULL )
	{
		val_ptr = g_hash_table_lookup(image_cache_hash, key );
	}
	return val_ptr;
}



static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot)
{
	struct graphics_image_priv* ret;

	if ( NULL == ( ret = image_cache_hash_lookup( name ) ) )
	{
		ret = g_new( struct graphics_image_priv, 1 );
		dbg(2, "loading image '%s'\n", name );
		ret->pxpm = Xpm2bmp_new();
		Xpm2bmp_load( ret->pxpm, name );
		image_cache_hash_add( name, ret );
	}

	return ret;
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	Xpm2bmp_paint( img->pxpm , hMemDC, p->x, p->y );
}

static struct graphics_methods graphics_methods = {
 	graphics_destroy,
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	draw_circle,
	draw_text,
	draw_image,
#ifdef HAVE_IMLIB2
	NULL, // draw_image_warp,
#else
	NULL,
#endif
	draw_restore,
	NULL,	// draw_drag
	font_new,
	gc_new,
	background_gc,
	NULL, // overlay_new,
	image_new,
	get_data,
};

static struct graphics_priv * graphics_win32_drawing_area_new_helper(struct graphics_methods *meth)
{
	struct graphics_priv *this_=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;
	this_->mode = -1;
	return this_;
}

struct graphics_priv* win32_graphics_new( struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct graphics_priv* this_;
	if (!event_request_system("win32","graphics_win32"))
		return NULL;
	this_=graphics_win32_drawing_area_new_helper(meth);
	this_->cbl=cbl;
	return this_;
}

static void
event_win32_main_loop_run(void)
{
	MSG msg;

	dbg(0,"enter\n");
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);       /*  Keyboard input.      */
		DispatchMessage(&msg);
	}

}

static void event_win32_main_loop_quit(void)
{
	dbg(0,"enter\n");
	PostQuitMessage(0);
}

static struct event_watch *
event_win32_add_watch(struct file *f, enum event_watch_cond cond, struct callback *cb)
{
	dbg(0,"enter\n");
	return NULL;
}

static void
event_win32_remove_watch(struct event_watch *ev)
{
	dbg(0,"enter\n");
}

static GList *timers;
struct event_timeout {
	UINT_PTR timer_id;
	int multi;
	struct callback *cb;
	struct event_timeout *next;
};

static void run_timer(UINT_PTR idEvent)
{
	GList *l;
	struct event_timeout *t;
	l = timers;
	while (l) {
		t = l->data;
		if (t->timer_id == idEvent) {
			dbg(1, "run timer:%d\n", idEvent);
			callback_call_0(t->cb);
			if (!t->multi) {
				KillTimer(NULL, t->timer_id);
				timers = g_list_remove(timers, t);
				free(t);
			}
			return;
		}
		l = g_list_next(l);
	}
	dbg(0, "timer %d not found\n", idEvent);
}

static VOID CALLBACK win32_timer_cb(HWND hwnd, UINT uMsg, 
			UINT_PTR idEvent,
			DWORD dwTime)
{
	run_timer(idEvent);
}

static struct event_timeout *
event_win32_add_timeout(int timeout, int multi, struct callback *cb)
{
	struct event_timeout *t;
	t = calloc(1, sizeof(*t));
	if (!t)
		return t;
	t->multi = multi;
	timers = g_list_prepend(timers, t);
	t->cb = cb;
	t->timer_id = SetTimer(NULL, 0, timeout, win32_timer_cb);
	dbg(1, "Started timer %d for %d\n", t->timer_id, timeout);
	return t;
}

static void
event_win32_remove_timeout(struct event_timeout *to)
{
	GList *l;
	struct event_timeout *t;
	dbg(1,"enter:%d\n", t->timer_id);
	l = timers;
	while (l) {
		t = l->data;
		/* Use the pointer not the ID, IDs are reused */
		if (t == to) {
			KillTimer(NULL, t->timer_id);
			timers = g_list_remove(timers, t);
			free(t);
			return;
		}
		l = g_list_next(l);
	}
	dbg(0, "Timer %d not found\n", to->timer_id);
	free(to);
}

static struct event_idle *
event_win32_add_idle(struct callback *cb)
{
	dbg(0,"enter\n");
	return NULL;
}

static void
event_win32_remove_idle(struct event_idle *ev)
{
	dbg(0,"enter\n");
}

static void
event_win32_call_callback(struct callback_list *cb)
{
	PostMessage(g_hwnd, WM_USER+2, (WPARAM)cb , (LPARAM)0);
}

static struct event_methods event_win32_methods = {
	event_win32_main_loop_run,
	event_win32_main_loop_quit,
	event_win32_add_watch,
	event_win32_remove_watch,
	event_win32_add_timeout,
	event_win32_remove_timeout,
	event_win32_add_idle,
	event_win32_remove_idle,
	event_win32_call_callback,
};

static struct event_priv *
event_win32_new(struct event_methods *meth)
{
        *meth=event_win32_methods;
	return NULL;
}

void
plugin_init(void)
{
	plugin_register_graphics_type("win32", win32_graphics_new);
        plugin_register_event_type("win32", event_win32_new);
}
