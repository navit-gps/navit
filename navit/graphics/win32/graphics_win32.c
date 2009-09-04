#include <windows.h>
#include <windowsx.h>
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
#include "window.h"
#include "graphics_win32.h"
#include "xpm2bmp.h"
#include "support/win32/ConvertUTF.h"

struct graphics_priv
{
	struct navit *nav;
	struct window window;
	struct point p;
	int width;
	int height;
	int disabled;
	HANDLE wnd_parent_handle;
	HANDLE wnd_handle;
	COLORREF bg_color;
	struct callback_list *cbl;
	enum draw_mode_num mode;
	struct graphics_priv* parent;
	struct graphics_priv *overlays;
	struct graphics_priv *next;
	struct color transparent_color;
	HDC hMemDC;
	HDC hPrebuildDC;
	HBITMAP hBitmap;
	HBITMAP hPrebuildBitmap;
};

static HWND g_hwnd = NULL;

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

struct graphics_image_priv
{
	PXPM2BMP pxpm;
	int width,height,row_bytes,channels;
	unsigned char *png_pixels;
	struct point hot;
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

	dbg(0, "%s failed with error %d: %s", lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}



struct graphics_gc_priv
{
	HWND        hwnd;
	int         line_width;
	COLORREF    fg_color;
	int         fg_alpha;
	COLORREF    bg_color;
	struct graphics_priv *gr;
};


static void create_memory_dc(struct graphics_priv *gr)
{
	HDC hdc;
	hdc = GetDC( gr->wnd_handle );
	// Creates memory DC
	gr->hMemDC = CreateCompatibleDC(hdc);
	if ( gr->hMemDC )
	{
		RECT rectRgn;
		GetClientRect( gr->wnd_handle, &rectRgn );

		int Width = rectRgn.right - rectRgn.left;
		int Height = rectRgn.bottom - rectRgn.top;

		if ( gr->parent )
		{
			Width = gr->width;
			Height = gr->height;
			dbg(0, "resize overlay memDC to: %d %d \n", Width, Height );
		}
		else
		{
			dbg(0, "resize memDC to: %d %d \n", Width, Height );
		}

		gr->hBitmap = CreateCompatibleBitmap(hdc, Width, Height );

		if ( gr->hBitmap )
		{
			SelectObject( gr->hMemDC, gr->hBitmap);
		}
	}
	ReleaseDC( gr->wnd_handle, hdc );
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

	struct graphics_priv* gra_priv = (struct graphics_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

	switch (Message)
	{
	case WM_CREATE:
	{
		if ( gra_priv )
		{
			create_memory_dc(gra_priv);
		}
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
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

			GetClientRect( hwnd, &rc );
			gra_priv->width = rc.right;
			gra_priv->height = rc.bottom;

			create_memory_dc(gra_priv);
			callback_list_call_attr_2(gra_priv->cbl, attr_resize, (void *)gra_priv->width, (void *)gra_priv->height);
		}
		break;
	case WM_USER+2:
	{
		struct callback_list *cbl = (struct callback_list*)wParam;
#ifdef HAVE_API_WIN32_CE
		/* FIXME: Reset the idle timer  need a better place */
		SystemIdleTimerReset();
#endif
		callback_list_call_0(cbl);
	}
	break;

	case WM_SIZE:
		if ( gra_priv )
		{
			gra_priv->width = LOWORD( lParam );
			gra_priv->height  = HIWORD( lParam );
			create_memory_dc(gra_priv);
			dbg(0, "resize gfx to: %d %d \n", gra_priv->width, gra_priv->height );
			callback_list_call_attr_2(gra_priv->cbl, attr_resize, (void *)gra_priv->width, (void *)gra_priv->height);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		if ( gra_priv && gra_priv->hMemDC)
		{
			dbg(0, "WM_PAINT\n");
			struct graphics_priv* overlay = gra_priv->overlays;

			while ( !gra_priv->disabled && overlay && !overlay->disabled )
			{
				if ( overlay->p.x < gra_priv->width && overlay->p.y < gra_priv->height )
				{

#ifdef  FAST_TRANSPARENCY
					if ( !overlay->hMaskDC )
					{
						overlay->hPrebuildBitmap = CreateBitmap(overlay->width,overlay->height,1,1,NULL);
						overlay->hPrebuildDC = CreateCompatibleDC(NULL);
						SelectObject(overlay->hPrebuildDC,overlay->hPrebuildBitmap);
						SetBkColor(overlay->hMemDC,RGB(overlay->transparent_color.r >> 8,overlay->transparent_color.g >> 8,overlay->transparent_color.b >> 8));
						BitBlt(overlay->hPrebuildDC,0,0,overlay->width,overlay->height,overlay->hMemDC,0,0,SRCCOPY);
						BitBlt(overlay->hMemDC,0,0,overlay->width,overlay->height,overlay->hPrebuildDC,0,0,SRCINVERT);
					}

#else
					int x;
					int y;
					COLORREF newColor;
					const COLORREF transparent_color = RGB(overlay->transparent_color.r >> 8,overlay->transparent_color.g >> 8,overlay->transparent_color.b >> 8);

					overlay->hPrebuildBitmap =  CreateCompatibleBitmap(overlay->hMemDC, overlay->width, overlay->height );
					overlay->hPrebuildDC = CreateCompatibleDC(NULL);
					SelectObject(overlay->hPrebuildDC,overlay->hPrebuildBitmap);

					for ( y = 0; y < overlay->height;y++ )
					{
						for ( x = 0; x < overlay->width; x++ )
						{
							newColor = GetPixel( overlay->hMemDC, x, y);

							if ( newColor == transparent_color )
							{
								COLORREF origColor = GetPixel( overlay->parent->hMemDC, x + overlay->p.x, y + overlay->p.y );
								newColor = RGB ( ((65535 - overlay->transparent_color.a) * GetRValue(origColor) + overlay->transparent_color.a * GetRValue(newColor)) / 65535,
												 ((65535 - overlay->transparent_color.a) * GetGValue(origColor) + overlay->transparent_color.a * GetGValue(newColor)) / 65535,
												 ((65535 - overlay->transparent_color.a) * GetBValue(origColor) + overlay->transparent_color.a * GetBValue(newColor)) / 65535);
							}

							SetPixel( overlay->hPrebuildDC, x , y , newColor);
						}

					}
#endif
				}
				overlay = overlay->next;
			}

			PAINTSTRUCT ps = { 0 };
			overlay = gra_priv->overlays;

			HDC hdc = BeginPaint(hwnd, &ps);
			BitBlt( hdc, 0, 0, gra_priv->width , gra_priv->height, gra_priv->hMemDC, 0, 0, SRCCOPY );

			while ( !gra_priv->disabled && overlay && !overlay->disabled )
			{
				if ( overlay->p.x < gra_priv->width && overlay->p.y < gra_priv->height )
				{
#ifdef  FAST_TRANSPARENCY
					BitBlt(hdc,overlay->p.x,overlay->p.y,overlay->width,overlay->height,overlay->hPrebuildDC,0,0,SRCAND);
					BitBlt(hdc,overlay->p.x,overlay->p.y,overlay->width,overlay->height,overlay->hMemDC,0,0,SRCPAINT);
#else
					BitBlt(hdc,overlay->p.x,overlay->p.y,overlay->width,overlay->height, overlay->hPrebuildDC, 0, 0,SRCCOPY);
#endif
				}
				overlay = overlay->next;
			}
			EndPaint(hwnd, &ps);
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
	{
		dbg(1, "LBUTTONDOWN\n");
		HandleButtonClick( gra_priv, 1, 1, lParam);
	}
	break;
	case WM_LBUTTONUP:
	{
		dbg(1, "LBUTTONUP\n");
		HandleButtonClick( gra_priv, 0, 1, lParam);
	}
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


static const TCHAR g_szClassName[] = {'N','A','V','G','R','A','\0'};

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
	if (!RegisterClass(&wc))
#else
	if (!RegisterClassEx(&wc))
#endif
	{
		ErrorExit( TEXT("Window Registration Failed!") );
		return NULL;
	}

	gr->width = rcParent.right - rcParent.left;
	gr->height  = rcParent.bottom - rcParent.top;
	callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->width, (void *)gr->height);

	g_hwnd = hwnd = CreateWindow(g_szClassName,
								 TEXT(""),
#ifdef HAVE_API_WIN32_CE
								 WS_VISIBLE,
#elif 1
								 WS_CHILD,
#else
								 WS_OVERLAPPEDWINDOW,
#endif
								 0,
								 0,
								 gr->width,
								 gr->height,
#if 1
								 gr->wnd_parent_handle,
#else
								 NULL,
#endif
#if 1
								 (HMENU)ID_CHILD_GFX,
#else
								 NULL,
#endif
								 GetModuleHandle(NULL),
								 NULL);

	if (hwnd == NULL)
	{
		ErrorExit( TEXT("Window Creation Failed!") );
		return NULL;
	}
	gr->wnd_handle = hwnd;

	create_memory_dc(gr);

	SetWindowLongPtr( hwnd , DWLP_USER, (LONG_PTR)gr );

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


static void gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->fg_color = RGB( c->r >> 8, c->g >> 8, c->b >> 8);
	gc->fg_alpha = c->a;

	if ( gc->gr && c->a < 0xFFFF )
	{
		gc->gr->transparent_color = *c;
	}

}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc->bg_color = RGB( c->r >> 8, c->g >> 8, c->b >> 8);
	if ( gc->gr && gc->gr->hMemDC )
		SetBkColor( gc->gr->hMemDC, gc->bg_color );

}

static struct graphics_gc_methods gc_methods =
{
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,
	gc_set_foreground,
	gc_set_background,
	NULL, //gc_set_stipple
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);
	*meth=gc_methods;
	gc->hwnd = gr->wnd_handle;
	gc->line_width = 1;
	gc->fg_color = RGB( 0,0,0 );
	gc->bg_color = RGB( 255,255,255 );
	gc->gr = gr;
	return gc;
}


static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int i;
	HPEN hpen;

	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
	SelectObject( gr->hMemDC, hpen );

	SetBkColor( gr->hMemDC, gc->bg_color );

	int first = 1;
	for ( i = 0; i< count; i++ )
	{
		if ( first )
		{
			first = 0;
			MoveToEx( gr->hMemDC, p[0].x, p[0].y, NULL );
		}
		else
		{
			LineTo( gr->hMemDC, p[i].x, p[i].y );
		}
	}

	DeleteObject( hpen );
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{

	int i;
	POINT points[ count ];
	for ( i=0;i< count; i++ )
	{
		points[i].x = p[i].x;
		points[i].y = p[i].y;
	}
	HPEN hpen;
	HBRUSH hbrush;

	SetBkColor( gr->hMemDC, gc->bg_color );

	hpen = CreatePen( PS_NULL, gc->line_width, gc->fg_color );
	SelectObject( gr->hMemDC, hpen );
	hbrush = CreateSolidBrush( gc->fg_color );
	SelectObject( gr->hMemDC, hbrush );

	Polygon( gr->hMemDC, points,count );

	DeleteObject( hbrush );
	DeleteObject( hpen );
}


static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	HPEN hpen;
	HBRUSH hbrush;

	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
	SelectObject( gr->hMemDC, hpen );
	hbrush = CreateSolidBrush( gc->fg_color );
	SelectObject( gr->hMemDC, hbrush );
	SetBkColor( gr->hMemDC, gc->bg_color );

	Rectangle(gr->hMemDC, p->x, p->y, p->x+w, p->y+h);

	DeleteObject( hpen );
	DeleteObject( hbrush );

}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	HPEN hpen;
	HBRUSH hbrush;
	r=r/2;

	hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
	SelectObject( gr->hMemDC, hpen );

	hbrush = CreateSolidBrush( RGB(gr->transparent_color.r >> 8, gr->transparent_color.g >> 8, gr->transparent_color.b >> 8) );
	SelectObject( gr->hMemDC, hbrush );

	SetBkColor( gr->hMemDC, gc->bg_color );

	Ellipse( gr->hMemDC, p->x - r, p->y -r, p->x + r, p->y + r );

	DeleteObject( hpen );
	DeleteObject( hbrush );
}



static void draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	InvalidateRect( gr->wnd_handle, NULL, FALSE );
}

static void draw_drag(struct graphics_priv *gr, struct point *p)
{
	if ( p )
	{
		gr->p.x    = p->x;
		gr->p.y    = p->y;
	}
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	dbg( 0, "set draw_mode to %x, %d\n", gr, (int)mode );

	if ( mode == draw_mode_begin )
	{
		if ( gr->wnd_handle == NULL )
		{
			CreateGraphicsWindows( gr );
		}
		if ( gr->mode != draw_mode_begin )
		{
			if ( gr->hMemDC )
			{
				dbg(0, "Erase dc: %x, w: %d, h: %d, bg_color: %x\n", gr, gr->width, gr->height, gr->bg_color);
#if 0
				RECT rcClient = { 0, 0, gr->width, gr->height};
				HBRUSH bgBrush = CreateSolidBrush( gr->bg_color  );
				FillRect( gr->hMemDC, &rcClient, bgBrush );
				DeleteObject( bgBrush );
#endif
				if ( gr->hPrebuildDC )
				{
					DeleteBitmap(gr->hPrebuildBitmap);
					DeleteDC(gr->hPrebuildDC);
					gr->hPrebuildDC = 0;
				}
			}
		}
		else if ( gr->hPrebuildDC )
		{
			DeleteBitmap(gr->hPrebuildBitmap);
			DeleteDC(gr->hPrebuildDC);
			gr->hPrebuildDC = 0;
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
	if (!strcmp(type, "window"))
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

		GetClientRect( this_->wnd_parent_handle,&rcParent);

#ifdef HAVE_API_WIN32_CE
		if (!RegisterClass(&wc))
#else
		if (!RegisterClassEx(&wc))
#endif
		{
			ErrorExit( TEXT("Window Registration Failed!") );
			return NULL;
		}

		callback_list_call_attr_2(this_->cbl, attr_resize, (void *)this_->width, (void *)this_->height);

		g_hwnd = hwnd = CreateWindow(g_szClassName,
									 TEXT(""),
#ifdef HAVE_API_WIN32_CE
									 WS_VISIBLE,
#else
									 WS_OVERLAPPEDWINDOW,
#endif
									 0,
									 0,
									 this_->width,
									 this_->height,
									 this_->wnd_parent_handle,
									 NULL,
									 GetModuleHandle(NULL),
									 NULL);

		if (hwnd == NULL)
		{
			ErrorExit( TEXT("Window Creation Failed!") );
			return NULL;
		}
		this_->wnd_handle = hwnd;

		SetWindowLongPtr( hwnd , DWLP_USER, (LONG_PTR)this_ );

		ShowWindow( hwnd, TRUE );
		UpdateWindow( hwnd );

		create_memory_dc(this_);

		PostMessage( this_->wnd_parent_handle, WM_USER + 1, 0, 0 );

		this_->window.priv=this_;
		return &this_->window;
	}
	return NULL;
}


static void background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	RECT rcClient = { 0, 0, gr->width, gr->height };
	HBRUSH bgBrush;

	bgBrush = CreateSolidBrush( gc->bg_color  );
	gr->bg_color = gc->bg_color;

	FillRect( gr->hMemDC, &rcClient, bgBrush );
	DeleteObject( bgBrush );
}

struct graphics_font_priv
{
	LOGFONT lf;
	HFONT hfont;
	int size;
};

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	RECT rcClient;
	GetClientRect( gr->wnd_handle, &rcClient );

	SetTextColor(gr->hMemDC, fg->fg_color);
	int prevBkMode = SetBkMode( gr->hMemDC, TRANSPARENT );

	if ( NULL == font->hfont )
	{
#ifdef WIN_USE_SYSFONT
		font->hfont = (HFONT) GetStockObject (SYSTEM_FONT);
		GetObject (font->hfont, sizeof (LOGFONT), &font->lf);
#else
		font->hfont = EzCreateFont (gr->hMemDC, TEXT ("Arial"), font->size/2, 0, 0, TRUE);
		GetObject ( font->hfont, sizeof (LOGFONT), &font->lf) ;
#endif
	}


	double angle = -atan2( dy, dx ) * 180 / 3.14159 ;
	if (angle < 0)
		angle += 360;

	SetTextAlign (gr->hMemDC, TA_BASELINE) ;
	SetViewportOrgEx (gr->hMemDC, p->x, p->y, NULL) ;
	font->lf.lfEscapement = font->lf.lfOrientation = ( angle * 10 ) ;
	DeleteObject (font->hfont) ;

	font->hfont = CreateFontIndirect (&font->lf);
	HFONT hOldFont = SelectObject(gr->hMemDC, font->hfont );

	{
		wchar_t utf16[1024];
		const UTF8 *utf8 = (UTF8 *)text;
		UTF16 *utf16p = (UTF16 *) utf16;
		SetBkMode (gr->hMemDC, TRANSPARENT);
		if (ConvertUTF8toUTF16(&utf8, utf8+strlen(text),
							   &utf16p, utf16p+sizeof(utf16),
							   lenientConversion) == conversionOK)
		{
#ifdef _WIN32_WCE
			ExtTextOut (gr->hMemDC, 0, 0, 0, NULL,
						utf16, (wchar_t*) utf16p - utf16, NULL);
#else
			ExtTextOutW(gr->hMemDC, 0, 0, 0, NULL,
						utf16, (wchar_t*) utf16p - utf16, NULL);
#endif
		}
	}


	SelectObject(gr->hMemDC, hOldFont);
	DeleteObject (font->hfont) ;

	SetBkMode( gr->hMemDC, prevBkMode );

	SetViewportOrgEx (gr->hMemDC, 0, 0, NULL) ;
}

static void font_destroy(struct graphics_font_priv *font)
{
	if ( font->hfont )
	{
		DeleteObject(font->hfont);
	}
	g_free(font);
}

static struct graphics_font_methods font_methods =
{
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *name, int size, int flags)
{
	struct graphics_font_priv *font=g_new(struct graphics_font_priv, 1);
	*meth = font_methods;

	font->hfont = NULL;
	font->size = size;

	return font;
}

static void image_cache_hash_add( const char* key, struct graphics_image_priv* val_ptr)
{
	if ( image_cache_hash == NULL )
	{
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



#include "png.h"

static int
pngdecode(char *name, struct graphics_image_priv *img)
{
  png_struct    *png_ptr = NULL;
  png_info	*info_ptr = NULL;
  png_byte      buf[8];
  png_byte      **row_pointers = NULL;
  png_byte      *pix_ptr = NULL;

  int           bit_depth;
  int           color_type;
  int           alpha_present;
  int           row, col;
  int           ret;
  int           i;
  long          dep_16;
  FILE *png_file;

	dbg(0,"enter %s\n",name);
  png_file=fopen(name, "rb");
  if (!png_file)
    return FALSE;


  /* read and check signature in PNG file */
  ret = fread (buf, 1, 8, png_file);
  if (ret != 8) {
    fclose(png_file);
    return FALSE;
  }

  ret = png_check_sig (buf, 8);
  if (!ret) {
    fclose(png_file);
    return FALSE;
  }

  /* create png and info structures */

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(png_file);
    return FALSE;   /* out of memory */
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
  {
    fclose(png_file);
    png_destroy_read_struct (&png_ptr, NULL, NULL);
    return FALSE;   /* out of memory */
  }

  if (setjmp (png_jmpbuf(png_ptr)))
  {
    fclose(png_file);
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return FALSE;
  }

  /* set up the input control for C streams */
  png_init_io (png_ptr, png_file);
  png_set_sig_bytes (png_ptr, 8);  /* we already read the 8 signature bytes */

  /* read the file information */
  png_read_info (png_ptr, info_ptr);

  /* get size and bit-depth of the PNG-image */
  png_get_IHDR (png_ptr, info_ptr,
    &img->width, &img->height, &bit_depth, &color_type,
    NULL, NULL, NULL);

  /* set-up the transformations */

  /* transform paletted images into full-color rgb */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand (png_ptr);
  /* expand images to bit-depth 8 (only applicable for grayscale images) */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand (png_ptr);
  /* transform transparency maps into full alpha-channel */
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand (png_ptr);

  /* all transformations have been registered; now update info_ptr data,
   * get rowbytes and channels, and allocate image memory */

  png_read_update_info (png_ptr, info_ptr);

  /* get the new color-type and bit-depth (after expansion/stripping) */
  png_get_IHDR (png_ptr, info_ptr, &img->width, &img->height, &bit_depth, &color_type,
    NULL, NULL, NULL);

  /* calculate new number of channels and store alpha-presence */
  if (color_type == PNG_COLOR_TYPE_GRAY)
    img->channels = 1;
  else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    img->channels = 2;
  else if (color_type == PNG_COLOR_TYPE_RGB)
    img->channels = 3;
  else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    img->channels = 4;
  else
    img->channels = 0; /* should never happen */
  alpha_present = (img->channels - 1) % 2;

  /* row_bytes is the width x number of channels x (bit-depth / 8) */
  img->row_bytes = png_get_rowbytes (png_ptr, info_ptr);

  if ((img->png_pixels = (png_byte *) malloc (img->row_bytes * img->height * sizeof (png_byte))) == NULL) {
    fclose(png_file);
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return FALSE;
  }

  if ((row_pointers = (png_byte **) malloc (img->height * sizeof (png_bytep))) == NULL)
  {
    fclose(png_file);
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    free (img->png_pixels);
    img->png_pixels = NULL;
    return FALSE;
  }

  /* set the individual row_pointers to point at the correct offsets */
  for (i = 0; i < (img->height); i++)
    row_pointers[i] = img->png_pixels + i * img->row_bytes;

  /* now we can go ahead and just read the whole image */
  png_read_image (png_ptr, row_pointers);

  /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
  png_read_end (png_ptr, info_ptr);

  /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);

#if 0
  pix_ptr = png_pixels;

  for (row = 0; row < height; row++)
  {
    for (col = 0; col < width; col++)
    {
      for (i = 0; i < (channels - alpha_present); i++)
      {
        if (raw)
          fputc ((int) *pix_ptr++ , pnm_file);
        else
          if (bit_depth == 16){
	    dep_16 = (long) *pix_ptr++;
            fprintf (pnm_file, "%ld ", (dep_16 << 8) + ((long) *pix_ptr++));
          }
          else
            fprintf (pnm_file, "%ld ", (long) *pix_ptr++);
      }
      if (alpha_present)
      {
        if (!alpha)
        {
          pix_ptr++; /* alpha */
          if (bit_depth == 16)
            pix_ptr++;
        }
        else /* output alpha-channel as pgm file */
        {
          if (raw)
            fputc ((int) *pix_ptr++ , alpha_file);
          else
            if (bit_depth == 16){
	      dep_16 = (long) *pix_ptr++;
              fprintf (alpha_file, "%ld ", (dep_16 << 8) + (long) *pix_ptr++);
	    }
            else
              fprintf (alpha_file, "%ld ", (long) *pix_ptr++);
        }
      } /* if alpha_present */

      if (!raw)
        if (col % 4 == 3)
          fprintf (pnm_file, "\n");
    } /* end for col */

    if (!raw)
      if (col % 4 != 0)
        fprintf (pnm_file, "\n");
  } /* end for row */

#endif
  if (row_pointers != (unsigned char**) NULL)
    free (row_pointers);
#if 0
  if (png_pixels != (unsigned char*) NULL)
    free (png_pixels);
#endif
  img->hot.x=img->width/2-1;
  img->hot.y=img->height/2-1;
dbg(0,"ok\n");
  fclose(png_file);
  return TRUE;

} /* end of source */

static void
pngrender(struct graphics_image_priv *img, struct graphics_priv *gr, int x0,int y0)
{
	int x,y;
	HDC hdc=gr->hMemDC;
	int w=gr->width;
	int h=gr->height;
  	png_byte *pix_ptr = img->png_pixels;

	dbg(0,"enter %d,%d %dx%d %d\n",x0,y0,img->width,img->height,img->channels);

	for (y=0 ; y < img->width ; y++) {
		for (x=0 ; x < img->height ; x++) {
			int xdst=x0+x,ydst=y0+y;
			if (xdst >= 0 && ydst >= 0 && xdst < w && ydst < h) {
				COLORREF newColor, origColor;
				int a=pix_ptr[3];
				if (a != 0xff) {
					int ai=0xff-a;
					origColor = GetPixel( hdc, xdst, ydst);
					newColor = RGB ( (pix_ptr[0]*a+GetRValue(origColor)*ai)/255,
					                 (pix_ptr[1]*a+GetGValue(origColor)*ai)/255,
					                 (pix_ptr[2]*a+GetBValue(origColor)*ai)/255);
				} else
					newColor = RGB ( pix_ptr[0], pix_ptr[1], pix_ptr[2]);
                                SetPixel( hdc, xdst, ydst, newColor);
			}
			pix_ptr+=4;
		}
	}
}

static int
xpmdecode(char *name, struct graphics_image_priv *img)
{
	img->pxpm = Xpm2bmp_new();
	if (Xpm2bmp_load( img->pxpm, name ) != 0) {
		free(img->pxpm);
		return FALSE;
	}
	img->width=img->pxpm->size_x;
	img->height=img->pxpm->size_y;
	img->hot.x=img->pxpm->hotspot_x;
	img->hot.y=img->pxpm->hotspot_y;
	return TRUE;
}


static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot, int rotation)
{
	struct graphics_image_priv* ret;
	int len=strlen(name);
	char *ext;
	int rc=0;

	if (len < 4)
		return NULL;
	ext=name+len-4;
	if ( NULL == ( ret = image_cache_hash_lookup( name ) ) )
	{
		ret = g_new0( struct graphics_image_priv, 1 );
		dbg(2, "loading image '%s'\n", name );
		if (!strcmp(ext,".xpm")) {
			rc=xpmdecode(name, ret);
		} else if (!strcmp(ext,".png")) {
			rc=pngdecode(name, ret);
		}
		if (rc) {
			image_cache_hash_add( name, ret );
			*w=ret->width;
			*h=ret->height;
			if (hot)
				*hot=ret->hot;
		} else {
			g_free(ret);
			ret=NULL;
		}
	}
	return ret;
}

static void draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	if (img->pxpm)
		Xpm2bmp_paint( img->pxpm , gr->hMemDC, p->x, p->y );
	if (img->png_pixels)
		pngrender(img, gr, p->x, p->y);
}

static struct graphics_priv *
			graphics_win32_new_helper(struct graphics_methods *meth);

static void overlay_resize(struct graphics_priv *gr, struct point *p, int w, int h, int alpha, int wraparound)
{
	dbg(0, "resize overlay: %x, x: %d, y: %d, w: %d, h: %d, alpha: %x, wraparound: %d\n", gr, p->x, p->y, w, h, alpha, wraparound);

	if ( gr->width != w || gr->height != h )
	{
		gr->width  = w;
		gr->height = h;
// TODO (Rikky#1#): should be deleted first
		create_memory_dc(gr);
	}
	gr->p.x    = p->x;
	gr->p.y    = p->y;

}


static struct graphics_priv *
			overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound)
{
	struct graphics_priv *this=graphics_win32_new_helper(meth);
	dbg(1, "overlay: %x, x: %d, y: %d, w: %d, h: %d, alpha: %x, wraparound: %d\n", this, p->x, p->y, w, h, alpha, wraparound);
	this->width  = w;
	this->height = h;
	this->parent = gr;
	this->p.x    = p->x;
	this->p.y    = p->y;
	this->disabled = 0;
	this->hPrebuildDC = 0;

	this->next = gr->overlays;
	gr->overlays = this;
	this->wnd_handle = gr->wnd_handle;

	if (wraparound)
	{
		if ( p->x < 0 )
		{
			this->p.x += gr->width;
		}

		if ( p->y < 0 )
		{
			this->p.y += gr->height;
		}
	}

	create_memory_dc(this);
	return this;
}

static void overlay_disable(struct graphics_priv *gr, int disable)
{
	dbg(1, "overlay: %x, disable: %d\n", gr, disable);
	gr->disabled = disable;
}

static void get_text_bbox(struct graphics_priv *gr, struct graphics_font_priv *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
	dbg(1, "Get bbox for %s\n", text);
	int len = g_utf8_strlen(text, -1);
	int xMin = 0;
	int yMin = 0;
	int yMax = 13*font->size/256;
	int xMax = 9*font->size*len/256;

	ret[0].x = xMin;
	ret[0].y = -yMin;
	ret[1].x = xMin;
	ret[1].y = -yMax;
	ret[2].x = xMax;
	ret[2].y = -yMax;
	ret[3].x = xMax;
	ret[3].y = -yMin;
}


static struct graphics_methods graphics_methods =
{
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
	draw_drag,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	NULL,   //image_free
	get_text_bbox,
	overlay_disable,
	overlay_resize,
};


static struct graphics_priv *
			graphics_win32_new_helper(struct graphics_methods *meth)
{
	struct graphics_priv *this_=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;
	this_->mode = -1;
	return this_;
}



static struct graphics_priv*
			graphics_win32_new( struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct attr *attr;

	struct graphics_priv* this_;
	if (!event_request_system("win32","graphics_win32"))
		return NULL;
	this_=graphics_win32_new_helper(meth);
	this_->nav=nav;
	this_->width=792;
	if ((attr=attr_search(attrs, NULL, attr_w)))
		this_->width=attr->u.num;
	this_->height=547;
	if ((attr=attr_search(attrs, NULL, attr_h)))
		this_->height=attr->u.num;
	this_->overlays = NULL;
	this_->cbl=cbl;
	this_->parent = NULL;
	return this_;
}


static void
event_win32_main_loop_run(void)
{
	MSG msg;

	dbg(0,"enter\n");
	while (GetMessage(&msg, 0, 0, 0))
	{
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
			event_win32_add_watch(void *h, enum event_watch_cond cond, struct callback *cb)
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
struct event_timeout
{
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
	while (l)
	{
		t = l->data;
		if (t->timer_id == idEvent)
		{
			callback_call_0(t->cb);
			if (!t->multi)
			{
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
	dbg(0,"enter:\n");
	if (to)
	{
		GList *l;
		struct event_timeout *t=NULL;
		l = timers;
		while (l)
		{
			t = l->data;
			/* Use the pointer not the ID, IDs are reused */
			if (t == to)
			{
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
}

static struct event_idle *
			event_win32_add_idle(int priority, struct callback *cb)
{
	return (struct event_idle *)event_win32_add_timeout(1, 1, cb);
}

static void
event_win32_remove_idle(struct event_idle *ev)
{
	event_win32_remove_timeout((struct event_timeout *)ev);
}

static void
event_win32_call_callback(struct callback_list *cb)
{
	PostMessage(g_hwnd, WM_USER+2, (WPARAM)cb , (LPARAM)0);
}

static struct event_methods event_win32_methods =
{
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
	plugin_register_graphics_type("win32", graphics_win32_new);
	plugin_register_event_type("win32", event_win32_new);
}
