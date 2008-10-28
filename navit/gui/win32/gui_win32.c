#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <glib.h>
#include <libintl.h>
#include "config.h"
#include "plugin.h"
#include "gui.h"
#include "win32_gui.h"
#include "point.h"
#include "menu.h"
#include "item.h"
#include "attr.h"
#include "callback.h"
#include <commctrl.h>
#include "debug.h"
#ifdef __CEGCC__
#include <sipapi.h>
#include <aygshell.h>
#include "ceglue.h"

static int ce_backlight = 1;
static int ce_fullscreen;
#endif

#ifdef HAVE_GLIB
//static GHashTable *popup_callback_hash = NULL;
static GArray *popup_menu_array;
#endif

const TCHAR g_szClassName[] = TEXT("navit_gui_class");


static menu_id = 0;
static POINT menu_pt;


gboolean message_pump( gpointer data )
{
    MSG messages;

	Sleep( 1 );

    if (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    else{
    	exit( 0 );
    }
	return TRUE;
}



//extern struct graphics_priv *g_gra;

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
    LPRECT rcParent;
    int idChild;

    idChild = GetWindowLong(hwndChild, GWL_ID);

	if ( idChild == ID_CHILD_GFX )
	{
		rcParent = (LPRECT) lParam;

		MoveWindow( hwndChild,  0, 0, rcParent->right, rcParent->bottom, TRUE );
		PostMessage( hwndChild, WM_USER+1, 0, 0 );
	}

    return TRUE;
}

#ifndef GET_WHEEL_DELTA_WPARAM
	#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

static void CreateToolBar(HWND hwnd)
{
	// Create Toolbar
	HWND hTool;
	TBBUTTON tbb[8];
	TBADDBITMAP tbab;
#ifdef _WIN32_WCE
	/* Have to initialize common controls under CE */
	INITCOMMONCONTROLSEX iccex;
  	iccex.dwSize = sizeof (INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx (&iccex);
#endif

	hTool = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		hwnd, (HMENU)ID_CHILD_TOOLBAR, GetModuleHandle(NULL), NULL);

	if(hTool == NULL)
		MessageBox(hwnd, TEXT("Could not create tool bar."), TEXT("Error"), MB_OK | MB_ICONERROR);

	SendMessage(hTool, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	tbab.hInst = GetModuleHandle(NULL);
	tbab.nID = IDB_NAVITTOOLBAR;
	int iImageOffset = SendMessage(hTool, TB_ADDBITMAP, 10, (LPARAM) &tbab);

	int iStr;

	ZeroMemory(tbb, sizeof(tbb));

	tbb[0].iBitmap = iImageOffset;
	tbb[0].fsState = TBSTATE_ENABLED;
	tbb[0].fsStyle = TBSTYLE_BUTTON;
	tbb[0].idCommand = ID_DISPLAY_ZOOMIN;
	iStr = SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"ZoomIn" );
	tbb[0].iString = iStr;

	tbb[1].iBitmap = iImageOffset+1;
	tbb[1].fsState = TBSTATE_ENABLED;
	tbb[1].fsStyle = TBSTYLE_BUTTON;
	tbb[1].idCommand = ID_DISPLAY_ZOOMOUT;
	iStr = SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"ZoomOut" );
	tbb[1].iString = iStr;

	tbb[2].iBitmap = iImageOffset+4;
	tbb[2].fsState = TBSTATE_ENABLED;
	tbb[2].fsStyle = TBSTYLE_BUTTON;
	tbb[2].idCommand = ID_DISPLAY_REFRESH;
	iStr = SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"Refresh" );
	tbb[2].iString = iStr;

	tbb[3].iBitmap = iImageOffset+2;
	tbb[3].fsState = TBSTATE_ENABLED;
	tbb[3].fsStyle = TBSTYLE_BUTTON;
	tbb[3].idCommand = ID_DISPLAY_ZOOMIN;
	iStr = SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"Cursor" );
	tbb[3].iString = iStr;

	tbb[4].iBitmap = iImageOffset+5;
	tbb[4].fsState = TBSTATE_ENABLED;
	tbb[4].fsStyle = TBSTYLE_BUTTON;
	tbb[4].idCommand = ID_DISPLAY_ORIENT;
	iStr = SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"Orientation" );
	tbb[4].iString = iStr;

	tbb[5].iBitmap = iImageOffset+8;
	tbb[5].fsState = TBSTATE_ENABLED;
	tbb[5].fsStyle = TBSTYLE_BUTTON;
	tbb[5].idCommand = ID_DISPLAY_ZOOMIN;
	iStr= SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"Destination" );
	tbb[5].iString = iStr;

	tbb[6].iBitmap = iImageOffset+3;
	tbb[6].fsState = TBSTATE_ENABLED;
	tbb[6].fsStyle = TBSTYLE_BUTTON;
	tbb[6].idCommand = ID_DISPLAY_ZOOMIN;
	iStr= SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"Roadbook" );
	tbb[6].iString = iStr;

	tbb[7].iBitmap = iImageOffset+9;
	tbb[7].fsState = TBSTATE_ENABLED;
	tbb[7].fsStyle = TBSTYLE_BUTTON;
	tbb[7].idCommand = ID_FILE_EXIT;
	iStr= SendMessage(hTool, TB_ADDSTRINGW, 0, (LPARAM)  L"_Quit" );
	tbb[7].iString = iStr;

	SendMessage(hTool, TB_ADDBUTTONS, sizeof(tbb)/sizeof(TBBUTTON), (LPARAM)&tbb);
}

static void window_layout( HWND hwnd )
{
#ifndef __CEGCC__
	RECT rcClient;
	RECT rcTool;
	int iToolHeight;

	HWND hChild = GetDlgItem(hwnd, ID_CHILD_TOOLBAR);
	SendMessage(hChild, TB_AUTOSIZE, 0, 0);

	GetWindowRect(hChild, &rcTool);
	iToolHeight = rcTool.bottom - rcTool.top;

	GetClientRect(hwnd, &rcClient);
	//printf( "BEFORE resize gui to: %d %d %d %d \n", rcClient.left, rcClient.right, rcClient.top, rcClient.bottom );

	rcClient.top += iToolHeight;

	printf( "resize gui to: %d %d %d %d \n", rcClient.left, rcClient.right, rcClient.top, rcClient.bottom );


	hChild = GetDlgItem(hwnd, ID_CHILD_GFX);
	if ( hChild )
	{
		MoveWindow( hChild,  rcClient.left, rcClient.top, rcClient.right- rcClient.left, rcClient.bottom - rcClient.top, TRUE );
		PostMessage( hChild, WM_USER+1, 0, 0 );
	}
#endif
}
#ifdef __CEGCC__
static void toggle_fullscreen(HWND mWnd)
{
	if (SHFullScreenPtr) {
		if (!ce_fullscreen) {
			(*SHFullScreenPtr)(mWnd, SHFS_HIDETASKBAR |
			SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);
		} else {
			(*SHFullScreenPtr)(mWnd, SHFS_HIDESIPBUTTON);
		}
		ce_fullscreen = !ce_fullscreen;
	}
}

static void toggle_backlight(void)
{
	if (ce_backlight)
		if (CeEnableBacklight(FALSE))
			ce_backlight = 0;
	else
		if (CeEnableBacklight(TRUE))
			ce_backlight = 1;
}
#endif

static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    RECT rcClient;

//	printf( "PARENT %d %d %d \n", Message, wParam, lParam );

	switch(Message)
	{
		case WM_CREATE:
		{
			HMENU hMenu, hSubMenu;

			CreateToolBar( hwnd );

			hMenu = CreateMenu();
			// g_this_->hwnd = hwnd;

			hSubMenu = CreatePopupMenu();

			AppendMenuW(hSubMenu, MF_STRING, ID_DISPLAY_ZOOMIN, L"ZoomIn" );
			AppendMenuW(hSubMenu, MF_STRING, ID_DISPLAY_ZOOMOUT, L"ZoomOut" );
			AppendMenuW(hSubMenu, MF_STRING, ID_DISPLAY_REFRESH, L"Refresh" );
			AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL );
			AppendMenuW(hSubMenu, MF_STRING, ID_FILE_EXIT, L"_Quit" );

			AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, L"Display" );
			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, TEXT("&Go"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, TEXT("&Stuff"));

#ifndef __CEGCC__
			SetMenu(hwnd, hMenu);
#endif

			window_layout( hwnd );

		}
		break;
		case WM_COMMAND:
		{
			printf( "WM_COMMAND %d\n", LOWORD(wParam) );
			struct gui_priv* gui = (struct gui_priv*)GetWindowLongPtr( hwnd , DWLP_USER );


			switch(LOWORD(wParam))
			{
				case ID_DISPLAY_ZOOMIN:
					navit_zoom_in(gui->nav, 2, NULL);
					return 0;
				break;
				case ID_DISPLAY_ZOOMOUT:
					navit_zoom_out(gui->nav, 2, NULL);
					return 0;
				break;
				case ID_DISPLAY_REFRESH:
					navit_draw(gui->nav);
					return 0;
				break;
				case ID_DISPLAY_CURSOR:
				{
					struct attr attr;
					attr.type=attr_cursor;
					// TODO attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
					if(!navit_set_attr(gui->nav, &attr)) {
						dbg(0, "Failed to set attr_cursor\n");
					}
					return 0;
				}
				break;
				case ID_DISPLAY_ORIENT:
				{
					struct attr attr;

					attr.type=attr_orientation;
					// attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
					attr.u.num = 0; // TODO
					if(!navit_set_attr(gui->nav, &attr)) {
						dbg(0, "Failed to set attr_orientation\n");
					}
					return 0;
				}

				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					return 0;
				break;
			}
#if HAVE_GLIB
			if ( popup_menu_array )
			{
				struct menu_priv* priv = (struct menu_priv*)g_array_index( popup_menu_array, gint, LOWORD(wParam) - POPUP_MENU_OFFSET );

				if ( priv )
				{
					struct callback* cb = priv->cb;
					if ( priv->cb )
					{
						callback_call_0( priv->cb );
						return 0;
					}
				}
			}
#endif
		}
		break;
		case WM_USER+ 1:
            GetClientRect(hwnd, &rcClient);
			printf( "resize gui to: %d %d \n", rcClient.right, rcClient.bottom );

			window_layout( hwnd );
            //EnumChildWindows(hwnd, EnumChildProc, (LPARAM) &rcClient);
            return 0;
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_SIZE:
			window_layout( hwnd );
            return 0;
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;


		case WM_MOUSEWHEEL:
		{
			struct gui_priv* gui = (struct gui_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

			short delta = GET_WHEEL_DELTA_WPARAM( wParam );
			if ( delta > 0 )
			{
				navit_zoom_in(gui->nav, 2, NULL);
			}
			else{
				navit_zoom_out(gui->nav, 2, NULL);
			}
		}
		break;
#ifdef __CEGCC__
		case WM_KEYDOWN:
			{
			struct point p;
			int w,h;
			struct gui_priv* gui = (struct gui_priv*)GetWindowLongPtr( hwnd , DWLP_USER );
			transform_get_size(navit_get_trans(gui->nav), &w, &h);
			
			if (wParam == VK_LEFT || wParam == '4') {
				p.x=0;
				p.y=h/2;
				navit_set_center_screen(gui->nav, &p);
			} else if (wParam == VK_RIGHT || wParam == '6') {
				p.x=w;
				p.y=h/2;
				navit_set_center_screen(gui->nav, &p);
			} else if (wParam == VK_UP || wParam == '2') {
				p.x=w/2;
				p.y=0;
				navit_set_center_screen(gui->nav, &p);
			} else if (wParam == VK_DOWN || wParam == '8') {
				p.x=w/2;
				p.y=h;
				navit_set_center_screen(gui->nav, &p);
			} else if (wParam == '1') {
				navit_zoom_in(gui->nav, 2, NULL);
			} else if (wParam == '3') {
				navit_zoom_out(gui->nav, 2, NULL);
			} else if (wParam == '7') {
				toggle_backlight();
			} else if (wParam == '9') {
				toggle_fullscreen(hwnd);
			}
			}
		break;
#endif
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

HANDLE CreateWin32Window( void )
{
#ifdef __CEGCC__
	WNDCLASS wc;
#else
	WNDCLASSEX wc;
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.hIconSm		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NAVIT));
#endif
	HWND hwnd;
	MSG Msg;

	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 32;
	wc.hInstance	 = NULL;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NAVIT));

#ifdef __CEGCC__
	if(!RegisterClass(&wc))
#else
	if(!RegisterClassEx(&wc))
#endif

	{
		MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		TEXT( "Navit" ),
#if defined(__CEGCC__)
		WS_SYSMENU | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
#else

		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
#endif
		NULL, NULL, NULL, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, TRUE);
	UpdateWindow(hwnd);

#if 0
	g_idle_add (message_pump, NULL);
#endif

	return hwnd;
}


static int win32_gui_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	HANDLE* wndHandle_ptr = graphics_get_data(gra, "wnd_parent_handle_ptr");
	*wndHandle_ptr = this_->hwnd;
	graphics_get_data(gra, "START_CLIENT");
	return 0;
}


static void win32_gui_add_bookmark_do(struct gui_priv *gui)
{
//	navit_add_bookmark(gui->nav, &gui->dialog_coord, gtk_entry_get_text(GTK_ENTRY(gui->dialog_entry)));
//	gtk_widget_destroy(gui->dialog_win);
}

static int win32_gui_add_bookmark(struct gui_priv *gui, struct pcoord *c, char *description)
{
 	return 1;
}


static struct menu_methods menu_methods;


static struct menu_priv *add_menu(	struct menu_priv *menu,
									struct menu_methods *meth,
									char *name,
									enum menu_type type,
									struct callback *cb)
{
	struct menu_priv* ret = NULL;

	ret = g_new0(struct menu_priv, 1);

	*ret = *menu;
	*meth = menu_methods;

	TCHAR *menuname = newSysString(name);

	if ( type == menu_type_submenu )
	{
		HMENU hSubMenu = NULL;
		hSubMenu = CreatePopupMenu();
		AppendMenu(menu->hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, menuname );
		ret->hMenu = hSubMenu;
	}
	else
	{
		AppendMenu( menu->hMenu, MF_STRING, menu_id, name );
	}

	// g_hash_table_insert( popup_callback_hash, GINT_TO_POINTER( menu_id ),  (gpointer)cb );
#if HAVE_GLIB
	g_array_append_val( popup_menu_array, ret );
#endif

	ret->cb = cb;

	menu_id++;

	return ret;

}

static void set_toggle(struct menu_priv *menu, int active)
{
	// gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(menu->action), active);
}

static int get_toggle(struct menu_priv *menu)
{
	// return gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(menu->action));
	return 0;
}

static struct menu_methods menu_methods = {
	add_menu,
	set_toggle,
	get_toggle,
};

static void popup_activate(struct menu_priv *menu)
{
	POINT pnt;
	GetCursorPos( &pnt );

	if (menu->hMenu)
	{
		TrackPopupMenu( menu->hMenu, 0, pnt.x, pnt.y, 0, menu->wnd_handle, NULL );
		DestroyMenu( menu->hMenu );
	}
}


static void popup_deactivate( struct menu_priv *menu )
{
	DestroyMenu( menu->hMenu );
}

struct menu_priv* win32_gui_popup_new(struct gui_priv *this_, struct menu_methods *meth)
{
	struct menu_priv* ret = NULL;

	ret = g_new0(struct menu_priv, 1);
	*meth = menu_methods;

	menu_id = POPUP_MENU_OFFSET;

#if HAVE_GLIB
	if ( popup_menu_array )
	{
		g_array_free (popup_menu_array, TRUE);
		popup_menu_array = NULL;
	}

	popup_menu_array = g_array_new (FALSE, FALSE, sizeof (gint));
#endif

	ret->cb = NULL;
	ret->hMenu = CreatePopupMenu();
	ret->wnd_handle = this_->hwnd;
	meth->popup=popup_activate;

printf( "create popup menu %d \n", ret->hMenu );

	return ret;
}

struct gui_methods win32_gui_methods = {
	NULL, // win32_gui_menubar_new,
	win32_gui_popup_new,
	win32_gui_set_graphics,
	NULL,
	NULL, // win32_gui_datawindow_new,
	win32_gui_add_bookmark,
};



static struct gui_priv *win32_gui_new( struct navit *nav, struct gui_methods *meth, struct attr **attrs)
{
	struct gui_priv *this_;
#ifdef __CEGCC__
	/* Do not run multiple instances under CE */
	HWND prev;
	prev = FindWindow(g_szClassName, NULL);
	if (prev) {
		ShowWindow(prev, SW_RESTORE);
		SetForegroundWindow(prev);
		InvalidateRect (prev, NULL, FALSE);
		exit(0);
	}
	InitCeGlue();
#endif

	*meth=win32_gui_methods;

	this_=g_new0(struct gui_priv, 1);
	this_->nav=nav;

	this_->hwnd = CreateWin32Window();
	SetWindowLongPtr( this_->hwnd , DWLP_USER, this_ );

	return this_;
}

void plugin_init(void)
{
	plugin_register_gui_type("win32", win32_gui_new);
}
