#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <glib.h>
#include "win32_gui_notify.h"

struct window_data
{
    HWND hwnd;
    UINT message;
    void(*func)(struct datawindow_priv *parent, int param1, int param2);
};

struct notify_priv
{
    GList *window_list;
    struct datawindow_priv *parent;

};


void win32_gui_notify(struct notify_priv* notify, HWND hwnd, int message_id, void(*func)(struct datawindow_priv *parent, int param1, int param2))
{
    struct window_data *wnd_data = g_new( struct window_data,1);

    wnd_data->hwnd = hwnd;
    wnd_data->message = message_id;
    wnd_data->func = func;

    notify->window_list = g_list_append( notify->window_list, (gpointer) wnd_data );

}

struct notify_priv* win32_gui_notify_new(struct datawindow_priv *parent)
{
    struct notify_priv* notify = g_new0(struct notify_priv,1);
    notify->parent = parent;
    return notify;
}

LRESULT CALLBACK message_handler(HWND hwnd, UINT win_message, WPARAM wParam, LPARAM lParam)
{
    enum message_id message = INVALID;
    int param1 = -1;
    int param2 = -1;
    HWND hwndDlg = hwnd;

    switch (win_message)
    {
        case WM_CREATE:
        {
            message = WINDOW_CREATE;
        }
        break;
        case WM_SIZE:
        {
            message = WINDOW_SIZE;
            param1 = LOWORD(lParam);
            param2 = HIWORD(lParam);
        }
        break;
        case WM_DESTROY:
        {
            message = WINDOW_DESTROY;
        }
        break;
        case WM_NOTIFY:
        {
            hwndDlg = (((LPNMHDR)lParam)->hwndFrom);
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_DBLCLK:
            {
                message = DBLCLICK;
#ifdef LPNMITEMACTIVATE
                param1 = ((LPNMITEMACTIVATE)lParam)->iItem;
#endif
            }
            break;
            case NM_CLICK:
                message = CLICK;
            break;
            }
        }
        break;
        case WM_COMMAND:
        {
            hwndDlg = (HWND)lParam;

            switch (HIWORD(wParam))
            {
                case EN_CHANGE:
                {
                    message = CHANGE;
                }
                break;
                case BN_CLICKED:
                {
                    message = BUTTON_CLICK;
                }
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, win_message, wParam, lParam);
    }

    struct notify_priv* notify_data = (struct notify_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

    if ( message != INVALID && notify_data && notify_data->window_list )
    {

        GList* current_element = g_list_first(notify_data->window_list);


        struct window_data* wnd_data = NULL;
        while (current_element != NULL)
        {
            wnd_data = current_element->data;

            if ( (wnd_data->hwnd == hwndDlg || wnd_data->hwnd == NULL) && message == wnd_data->message)
            {
                wnd_data->func(notify_data->parent, param1, param2);
            }

            current_element = g_list_next(current_element);
        }
    }
    return FALSE;
}
