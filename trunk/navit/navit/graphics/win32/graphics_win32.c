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
#include "profile.h"
#include "keys.h"

//#define FAST_TRANSPARENCY 1

typedef BOOL (WINAPI *FP_AlphaBlend) ( HDC hdcDest,
                                       int nXOriginDest,
                                       int nYOriginDest,
                                       int nWidthDest,
                                       int nHeightDest,
                                       HDC hdcSrc,
                                       int nXOriginSrc,
                                       int nYOriginSrc,
                                       int nWidthSrc,
                                       int nHeightSrc,
                                       BLENDFUNCTION blendFunction
                                     );

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
    DWORD* pPixelData;
    HDC hMemDC;
    HDC hPrebuildDC;
    HBITMAP hBitmap;
    HBITMAP hPrebuildBitmap;
    HBITMAP hOldBitmap;
    HBITMAP hOldPrebuildBitmap;
    FP_AlphaBlend AlphaBlend;
    HANDLE hCoreDll;
};

struct window_priv
{
    HANDLE hBackLight;
};

static HWND g_hwnd = NULL;

#ifdef HAVE_API_WIN32_CE
static int fullscr = 0;
#endif


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
    HBITMAP hBitmap;
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
    _tprintf((LPTSTR)lpDisplayBuf, TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);

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
    HPEN hpen;
    HBRUSH hbrush;
    struct graphics_priv *gr;
};


static void create_memory_dc(struct graphics_priv *gr)
{
    HDC hdc;
    BITMAPINFO bOverlayInfo;

    if (gr->hMemDC)
    {
        (void)SelectBitmap(gr->hMemDC, gr->hOldBitmap);
        DeleteBitmap(gr->hBitmap);
        DeleteDC(gr->hMemDC);

        (void)SelectBitmap(gr->hPrebuildDC, gr->hOldPrebuildBitmap);
        DeleteBitmap(gr->hPrebuildBitmap);
        DeleteDC(gr->hPrebuildDC);
        gr->hPrebuildDC = 0;
    }


    hdc = GetDC( gr->wnd_handle );
    // Creates memory DC
    gr->hMemDC = CreateCompatibleDC(hdc);
    dbg(0, "resize memDC to: %d %d \n", gr->width, gr->height );


#ifndef  FAST_TRANSPARENCY

    memset(&bOverlayInfo, 0, sizeof(BITMAPINFO));
    bOverlayInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bOverlayInfo.bmiHeader.biWidth = gr->width;
    bOverlayInfo.bmiHeader.biHeight = -gr->height;
    bOverlayInfo.bmiHeader.biBitCount = 32;
    bOverlayInfo.bmiHeader.biCompression = BI_RGB;
    bOverlayInfo.bmiHeader.biPlanes = 1;
    gr->hPrebuildDC = CreateCompatibleDC(NULL);
    gr->hPrebuildBitmap = CreateDIBSection(gr->hMemDC, &bOverlayInfo, DIB_RGB_COLORS , (void **)&gr->pPixelData, NULL, 0);
    gr->hOldPrebuildBitmap = SelectBitmap(gr->hPrebuildDC, gr->hPrebuildBitmap);

#endif
    gr->hBitmap = CreateCompatibleBitmap(hdc, gr->width, gr->height );

    if ( gr->hBitmap )
    {
        gr->hOldBitmap = SelectBitmap( gr->hMemDC, gr->hBitmap);
    }
    ReleaseDC( gr->wnd_handle, hdc );
}

static void HandleButtonClick( struct graphics_priv *gra_priv, int updown, int button, long lParam )
{
    struct point pt = { LOWORD(lParam), HIWORD(lParam) };
    callback_list_call_attr_3(gra_priv->cbl, attr_button, (void *)updown, (void *)button, (void *)&pt);
}

static void HandleKeyChar(struct graphics_priv *gra_priv, WPARAM wParam)
{
    TCHAR key = (TCHAR) wParam;
    char *s=NULL;
    char k[]={0,0};
    dbg(1,"HandleKey %d\n",key);
	switch (key) {
    default:
		k[0]=key;
		s=k;
        break;
    }
    if (s) 
        callback_list_call_attr_1(gra_priv->cbl, attr_keypress, (void *)s);
}

static void HandleKeyDown(struct graphics_priv *gra_priv, WPARAM wParam)
{
    int key = (int) wParam;
    char up[]={NAVIT_KEY_UP,0};
    char down[]={NAVIT_KEY_DOWN,0};
    char left[]={NAVIT_KEY_LEFT,0};
    char right[]={NAVIT_KEY_RIGHT,0};
    char *s=NULL;
    dbg(1,"HandleKey %d\n",key);
	switch (key) {
    case 37:
		s=left;
        break;
    case 38:
		s=up;
        break;
    case 39:
		s=right;
        break;
    case 40:
		s=down;
        break;
    }
    if (s) 
        callback_list_call_attr_1(gra_priv->cbl, attr_keypress, (void *)s);
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
            RECT rc ;

            GetClientRect( hwnd, &rc );
            gra_priv->width = rc.right;
            gra_priv->height = rc.bottom;
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
#ifdef HAVE_API_WIN32_CE
        if ( gra_priv && gra_priv->window.priv )
        {
            struct window_priv *win_priv = gra_priv->window.priv;
            if (win_priv->hBackLight)
            {
                ReleasePowerRequirement(win_priv->hBackLight);
            }
        }
#endif
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        if ( gra_priv && gra_priv->hMemDC)
        {
    	    struct graphics_priv* overlay;
            PAINTSTRUCT ps = { 0 };
            HDC hdc;
            profile(0, NULL);
            dbg(1, "WM_PAINT\n");
            overlay = gra_priv->overlays;

#ifndef FAST_TRANSPARENCY
			BitBlt( gra_priv->hPrebuildDC, 0, 0, gra_priv->width , gra_priv->height, gra_priv->hMemDC, 0, 0, SRCCOPY);
#endif
			while ( !gra_priv->disabled && overlay)
			{
				if ( !overlay->disabled && overlay->p.x >= 0 &&
					 overlay->p.y >= 0 &&
					 overlay->p.x < gra_priv->width &&
					 overlay->p.y < gra_priv->height )
				{
					int x,y;
					int destPixel, srcPixel;
                    int h,w;
#ifdef FAST_TRANSPARENCY
					if ( !overlay->hPrebuildDC )
					{
						overlay->hPrebuildBitmap = CreateBitmap(overlay->width,overlay->height,1,1,NULL);
						overlay->hPrebuildDC = CreateCompatibleDC(NULL);
						overlay->hOldPrebuildBitmap = SelectBitmap( overlay->hPrebuildDC, overlay->hPrebuildBitmap);
						SetBkColor(overlay->hMemDC,RGB(overlay->transparent_color.r >> 8,overlay->transparent_color.g >> 8,overlay->transparent_color.b >> 8));
						BitBlt(overlay->hPrebuildDC,0,0,overlay->width,overlay->height,overlay->hMemDC,0,0,SRCCOPY);
						BitBlt(overlay->hMemDC,0,0,overlay->width,overlay->height,overlay->hPrebuildDC,0,0,SRCINVERT);
					}

#else
					const COLORREF transparent_color = RGB(overlay->transparent_color.r >> 8,overlay->transparent_color.g >> 8,overlay->transparent_color.b >> 8);

					BitBlt( overlay->hPrebuildDC, 0, 0, overlay->width , overlay->height, overlay->hMemDC, 0, 0, SRCCOPY);

					h=overlay->height;
					w=overlay->width;
					if (w > gra_priv->width-overlay->p.x)
						w=gra_priv->width-overlay->p.x;
					if (h > gra_priv->height-overlay->p.y)
						h=gra_priv->height-overlay->p.y;
					for ( y = 0; y < h ;y++ )
					{
						for ( x = 0; x < w; x++ )
						{
							srcPixel = y*overlay->width+x;
							destPixel = ((overlay->p.y + y) * gra_priv->width) + (overlay->p.x + x);
							if ( overlay->pPixelData[srcPixel] == transparent_color )
							{
								destPixel = ((overlay->p.y + y) * gra_priv->width) + (overlay->p.x + x);

								gra_priv->pPixelData[destPixel] = RGB ( ((65535 - overlay->transparent_color.a) * GetRValue(gra_priv->pPixelData[destPixel]) + overlay->transparent_color.a * GetRValue(overlay->pPixelData[srcPixel])) / 65535,
																((65535 - overlay->transparent_color.a) * GetGValue(gra_priv->pPixelData[destPixel]) + overlay->transparent_color.a * GetGValue(overlay->pPixelData[srcPixel])) / 65535,
																((65535 - overlay->transparent_color.a) * GetBValue(gra_priv->pPixelData[destPixel]) + overlay->transparent_color.a * GetBValue(overlay->pPixelData[srcPixel])) / 65535);

							}
							else
							{
								gra_priv->pPixelData[destPixel] = overlay->pPixelData[srcPixel];
							}
						}

					}
#endif
				}
				overlay = overlay->next;
			}


#ifndef FAST_TRANSPARENCY
            hdc = BeginPaint(hwnd, &ps);
            BitBlt( hdc, 0, 0, gra_priv->width , gra_priv->height, gra_priv->hPrebuildDC, 0, 0, SRCCOPY );
#else
            HDC hdc = BeginPaint(hwnd, &ps);

            BitBlt( hdc, 0, 0, gra_priv->width , gra_priv->height, gra_priv->hMemDC, 0, 0, SRCCOPY );

            overlay = gra_priv->overlays;
            while ( !gra_priv->disabled && overlay && !overlay->disabled )
            {
                if ( overlay->p.x > 0 &&
                        overlay->p.y > 0 &&
                        overlay->p.x + overlay->width < gra_priv->width &&
                        overlay->p.y + overlay->height < gra_priv->height )
                {
                    BitBlt(hdc,overlay->p.x,overlay->p.y,overlay->width,overlay->height,overlay->hPrebuildDC,0,0,SRCAND);
                    BitBlt(hdc,overlay->p.x,overlay->p.y,overlay->width,overlay->height,overlay->hMemDC,0,0,SRCPAINT);
                }
                overlay = overlay->next;
            }
#endif
            EndPaint(hwnd, &ps);
            profile(0, "WM_PAINT\n");
        }
        break;
    case WM_MOUSEMOVE:
    {
        struct point pt = { LOWORD(lParam), HIWORD(lParam) };
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
    case WM_CHAR:
        HandleKeyChar( gra_priv, wParam);
        break;
    case WM_KEYDOWN:
        HandleKeyDown( gra_priv, wParam);
        break;
#ifdef HAVE_API_WIN32_CE
    case WM_SETFOCUS:
        if (fullscr) {
           HWND hwndSip = FindWindow(L"MS_SIPBUTTON", NULL);
           // deactivate the SIP button
           ShowWindow(hwndSip, SW_HIDE);
        }
        break;
   case WM_KILLFOCUS:
        if (fullscr != 1) {
           HWND hwndSip = FindWindow(L"MS_SIPBUTTON", NULL);
           // active the SIP button
           ShowWindow(hwndSip, SW_SHOW);
        }
        break;
#endif
    default:
        return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return 0;
}

static int fullscreen(struct window *win, int on)
{

#ifdef HAVE_API_WIN32_CE
    HWND hwndTaskbar = FindWindow(L"HHTaskBar", NULL);
    HWND hwndSip = FindWindow(L"MS_SIPBUTTON", NULL);
    fullscr = on;
	if (on) {
        ShowWindow(hwndTaskbar, SW_HIDE);
        MoveWindow(g_hwnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), FALSE);
	
	// deactivate the SIP button
	ShowWindow(hwndSip, SW_HIDE);
	
	} else {
        ShowWindow(hwndTaskbar, SW_SHOW);
        RECT taskbar_rect;
        GetWindowRect(  hwndTaskbar, &taskbar_rect);
        MoveWindow(g_hwnd, 0, taskbar_rect.bottom, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) - taskbar_rect.bottom, FALSE);
    
	// activate the SIP button
	ShowWindow(hwndSip, SW_SHOW);
    }

#else
	if (on) {
        ShowWindow(g_hwnd, SW_MAXIMIZE);
	} else {
        ShowWindow(g_hwnd, SW_RESTORE);
    }

#endif

    return 0;
}

extern void WINAPI SystemIdleTimerReset(void);
static struct event_timeout *
            event_win32_add_timeout(int timeout, int multi, struct callback *cb);

static void disable_suspend(struct window *win)
{
#ifdef HAVE_API_WIN32_CE
    struct window_priv *win_priv = win->priv;
    if ( win_priv && !win_priv->hBackLight )
    {
        win_priv->hBackLight = SetPowerRequirement(TEXT("BKL1:"), 0, 0x01, NULL, 0);
        event_win32_add_timeout(29000, 1, callback_new(SystemIdleTimerReset, 0, NULL));
    }

    SystemIdleTimerReset();
#endif
}

static const TCHAR g_szClassName[] = {'N','A','V','G','R','A','\0'};

static HANDLE CreateGraphicsWindows( struct graphics_priv* gr, HMENU hMenu )
{
    int wStyle = WS_VISIBLE;
    HWND hwnd;
#ifdef HAVE_API_WIN32_CE
    WNDCLASS wc;
#else
    WNDCLASSEX wc;
    wc.cbSize		 = sizeof(WNDCLASSEX);
    wc.hIconSm		 = NULL;
#endif

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


#ifdef HAVE_API_WIN32_CE
    gr->width = GetSystemMetrics(SM_CXSCREEN);
    gr->height = GetSystemMetrics(SM_CYSCREEN);

#if 0
    HWND hwndTaskbar = FindWindow(L"HHTaskBar", NULL);
    RECT taskbar_rect;
    GetWindowRect(  hwndTaskbar, &taskbar_rect);

    gr->height -= taskbar_rect.bottom;
#endif



#endif

#ifdef HAVE_API_WIN32_CE
    if (!RegisterClass(&wc))
#else
    if (!RegisterClassEx(&wc))
#endif
    {
        dbg(0, "Window registration failed\n");
        return NULL;
    }

    if ( hMenu )
    {
        wStyle = WS_CHILD;
	} else {
        wStyle = WS_OVERLAPPED|WS_VISIBLE;
    }

#ifdef HAVE_API_WIN32_CE
    g_hwnd = hwnd = CreateWindow(g_szClassName,
                                 TEXT("Navit"),
                                 wStyle,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 gr->wnd_parent_handle,
                                 hMenu,
                                 GetModuleHandle(NULL),
                                 NULL);
#else
    g_hwnd = hwnd = CreateWindow(g_szClassName,
                                 TEXT("Navit"),
                                 wStyle,
                                 0,
                                 0,
                                 gr->width,
                                 gr->height,
                                 gr->wnd_parent_handle,
                                 hMenu,
                                 GetModuleHandle(NULL),
                                 NULL);
#endif
    if (hwnd == NULL)
    {
        dbg(0, "Window creation failed: %d\n",  GetLastError());
        return NULL;
    }
    gr->wnd_handle = hwnd;

    callback_list_call_attr_2(gr->cbl, attr_resize, (void *)gr->width, (void *)gr->height);
    create_memory_dc(gr);

    SetWindowLongPtr( hwnd , DWLP_USER, (LONG_PTR)gr );

    ShowWindow( hwnd, SW_SHOW );
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
    DeleteObject( gc->hpen );
    DeleteObject( gc->hbrush );
    g_free( gc );
}

static void gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
    DeleteObject (gc->hpen);
    gc->line_width = w;
    gc->hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
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

    DeleteObject (gc->hpen);
    DeleteObject (gc->hbrush);
    gc->hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
    gc->hbrush = CreateSolidBrush( gc->fg_color );
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
    gc->hpen = CreatePen( PS_SOLID, gc->line_width, gc->fg_color );
    gc->hbrush = CreateSolidBrush( gc->fg_color );
    gc->gr = gr;
    return gc;
}


static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
    int i;

    HPEN hpenold = SelectObject( gr->hMemDC, gc->hpen );

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

    SelectObject( gr->hMemDC, hpenold);
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
    HPEN holdpen = SelectObject( gr->hMemDC, gc->hpen );
    HBRUSH holdbrush = SelectObject( gr->hMemDC, gc->hbrush );
    if (sizeof(POINT) != sizeof(struct point)) {
	    int i;
	    POINT* points=g_alloca(sizeof(POINT)*count);
	    for ( i=0;i< count; i++ )
	    {
		points[i].x = p[i].x;
		points[i].y = p[i].y;
	    }
            Polygon( gr->hMemDC, points,count );
    } else
	    Polygon( gr->hMemDC, (POINT *)p, count);
    SelectObject( gr->hMemDC, holdbrush);
    SelectObject( gr->hMemDC, holdpen);
}


static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
    HPEN holdpen = SelectObject( gr->hMemDC, gc->hpen );
    HBRUSH holdbrush = SelectObject( gr->hMemDC, gc->hbrush );

    Rectangle(gr->hMemDC, p->x, p->y, p->x+w, p->y+h);

    SelectObject( gr->hMemDC, holdbrush);
    SelectObject( gr->hMemDC, holdpen);
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
    HPEN holdpen = SelectObject( gr->hMemDC, gc->hpen );
    HBRUSH holdbrush = SelectObject( gr->hMemDC, GetStockObject(NULL_BRUSH));

    r=r/2;

    Ellipse( gr->hMemDC, p->x - r, p->y -r, p->x + r, p->y + r );

    SelectObject( gr->hMemDC, holdbrush);
    SelectObject( gr->hMemDC, holdpen);
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

        if ( p->x < 0 || p->y < 0 ||
                ( gr->parent && ((p->x + gr->width > gr->parent->width) || (p->y + gr->height > gr->parent->height) )))
        {
            gr->disabled = TRUE;
        }
        else
        {
            gr->disabled = FALSE;
        }
    }
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
    dbg( 1, "set draw_mode to %x, %d\n", gr, (int)mode );

    if ( mode == draw_mode_begin )
    {
        if ( gr->wnd_handle == NULL )
        {
            CreateGraphicsWindows( gr, (HMENU)ID_CHILD_GFX );
        }
        if ( gr->mode != draw_mode_begin )
        {
            if ( gr->hMemDC )
            {
                dbg(1, "Erase dc: %x, w: %d, h: %d, bg_color: %x\n", gr, gr->width, gr->height, gr->bg_color);
#ifdef  FAST_TRANSPARENCY
                if ( gr->hPrebuildDC )
                {
                    (void)SelectBitmap(gr->hPrebuildDC, gr->hOldPrebuildBitmap );
                    DeleteBitmap(gr->hPrebuildBitmap);
                    DeleteDC(gr->hPrebuildDC);
                    gr->hPrebuildDC = 0;
                }
#endif
            }
        }
#ifdef  FAST_TRANSPARENCY
        else if ( gr->hPrebuildDC )
        {
            (void)SelectBitmap(gr->hPrebuildDC, gr->hOldPrebuildBitmap );
            DeleteBitmap(gr->hPrebuildBitmap);
            DeleteDC(gr->hPrebuildDC);
            gr->hPrebuildDC = 0;
        }
#endif
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
        CreateGraphicsWindows( this_, (HMENU)ID_CHILD_GFX );
        return NULL;
    }
    if (!strcmp(type, "window"))
    {
        CreateGraphicsWindows( this_ , NULL);

        this_->window.fullscreen = fullscreen;
        this_->window.disable_suspend = disable_suspend;

        this_->window.priv=g_new0(struct window_priv, 1);

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
    int prevBkMode;
    HFONT hOldFont;
    double angle;

    GetClientRect( gr->wnd_handle, &rcClient );

    SetTextColor(gr->hMemDC, fg->fg_color);
    prevBkMode = SetBkMode( gr->hMemDC, TRANSPARENT );

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


    angle = -atan2( dy, dx ) * 180 / 3.14159 ;
    if (angle < 0)
        angle += 360;

    SetTextAlign (gr->hMemDC, TA_BASELINE) ;
    SetViewportOrgEx (gr->hMemDC, p->x, p->y, NULL) ;
    font->lf.lfEscapement = font->lf.lfOrientation = ( angle * 10 ) ;
    DeleteObject (font->hfont) ;

    font->hfont = CreateFontIndirect (&font->lf);
    hOldFont = SelectObject(gr->hMemDC, font->hfont );

    {
        wchar_t utf16[1024];
        const UTF8 *utf8 = (UTF8 *)text;
        UTF16 *utf16p = (UTF16 *) utf16;
        SetBkMode (gr->hMemDC, TRANSPARENT);
        if (ConvertUTF8toUTF16(&utf8, utf8+strlen(text),
                               &utf16p, utf16p+sizeof(utf16),
                               lenientConversion) == conversionOK)
        {
            ExtTextOutW(gr->hMemDC, 0, 0, 0, NULL,
                        utf16, (wchar_t*) utf16p - utf16, NULL);
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
pngdecode(struct graphics_priv *gr, char *name, struct graphics_image_priv *img)
{
    png_struct    *png_ptr = NULL;
    png_info      *info_ptr = NULL;
    png_byte      buf[8];
    png_byte      **row_pointers = NULL;

    int           bit_depth;
    int           color_type;
    int           alpha_present;
    int           ret;
    int           i;
    FILE *png_file;

    dbg(1,"enter %s\n",name);
    png_file=fopen(name, "rb");
    if (!png_file)
    {
        dbg(0,"failed to open %s\n",name);
        return FALSE;
    }


    /* read and check signature in PNG file */
    ret = fread (buf, 1, 8, png_file);
    if (ret != 8)
    {
        fclose(png_file);
        return FALSE;
    }

    ret = png_check_sig (buf, 8);
    if (!ret)
    {
        fclose(png_file);
        return FALSE;
    }

    /* create png and info structures */

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
                                      NULL, NULL, NULL);
    if (!png_ptr)
    {
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
                  (png_uint_32*)&img->width, (png_uint_32*)&img->height, &bit_depth, &color_type,
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

   png_set_bgr(png_ptr);

    /* all transformations have been registered; now update info_ptr data,
     * get rowbytes and channels, and allocate image memory */

    png_read_update_info (png_ptr, info_ptr);

    /* get the new color-type and bit-depth (after expansion/stripping) */
    png_get_IHDR (png_ptr, info_ptr, (png_uint_32*)&img->width, (png_uint_32*)&img->height, &bit_depth, &color_type,
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

    if (  gr->AlphaBlend && img->channels == 4 )
    {
        BITMAPINFO pnginfo;
	HDC dc;
        memset(&pnginfo, 0, sizeof(pnginfo));
        pnginfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pnginfo.bmiHeader.biWidth = img->width;
        pnginfo.bmiHeader.biHeight = -img->height;
        pnginfo.bmiHeader.biBitCount = 32;
        pnginfo.bmiHeader.biCompression = BI_RGB;
        pnginfo.bmiHeader.biPlanes = 1;
        dc = CreateCompatibleDC(NULL);
        img->hBitmap = CreateDIBSection(dc, &pnginfo, DIB_RGB_COLORS , (void **)&img->png_pixels, NULL, 0);
        DeleteDC(dc);

    }
    else
    {
        if ((img->png_pixels = (png_byte *) g_malloc (img->row_bytes * img->height * sizeof (png_byte))) == NULL)
        {
            fclose(png_file);
            png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
            return FALSE;
        }
    }

    if ((row_pointers = (png_byte **) g_malloc (img->height * sizeof (png_bytep))) == NULL)
    {
        fclose(png_file);
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        g_free (img->png_pixels);
        img->png_pixels = NULL;
        img->hBitmap = NULL;
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

    if (row_pointers != (unsigned char**) NULL)
        g_free (row_pointers);
    img->hot.x=img->width/2-1;
    img->hot.y=img->height/2-1;
    dbg(1,"ok\n");
    fclose(png_file);
    return TRUE;

} /* end of source */

static void
pngrender(struct graphics_image_priv *img, struct graphics_priv *gr, int x0, int y0)
{
	if (img->hBitmap)
	{
		HDC hdc;
		HBITMAP oldBitmap;
		BLENDFUNCTION blendFunction ;
		blendFunction.BlendOp = AC_SRC_OVER;
		blendFunction.BlendFlags = 0;
		blendFunction.SourceConstantAlpha = 255;
		blendFunction.AlphaFormat = AC_SRC_ALPHA;
		hdc = CreateCompatibleDC(NULL);
		oldBitmap = SelectBitmap(hdc, img->hBitmap);
		gr->AlphaBlend(gr->hMemDC, x0, y0, img->width, img->height, hdc, 0, 0, img->width, img->height, blendFunction);
		(void)SelectBitmap(hdc, oldBitmap);
		DeleteDC(hdc);
	}
	else
	{
		int x, y;
		HDC hdc=gr->hMemDC;
		png_byte *pix_ptr = img->png_pixels;
		COLORREF *pixeldata;
		HDC dc;
		HBITMAP bitmap;
		HBITMAP oldBitmap;

		BITMAPINFO pnginfo;

		memset(&pnginfo, 0, sizeof(pnginfo));
		pnginfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pnginfo.bmiHeader.biWidth = img->width;
		pnginfo.bmiHeader.biHeight = -img->height;
		pnginfo.bmiHeader.biBitCount = 32;
		pnginfo.bmiHeader.biCompression = BI_RGB;
		pnginfo.bmiHeader.biPlanes = 1;
		dc = CreateCompatibleDC(NULL);
		bitmap = CreateDIBSection(hdc, &pnginfo, DIB_RGB_COLORS, (void **)&pixeldata, NULL, 0);
		oldBitmap = SelectBitmap(dc, bitmap);
		BitBlt(dc, 0, 0, img->width, img->height, hdc, x0, y0, SRCCOPY);
		for (y=0 ; y < img->width ; y++)
		{
			for (x=0 ; x < img->height ; x++)
			{
				int b = pix_ptr[0];
				int g = pix_ptr[1];
				int r = pix_ptr[2];
				int a = pix_ptr[3];
				if (a != 0xff)
				{
					int p = *pixeldata;
					int ai = 0xff - a;
					r = (r * a + ((p >> 16) & 0xff) * ai) / 255;
					g = (g * a + ((p >>  8) & 0xff) * ai) / 255;
					b = (b * a + ((p >>  0) & 0xff) * ai) / 255;
				}
				*pixeldata++ = (r << 16) | (g << 8) | b;
				pix_ptr+=img->channels;
			}
		}

		BitBlt(hdc, x0, y0, img->width, img->height, dc, 0, 0, SRCCOPY);
		(void)SelectBitmap(dc, oldBitmap);
		DeleteBitmap(bitmap);
		DeleteDC(dc);
	}
}

static int
xpmdecode(char *name, struct graphics_image_priv *img)
{
    img->pxpm = Xpm2bmp_new();
    if (Xpm2bmp_load( img->pxpm, name ) != 0)
    {
        g_free(img->pxpm);
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
			rc=pngdecode(gr, name, ret);
        }
		if (rc) {
            image_cache_hash_add( name, ret );
		} else {
            g_free(ret);
            ret=NULL;
        }
    }
	if (ret) {
        *w=ret->width;
        *h=ret->height;
        if (hot)
            *hot=ret->hot;
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
    dbg(1, "resize overlay: %x, x: %d, y: %d, w: %d, h: %d, alpha: %x, wraparound: %d\n", gr, p->x, p->y, w, h, alpha, wraparound);

    if ( gr->width != w || gr->height != h )
    {
        gr->width  = w;
        gr->height = h;
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
    this->AlphaBlend = gr->AlphaBlend;

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
    int len = g_utf8_strlen(text, -1);
    int xMin = 0;
    int yMin = 0;
    int yMax = 13*font->size/256;
    int xMax = 9*font->size*len/256;

    dbg(2, "Get bbox for %s\n", text);

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


static void set_alphablend(struct graphics_priv* gra_priv)
{
#if HAVE_API_WIN32_CE
    gra_priv->hCoreDll = LoadLibrary(TEXT("coredll.dll"));
#else
    gra_priv->hCoreDll = LoadLibrary(TEXT("msimg32.dll"));
#endif
    if ( gra_priv->hCoreDll )
    {
        gra_priv->AlphaBlend  = (FP_AlphaBlend)GetProcAddress(gra_priv->hCoreDll, TEXT("AlphaBlend") );
        if (!gra_priv->AlphaBlend)
        {
            FreeLibrary(gra_priv->hCoreDll);
            gra_priv->hCoreDll = NULL;
            dbg(1, "AlphaBlend not supported\n");
        }
    }
    else
    {
        dbg(0, "Error loading coredll\n");
    }
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
    this_->window.priv = NULL;
    set_alphablend(this_);
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
#ifdef HAVE_API_WIN32_CE
    HWND hwndTaskbar = FindWindow(L"HHTaskBar", NULL);
    HWND hwndSip = FindWindow(L"MS_SIPBUTTON", NULL);

    // activate the SIP button
    ShowWindow(hwndSip, SW_SHOW);
    ShowWindow(hwndTaskbar, SW_SHOW);
#endif

    DestroyWindow(g_hwnd);
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
            struct callback *cb = t->cb;
            dbg(2, "Timer %d (multi: %d)\n", t->timer_id, t->multi);
            if (!t->multi)
            {
                KillTimer(NULL, t->timer_id);
                timers = g_list_remove(timers, t);
                g_free(t);
            }
            callback_call_0(cb);
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
    t = g_new0(struct event_timeout, 1);
    if (!t)
        return t;
    t->multi = multi;
    timers = g_list_prepend(timers, t);
    t->cb = cb;
    t->timer_id = SetTimer(NULL, 0, timeout, win32_timer_cb);
    dbg(1, "Started timer %d for %d (multi: %d)\n", t->timer_id, timeout, multi);
    return t;
}

static void
event_win32_remove_timeout(struct event_timeout *to)
{
    if (to)
    {
        GList *l;
        struct event_timeout *t=NULL;
        dbg(1, "Try stopping timer %d\n", to->timer_id);
        l = timers;
        while (l)
        {
            t = l->data;
            /* Use the pointer not the ID, IDs are reused */
            if (t == to)
            {
                KillTimer(NULL, t->timer_id);
                timers = g_list_remove(timers, t);
                g_free(t);
                return;
            }
            l = g_list_next(l);
        }
        dbg(0, "Timer %d not found\n", to->timer_id);
        g_free(to);
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
