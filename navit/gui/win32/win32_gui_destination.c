#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <glib.h>
#include "item.h"
#include "attr.h"
#include "navit.h"
#include "search.h"
#include "debug.h"
#include "util.h"
#include "win32_gui_notify.h"
#include "resources/resource.h"

static const TCHAR g_szDestinationClassName[] = TEXT("navit_gui_destinationwindow_class");

struct datawindow_priv
{
    HWND hwnd;
    HWND hwndLabel;
    HWND hwndEdit;
    HWND hwndList;
    HWND hwndButtonPrev;
    HWND hwndButtonNext;
    enum attr_type currentSearchState;
    struct search_list *sl;
    struct navit *nav;
    struct notify_priv *notifications;
};

static void setlayout(struct datawindow_priv *datawindow)
{
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    RECT winrect;
    GetWindowRect (datawindow->hwndList, &winrect);

    lvc.iSubItem = 1;
    lvc.cx = (winrect.right -  winrect.left) - 52  ;
    lvc.fmt = LVCFMT_LEFT;  // left-aligned column

    switch (datawindow->currentSearchState)
    {
    case attr_country_name:
    {
        Edit_SetText(datawindow->hwndLabel, TEXT("Country"));
        lvc.pszText = TEXT("Country");
    }
    break;
    case attr_town_name:
    {
        Edit_SetText(datawindow->hwndLabel, TEXT("Postal or Town"));
        lvc.pszText = TEXT("Town");
    }
    break;
    case attr_street_name:
    {
        Edit_SetText(datawindow->hwndLabel, TEXT("Street"));
        lvc.pszText = TEXT("Street");
    }
    break;
    default:
        break;

    }

    (void)ListView_SetColumn(datawindow->hwndList, 1, &lvc);

    Edit_SetText(datawindow->hwndEdit, TEXT(""));
    SetFocus(datawindow->hwndEdit);
}

static void notify_apply(struct datawindow_priv *datawindow, int index, int param2)
{
    TCHAR txtBuffer[1024];
    char search_string[1024];
    struct attr search_attr;
    struct search_list_result *res;
    

    if ( index >= 0 )
    {
        ListView_GetItemText(datawindow->hwndList, index, 1, txtBuffer, 1024);

        TCHAR_TO_UTF8(txtBuffer, search_string);

        search_attr.type = datawindow->currentSearchState;
        search_attr.u.str = search_string;

        search_list_search(datawindow->sl, &search_attr, 0);
        res=search_list_get_result(datawindow->sl);
    }

    switch (datawindow->currentSearchState)
    {
    case attr_country_name:
    {
        datawindow->currentSearchState = attr_town_name;
    }
    break;
    case attr_town_name:
    {
        datawindow->currentSearchState = attr_street_name;
    }
    break;
    case attr_street_name:
    {
        navit_set_destination(datawindow->nav, res->c, "Mein Test");
        DestroyWindow(datawindow->hwnd);
    }
    break;
    default:
        break;

    }

    setlayout(datawindow);

}

static void notify_back(struct datawindow_priv *datawindow, int param1, int param2)
{
    switch (datawindow->currentSearchState)
    {
    case attr_country_name:
    break;
    case attr_town_name:
    {
        datawindow->currentSearchState = attr_country_name;
    }
    break;
    case attr_street_name:
    {
        datawindow->currentSearchState = attr_town_name;
    }
    break;
    default:
        break;

    }

    setlayout(datawindow);
}

static void notify_textchange(struct datawindow_priv *datawindow, int param1, int param2)
{

    struct attr search_attr;
    struct search_list_result *res;
    char search_string[1024];
    TCHAR converted_iso2[32];
    

    int lineLength = Edit_LineLength(datawindow->hwndEdit, 0);
    TCHAR line[lineLength + 1];
    (void)Edit_GetLine(datawindow->hwndEdit, 0, line, lineLength  + 1);
    line[lineLength] = 0;


    (void)ListView_DeleteAllItems( datawindow->hwndList);

    TCHAR_TO_UTF8(line, search_string);

    search_attr.type = datawindow->currentSearchState;
    search_attr.u.str = search_string;

    if (lineLength<1)
        return;

    search_list_search(datawindow->sl, &search_attr, 1);


    TCHAR *tcharBuffer = NULL;
    int listIndex = 0;
    LVITEM lvI;

    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    while ((res=search_list_get_result(datawindow->sl)) && listIndex < 50)
    {

        switch (search_attr.type)
        {
        case attr_country_name:
            tcharBuffer = newSysString(res->country->name);
            break;
        case attr_town_name:
            tcharBuffer = newSysString(res->town->name);
            break;
        case attr_street_name:
            if (res->street->name)
            {
                tcharBuffer = newSysString(res->street->name);
            }
            else
            {
                continue;
            }
            break;
        default:
            dbg(0, "Unhandled search type");
        }

        lvI.iItem = listIndex;
        lvI.iImage = listIndex;
        lvI.iSubItem = 0;
        lvI.lParam = (LPARAM) res->country->iso2;
        UTF8_TO_TCHAR(res->country->iso2, converted_iso2);
        lvI.pszText = converted_iso2;//LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
        (void)ListView_InsertItem(datawindow->hwndList, &lvI);
        ListView_SetItemText(datawindow->hwndList, listIndex, 1, tcharBuffer);
        g_free(tcharBuffer);
        dbg(0,"%s\n", res->country->name);
        listIndex++;
    }
}

static void notify_destroy(struct datawindow_priv *datawindow, int param1, int param2)
{
    if ( datawindow )
    {
        search_list_destroy(datawindow->sl);
        g_free(datawindow);
    }
}

static void notify_size(struct datawindow_priv *datawindow, int width, int height)
{
   if (datawindow)
   {
        MoveWindow(datawindow->hwndLabel,
                   0, 0,                  // starting x- and y-coordinates
                   width,        // width of client area
                   20,        // height of client area
                   TRUE);                 // repaint window
        MoveWindow(datawindow->hwndEdit,
                   0, 20,                  // starting x- and y-coordinates
                   width,        // width of client area
                   20,        // height of client area
                   TRUE);                 // repaint window
        MoveWindow(datawindow->hwndList,
                   0, 40,                  // starting x- and y-coordinates
                   width,        // width of client area
                   height - 60,        // height of client area
                   TRUE);                 // repaint window
        MoveWindow(datawindow->hwndButtonPrev,
                   0, height - 20,                  // starting x- and y-coordinates
                   width/2,        // width of client area
                   20,        // height of client area
                   TRUE);                 // repaint window
        MoveWindow(datawindow->hwndButtonNext,
                   width/2, height - 20,                  // starting x- and y-coordinates
                   width/2,        // width of client area
                   20,        // height of client area
                   TRUE);                 // repaint window

        setlayout(datawindow);

    }
}

static BOOL init_lv_columns(HWND hWndListView)
{

//    struct LVCOLUMN lvc = {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
//                LVCFMT_LEFT, 100, szText[iCol], 0, iCol, 0, 0 };

    TCHAR szText[][8] = {TEXT("Iso"),TEXT("Country")};     // temporary buffer
    LVCOLUMN lvc;
    int iCol;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    for (iCol = 0; iCol < 2; iCol++)
    {
        lvc.iSubItem = iCol;
        lvc.pszText = szText[iCol];
        lvc.cx = 50;     // width of column in pixels

        if ( iCol < 2 )
            lvc.fmt = LVCFMT_LEFT;  // left-aligned column
        else
            lvc.fmt = LVCFMT_RIGHT; // right-aligned column

        if (ListView_InsertColumn(hWndListView, iCol, &lvc) == -1)
            return FALSE;
    }
    return TRUE;
}

BOOL register_destination_window()
{
    WNDCLASS wc;

    wc.style		 = 0;
    wc.lpfnWndProc	 = message_handler;
    wc.cbClsExtra	 = 0;
    wc.cbWndExtra	 = 32;
    wc.hInstance	 = NULL;
    wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szDestinationClassName;
    wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NAVIT));

    if (!RegisterClass(&wc))
    {
        dbg(0, "Window Registration Failed!\n");
        return FALSE;
    }
    return TRUE;
}

HANDLE create_destination_window( struct navit *nav )
{


    struct datawindow_priv *this_;

    this_=g_new0(struct datawindow_priv, 1);
    this_->nav = nav;
    this_->currentSearchState = attr_country_name;
    this_->sl=search_list_new(navit_get_mapset(this_->nav));

    this_->hwnd = CreateWindowEx(
                      WS_EX_CLIENTEDGE,
                      g_szDestinationClassName,
                      TEXT("Destination Input"),
#if defined(__CEGCC__)
                      WS_SYSMENU | WS_CLIPCHILDREN,
                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
#else
                      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                      CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
#endif
                      NULL, NULL, NULL, NULL);

    if (this_->hwnd == NULL)
    {
        dbg(0, "Window Creation Failed!\n");
        return 0;
    }

    this_->notifications = win32_gui_notify_new(this_);
    SetWindowLongPtr( this_->hwnd , DWLP_USER, (LONG_PTR) this_->notifications );

    this_->hwndLabel = CreateWindow(WC_STATIC,      // predefined class
                                    TEXT("Country"),        // no window title
                                    WS_CHILD | WS_VISIBLE | ES_LEFT , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                    0, 0, 0, 0,  // set size in WM_SIZE message
                                    this_->hwnd,        // parent window
                                    NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                    (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                    NULL);       // pointer not needed

    this_->hwndEdit = CreateWindow(WC_EDIT,      // predefined class
                                   NULL,        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed

    this_->hwndList = CreateWindow(WC_LISTVIEW,      // predefined class
                                   NULL,        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | LVS_REPORT  , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed

    this_->hwndButtonPrev = CreateWindow(WC_BUTTON,      // predefined class
                                   TEXT("<<"),        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | LVS_REPORT  , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed
    this_->hwndButtonNext = CreateWindow(WC_BUTTON,      // predefined class
                                   TEXT(">>"),        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | LVS_REPORT  , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed
#ifdef LVS_EX_FULLROWSELECT
    (void)ListView_SetExtendedListViewStyle(this_->hwndList,LVS_EX_FULLROWSELECT);
#endif


    win32_gui_notify( this_->notifications, this_->hwndEdit, CHANGE, notify_textchange);
    win32_gui_notify( this_->notifications, NULL, WINDOW_SIZE, notify_size);
    win32_gui_notify( this_->notifications, this_->hwndList, DBLCLICK, notify_apply);
    win32_gui_notify( this_->notifications, this_->hwnd, WINDOW_DESTROY, notify_destroy);

    win32_gui_notify( this_->notifications, this_->hwndButtonNext, BUTTON_CLICK, notify_apply);
    win32_gui_notify( this_->notifications, this_->hwndButtonPrev, BUTTON_CLICK, notify_back);

    init_lv_columns(this_->hwndList);
    SetFocus(this_->hwndEdit);
    ShowWindow(this_->hwnd, TRUE);
    UpdateWindow(this_->hwnd);

    return this_->hwnd;
}

