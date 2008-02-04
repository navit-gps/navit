#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <glib.h>
#include "config.h"
#include "plugin.h"
#include "gui.h"
#include "win32_gui.h"
#include "point.h"
#include "menu.h"


const char g_szClassName[] = "navit_gui_class";


static menu_id = 0;
static POINT menu_pt;

gboolean message_pump( gpointer data )
{
    MSG messages;

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






extern struct graphics_priv *g_gra;

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
    LPRECT rcParent;
    int idChild;

    idChild = GetWindowLong(hwndChild, GWL_ID);

	if ( idChild == ID_CHILD_GFX )
	{
		rcParent = (LPRECT) lParam;

		MoveWindow( hwndChild,  0, 0, rcParent->right, rcParent->bottom, TRUE );
		(*g_gra->resize_callback)(g_gra->resize_callback_data, rcParent->right, rcParent->bottom);
	}

    return TRUE;
}

#ifndef GET_WHEEL_DELTA_WPARAM
	#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    RECT rcClient;

	//printf( "PARENT %d %d %d \n", Message, wParam, lParam );

	switch(Message)
	{
		case WM_CREATE:
		{
			HMENU hMenu, hSubMenu;
			HICON hIcon, hIconSm;

			hMenu = CreateMenu();
			// g_this_->hwnd = hwnd;

			hSubMenu = CreatePopupMenu();

			gunichar2* utf16 = NULL;

			utf16 = g_utf8_to_utf16( _( "_Quit" ), -1, NULL, NULL, NULL );
			AppendMenuW(hSubMenu, MF_STRING, ID_FILE_EXIT, utf16 );
			g_free( utf16 );

			utf16 = g_utf8_to_utf16( _( "Zoom in" ), -1, NULL, NULL, NULL );
			AppendMenuW(hSubMenu, MF_STRING, ID_DISPLAY_ZOOMIN, utf16 );
			g_free( utf16 );

			utf16 = g_utf8_to_utf16( _( "Zoom out" ), -1, NULL, NULL, NULL );
			AppendMenuW(hSubMenu, MF_STRING, ID_DISPLAY_ZOOMOUT, utf16 );
			g_free( utf16 );

			utf16 = g_utf8_to_utf16( _( "Display" ), -1, NULL, NULL, NULL );
			AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, utf16 );
			g_free( utf16 );

			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, "&Go");
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Stuff");

			SetMenu(hwnd, hMenu);

#if 0
			hIcon = LoadImage(NULL, "menu_two.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
			if(hIcon)
				SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			else
				MessageBox(hwnd, "Could not load large icon!", "Error", MB_OK | MB_ICONERROR);

			hIconSm = LoadImage(NULL, "menu_two.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
			if(hIconSm)
				SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
			else
				MessageBox(hwnd, "Could not load small icon!", "Error", MB_OK | MB_ICONERROR);

#endif

		}
		break;
		case WM_COMMAND:
		{
			struct gui_priv* gui = (struct gui_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

			switch(LOWORD(wParam))
			{
				case ID_DISPLAY_ZOOMIN:
						navit_zoom_in(gui->nav, 2, NULL);
				break;
				case ID_DISPLAY_ZOOMOUT:
						navit_zoom_out(gui->nav, 2, NULL);
				break;
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_STUFF_GO:
								 	(*g_gra->resize_callback)(g_gra->resize_callback_data, g_gra->width, g_gra->height);

//					navit_draw(gui->nav);
					// MessageBox(hwnd, "You clicked Go!", "Woo!", MB_OK);
				break;
			}
		}
		break;
		case WM_USER+ 1:
			printf( "wm_user \n" );
			(*g_gra->resize_callback)( g_gra->resize_callback_data, g_gra->width, g_gra->height );
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_SIZE:
            GetClientRect(hwnd, &rcClient);
			printf( "resize gui to: %d %d \n", rcClient.right, rcClient.bottom );

            EnumChildWindows(hwnd, EnumChildProc, (LPARAM) &rcClient);
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

		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

HANDLE CreateWin32Window( void )
{
	WNDCLASSEX wc;
	HWND hwnd;
	MSG Msg;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = 0;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 32;
	wc.hInstance	 = NULL;
	wc.hIcon		 = NULL;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm		 = NULL;

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		_( "Navit" ),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, NULL, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, TRUE);
	UpdateWindow(hwnd);

	g_idle_add (message_pump, NULL);

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

struct menu_priv {
	HWND wnd_handle;
	HMENU hMenu;
};

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

	if ( type == menu_type_submenu )
	{
		HMENU hSubMenu = NULL;
		hSubMenu = CreatePopupMenu();
		AppendMenu(menu->hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, name );
		ret->hMenu = hSubMenu;
	}
	else
	{
		AppendMenu( menu->hMenu, MF_STRING, menu_id, name );
	}
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
	POINT menu_pt = { 200,200 };
	POINT pnt = menu_pt;

	ClientToScreen(menu->wnd_handle, (LPPOINT) &pnt);

	if (menu->hMenu)
	{
		TrackPopupMenu (menu->hMenu, 0, pnt.x, pnt.y, 0, menu->wnd_handle, NULL);
	}
}

struct menu_priv* win32_gui_popup_new(struct gui_priv *this_, struct menu_methods *meth)
{
	struct menu_priv* ret = NULL;

	ret = g_new0(struct menu_priv, 1);
	*meth = menu_methods;

	menu_id = 4000;

	ret->hMenu = CreatePopupMenu();
	ret->wnd_handle = this_->hwnd;
	meth->popup=popup_activate;

printf( "create popup menu %d \n", ret->hMenu );

	return ret;
}

struct gui_methods win32_gui_methods = {
	NULL, // win32_gui_menubar_new,
	NULL, // win32_gui_toolbar_new,
	NULL, // win32_gui_statusbar_new,
	win32_gui_popup_new,
	win32_gui_set_graphics,
	NULL,
	NULL, // win32_gui_datawindow_new,
	win32_gui_add_bookmark,
};



static struct gui_priv *win32_gui_new( struct navit *nav, struct gui_methods *meth, struct attr **attrs)
{
	struct gui_priv *this_;

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
	plugin_register_graphics_type("win32_graphics", win32_graphics_new);
}
