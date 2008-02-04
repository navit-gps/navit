#include <windows.h>
#include <wingdi.h>
#include <glib.h>

#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "win32_gui.h"

#include "xpm2bmp.h";

#ifndef GET_WHEEL_DELTA_WPARAM
	#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

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

     SetGraphicsMode (hdc, GM_ADVANCED) ;
     ModifyWorldTransform (hdc, NULL, MWT_IDENTITY) ;
     SetViewportOrgEx (hdc, 0, 0, NULL) ;
     SetWindowOrgEx   (hdc, 0, 0, NULL) ;

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

     DPtoLP (hdc, &pt, 1) ;
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


void ErrorExit(LPTSTR lpszFunction)
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
    sprintf((LPTSTR)lpDisplayBuf, TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);

    printf( "%s\n", lpDisplayBuf );
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


struct graphics_priv *g_gra;
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
		printf( "resize memDC to: %d %d \n", Width, Height );

		hBitmap = CreateCompatibleBitmap(hdc, Width, Height );

		if ( hBitmap )
		{
			hOldBitmap = (HBITMAP) SelectObject( hMemDC, hBitmap);
		}
	}
}

static void HandleButtonClick( int updown, int button, long lParam )
{
	int xPos = LOWORD(lParam);
	int yPos = HIWORD(lParam);
	// printf( "WM_LBUTTONDOWN: %d %d \n", xPos, yPos );
	if (g_gra->button_callback )
	{
		struct point pt = {xPos, yPos};
		(*g_gra->button_callback)(g_gra->button_callback_data, updown, button, &pt);
	}
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{

//if ( Message != 15 )
	// printf( "CHILD %d %d %d \n", Message, wParam, lParam );
	switch(Message)
	{
		case WM_CREATE:
		{
			g_gra->wnd_handle = hwnd;
			//if (g_gra->resize_callback)
			// 	(*g_gra->resize_callback)(g_gra->resize_callback_data, g_gra->width, g_gra->height);
			//MoveWindow( hwnd, 0,0, 780, 680, TRUE );

			{
				HDC hdc;
				hdc = GetDC( hwnd );
				MakeMemoryDC(hwnd, hdc );
				ReleaseDC( hwnd, hdc );
			}
			PostMessage( g_gra->wnd_parent_handle, WM_USER + 1, 0, 0 );
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
		case WM_SIZE:
			{
				//graphics = GetWindowLong( hwnd, DWL_USER, 0 );

				g_gra->width = LOWORD( lParam );
				g_gra->height  = HIWORD( lParam );

				{
					HDC hdc;
					hdc = GetDC( hwnd );
					MakeMemoryDC(hwnd, hdc );
					ReleaseDC( hwnd, hdc );
				}
				printf( "resize gfx to: %d %d \n", g_gra->width, g_gra->height );

			}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			exit( 0 );
		break;
		case WM_PAINT:
			{

			HDC hdc = GetDC(hwnd );
			if ( hMemDC )
			{
				BitBlt( hdc, 0, 0, g_gra->width , g_gra->height, hMemDC, 0, 0, SRCCOPY );
			}
			ReleaseDC( hwnd, hdc );
			}
		break;
		case WM_MOUSEMOVE:
		{
			int xPos = LOWORD(lParam);
			int yPos = HIWORD(lParam);
			struct point pt = {xPos, yPos};

			// printf( "WM_MOUSEMOVE: %d %d \n", xPos, yPos );
			(*g_gra->motion_callback)(g_gra->motion_callback_data, &pt);
		}

		break;

		case WM_LBUTTONDOWN:
			HandleButtonClick( 1, 1,lParam );
		break;
		case WM_LBUTTONUP:
			HandleButtonClick( 0, 1,lParam );
		break;
		case WM_RBUTTONDOWN:
			HandleButtonClick( 1, 3,lParam );
		break;
		case WM_RBUTTONUP:
			HandleButtonClick( 0, 3,lParam );
		break;

		case WM_HSCROLL:
		case WM_VSCROLL:
			printf( "mousewheel delta %d\n", wParam );
		break;
		case WM_MOUSEWHEEL:
			printf( "mousewheel delta %d\n", wParam );

		break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}


static const char g_szClassName[] = "NAVGRA";

void CreateGraphicsWindows( struct graphics_priv* gr )
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;
    RECT rcParent;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = NULL;
	wc.hIcon		 = NULL;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm		 = NULL;


	HANDLE hdl = gr->wnd_parent_handle;
	GetClientRect( gr->wnd_parent_handle,&rcParent);

	if(!RegisterClassEx(&wc))
	{
		ErrorExit( "Window Registration Failed!" );
		return NULL;
	}

	gr->width = rcParent.right - rcParent.left;
	gr->height  = rcParent.bottom - rcParent.top;

	hwnd = CreateWindow( 	g_szClassName,
							"",
							WS_CHILD  ,
							0,
							0,
							gr->width,
							gr->height,
							gr->wnd_parent_handle,
							(HMENU)ID_CHILD_GFX,
							NULL,
							NULL);

	if(hwnd == NULL)
	{
		ErrorExit( "Window Creation Failed!" );
		return 0;
	}

	ShowWindow(hwnd, TRUE);
	UpdateWindow(hwnd);
	gr->wnd_handle = hwnd;
}



static void graphics_destroy(struct graphics_priv *gr)
{
	g_free( gr );
}


static void gc_destroy(struct graphics_gc_priv *gc)
{
	g_free( gc );
}

static HPEN line_pen;

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->line_width = w;
}

static void gc_set_dashes(struct graphics_gc_priv *gc, unsigned char *dash_list, int n)
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
	gc->hwnd = g_gra->wnd_handle;
	gc->line_width = 1;
	gc->fg_color = RGB( 0,0,0 );
	gc->bg_color = RGB( 255,255,255 );
	return gc;
}


static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
	HANDLE hndl = gr->wnd_handle;

	HDC dc = hMemDC;

	HPEN holdpen;
	HPEN hpen;

	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
	holdpen = SelectObject( dc, hpen );

	SetBkColor( dc, gc->bg_color );

	int first = 1;
	for ( i = 0; i< count; i++ )
	{
		if ( first )
		{
			first = 0;
			MoveToEx( dc, p[0].x, p[0].y, NULL );
		}
		else
		{
			LineTo( dc, p[i].x, p[i].y );
		}
	}

	SelectObject( dc, holdpen );
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
	InvalidateRect( gr->wnd_handle, NULL, TRUE );
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


static void register_resize_callback(struct graphics_priv *this_, void (*callback)(void *data, int w, int h), void *data)
{
	this_->resize_callback=callback;
	this_->resize_callback_data=data;
}

static void register_motion_callback(struct graphics_priv *this_, void (*callback)(void *data, struct point *p), void *data)
{
	this_->motion_callback=callback;
	this_->motion_callback_data=data;
}

static void register_button_callback(struct graphics_priv *this_, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
	this_->button_callback=callback;
	this_->button_callback_data=data;
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
		int size = font->size;
		long lfHeight = -MulDiv(font->size, GetDeviceCaps(hMemDC, LOGPIXELSY), 72);

		font->hfont = EzCreateFont (hMemDC, TEXT ("Arial"), size/2, 0, 0, TRUE) ;
		GetObject ( font->hfont, sizeof (LOGFONT), &font->lf) ;

/*
		font->hfont = CreateFont( 	lfHeight,
									0,
									0,
									0,
									0,
									TRUE,
									0,
									0,
									DEFAULT_CHARSET,
									0,
									0,
									0,
									0,
									"Arial");
									*/
	}


//	RECT rc = { p->x, p->y, 800, 800 };
//	DrawText(hMemDC, text, -1, &rc, DT_NOCLIP | DT_CALCRECT| DT_SINGLELINE);
	double angle = -atan2( dy, dx ) * 180 / 3.14159 ;
// printf( "dx %d , dy %d angle %6.3f %s\n", dx, dy, angle, text);
	SetTextAlign (hMemDC, TA_BASELINE) ;
	SetViewportOrgEx (hMemDC, p->x, p->y, NULL) ;
	font->lf.lfEscapement = font->lf.lfOrientation = ( angle * 10 ) ;
	DeleteObject (font->hfont) ;

	font->hfont = CreateFontIndirect (&font->lf);
	HFONT hOldFont = SelectObject(hMemDC, font->hfont );

	TextOut(hMemDC, 0,0, text, strlen( text ) );

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

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size)
{
	struct graphics_font_priv *font=g_new(struct graphics_font_priv, 1);
	*meth = font_methods;

	font->hfont = NULL;
	font->size = size;
    // FontFamily fontFamily( "Liberation Mono");
//font( &fontFamily, size, FontStyleRegular, UnitPoint );
	return font;
}

static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot)
{
	struct graphics_image_priv* ret;

	ret = g_new( struct graphics_image_priv, 1 );
	printf( "loading image '%s'\n", name );
	ret->pxpm = Xpm2bmp_new();
	Xpm2bmp_load( ret->pxpm, name );
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
	font_new,
	gc_new,
	background_gc,
	NULL, // overlay_new,
	image_new,
	get_data,
	register_resize_callback,
	register_button_callback,
	register_motion_callback,
};

static struct graphics_priv * graphics_win32_drawing_area_new_helper(struct graphics_methods *meth)
{
	struct graphics_priv *this_=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;
	g_gra = this_;
	this_->mode = -1;
	return this_;
}

struct graphics_priv* win32_graphics_new( struct graphics_methods *meth, struct attr **attrs)
{
	struct graphics_priv* this_=graphics_win32_drawing_area_new_helper(meth);
	return this_;
}
