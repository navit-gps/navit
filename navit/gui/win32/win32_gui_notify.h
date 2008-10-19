
enum message_id
{
    WINDOW_CREATE,
    WINDOW_SIZE,
    WINDOW_DESTROY,
    DBLCLICK,
    CLICK,
    CHANGE,
    BUTTON_CLICK,
    INVALID

};

struct datawindow_priv;
struct notify_priv* win32_gui_notify_new();
void win32_gui_notify(struct notify_priv* notify, HWND hwnd, int message_id, void(*func)(struct datawindow_priv *parent, int param1, int param2));
LRESULT CALLBACK message_handler(HWND hwnd, UINT win_message, WPARAM wParam, LPARAM lParam);
