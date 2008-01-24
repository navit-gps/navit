#include <windows.h>
#include <wingdi.h>
#include <glib.h>

#include "config.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"




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

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
    sprintf((LPTSTR)lpDisplayBuf, TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);

    printf( "%s\n", lpDisplayBuf );
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}


struct graphics_priv {
	struct point p;
	int width;
	int height;
	int library_init;
	int visible;
	HANDLE wnd_parent_handle;
	HANDLE wnd_handle;

	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;
	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;
	enum draw_mode_num mode;
};


struct graphics_priv *g_gra;


static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_CREATE:
		{
				g_gra->wnd_handle = hwnd;
				//if (g_gra->resize_callback)
				// 	(*g_gra->resize_callback)(g_gra->resize_callback_data, g_gra->width, g_gra->height);
				//MoveWindow( hwnd, 0,0, 780, 680, TRUE );
		}
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			}
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_LBUTTONDBLCLK:
				if (g_gra->resize_callback)
				 	(*g_gra->resize_callback)(g_gra->resize_callback_data, g_gra->width, g_gra->height);
		break;
		case WM_SIZE:
			{
				//graphics = GetWindowLong( hwnd, DWL_USER, 0 );

				g_gra->width = LOWORD( lParam );
				g_gra->height  = HIWORD( lParam );

			}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
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
	GetWindowRect( gr->wnd_parent_handle,&rcParent);

	if(!RegisterClassEx(&wc))
	{
		ErrorExit( "Window Registration Failed!" );
		return 0;
	}

				g_gra->width = rcParent.right - rcParent.left;
				g_gra->height  = rcParent.bottom - rcParent.top;


	hwnd = CreateWindow( 	g_szClassName,
							"",
							WS_CHILD  ,
							0,
							0,
							g_gra->width,
							g_gra->height,
							gr->wnd_parent_handle,
							NULL,
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



static void
graphics_destroy(struct graphics_priv *gr)
{
}


static void
gc_destroy(struct graphics_gc_priv *gc)
{

}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
//	gdk_gc_set_line_attributes(gc->gc, w, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, unsigned char *dash_list, int n)
{
//	gdk_gc_set_dashes(gc->gc, 0, (gint8 *)dash_list, n);
//	gdk_gc_set_line_attributes(gc->gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_color(struct graphics_gc_priv *gc, struct color *c, int fg)
{
//	GdkColor gdkc;
//	gdkc.pixel=0;
//	gdkc.red=c->r;
//	gdkc.green=c->g;
//	gdkc.blue=c->b;
//	gdk_colormap_alloc_color(gc->gr->colormap, &gdkc, FALSE, TRUE);
//	if (fg)
//		gdk_gc_set_foreground(gc->gc, &gdkc);
//	else
//		gdk_gc_set_background(gc->gc, &gdkc);
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
//	gc_set_color(gc, c, 1);
}

static void gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
// 	gc_set_color(gc, c, 0);
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
//	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);

	*meth=gc_methods;
//	gc->gc=gdk_gc_new(gr->widget->window);
//	gc->gr=gr;
	return NULL;
}


static void draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{

	int i;
	HANDLE hndl = gr->wnd_handle;

	HDC dc = GetDC( gr->wnd_handle );

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
	ReleaseDC( gr->wnd_handle, dc );
}

static void draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
//	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
//		gdk_draw_polygon(gr->drawable, gc->gc, TRUE, (GdkPoint *)p, count);
//	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
//		gdk_draw_polygon(gr->widget->window, gc->gc, TRUE, (GdkPoint *)p, count);
}

static void draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
//	gdk_draw_rectangle(gr->drawable, gc->gc, TRUE, p->x, p->y, w, h);
}

static void draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
//	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
//		gdk_draw_arc(gr->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
//	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
//		gdk_draw_arc(gr->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
}



static void draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	printf( "restore\n");
}

static void draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	if ( mode == draw_mode_begin )
	{
		if ( gr->wnd_handle == NULL )
			CreateGraphicsWindows( gr );

		HDC hdcScreen = GetDC( gr->wnd_handle );
/*
		hbmScreen = CreateCompatibleBitmap(hdcScreen,
                     GetDeviceCaps(hdcScreen, HORZRES),
                     GetDeviceCaps(hdcScreen, VERTRES));
                     */

	}

	// force paint
	if (mode == draw_mode_end && gr->mode == draw_mode_begin)
	{
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
// 	gr->background_gc=gc;
}

static void draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
}

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size)
{
	meth = NULL;
	return NULL;
}

static struct graphics_image_priv *image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot)
{
	return NULL;
}


static struct graphics_methods graphics_methods = {
 	graphics_destroy,
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	draw_circle,
	draw_text,
	NULL, // draw_image,
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
	return this_;
}

struct graphics_priv* win32_graphics_new( struct graphics_methods *meth, struct attr **attrs)
{
	struct graphics_priv* this_=graphics_win32_drawing_area_new_helper(meth);
	return this_;
}
