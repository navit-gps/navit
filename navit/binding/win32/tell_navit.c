/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <XGetopt.h>
#endif
#include <glib.h>
#include "binding_win32.h"

static LRESULT CALLBACK message_handler( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    switch(uMsg) {
    case WM_CREATE:
        return 0;
    }
    return TRUE;
}

int errormode=1;

void err(char *fmt, ...) {
    va_list ap;
    char buf[1024];
#if defined HAVE_API_WIN32_CE
#define vsnprintf _vsnprintf
#endif
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    switch(errormode) {
    case 0: /* be silent */
        break;
    case 1:
        MessageBox(NULL, buf, "tell_navit", MB_ICONERROR|MB_OK);
        break;
    case 2:
        fprintf(stderr,"%s",buf);
        break;
    }
}

void print_usage(void) {
    err(
        "tell_navit usage:\n"
        "tell_navit [options] navit_command\n"
        "\t-h this help\n"
        "\t-e <way>: set way to report error messages:\n"
        "\t\t0 - suppress messages\n"
        "\t\t1 - use messagebox (default)\n"
        "\t\t2 - print to stderr\n"
    );
}

int main(int argc, char **argv) {
    HWND navitWindow;
    COPYDATASTRUCT cd;
    char opt;

    TCHAR *g_szClassName  = TEXT("TellNavitWND");
    WNDCLASS wc;
    HWND hwnd;
    HWND hWndParent=NULL;


    if(argc>0) {
        while((opt = getopt(argc, argv, ":hvc:d:e:s:")) != -1) {
            switch(opt) {
            case 'h':
                print_usage();
                exit(0);
                break;
            case 'e':
                errormode=atoi(optarg);
                break;
            default:
                err("Unknown option %c\n", opt);
                exit(1);
                break;
            }
        }
    } else {
        print_usage();
        exit(1);
    }
    if(optind==argc) {
        err("Navit command to execute is needed.");
        exit(1);
    }


    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc	= message_handler;
    wc.hInstance	= GetModuleHandle(NULL);
    wc.lpszClassName = g_szClassName;

    if (!RegisterClass(&wc)) {
        err(TEXT("Window class registration failed\n"));
        return 1;
    } else {
        hwnd = CreateWindow(
                   g_szClassName,
                   TEXT("Tell Navit"),
                   0,
                   0,
                   0,
                   0,
                   0,
                   hWndParent,
                   NULL,
                   GetModuleHandle(NULL),
                   NULL);
        if(!hwnd) {
            err(TEXT("Can't create hidden window\n"));
            UnregisterClass(g_szClassName,NULL);
            return 1;
        }
    }

    navitWindow=FindWindow( TEXT("NAVGRA"), NULL );
    if(!navitWindow) {
        err(TEXT("Navit window not found\n"));
        DestroyWindow(hwnd);
        UnregisterClass(g_szClassName,NULL);
        return 1;
    } else {
        int rv;
        char *command=g_strjoinv(" ",argv+optind);
        struct navit_binding_w32_msg *msg;
        int sz=sizeof(*msg)+strlen(command);

        cd.dwData=NAVIT_BINDING_W32_DWDATA;
        msg=g_malloc0(sz);
        msg->version=NAVIT_BINDING_W32_VERSION;
        g_strlcpy(msg->magic,NAVIT_BINDING_W32_MAGIC,sizeof(msg->magic));
        g_strlcpy(msg->text,command,sz-sizeof(*msg)+1);
        cd.cbData=sz;
        cd.lpData=msg;
        rv=SendMessage( navitWindow, WM_COPYDATA, (WPARAM)hwnd, (LPARAM) (LPVOID) &cd );
        g_free(command);
        g_free(msg);
        if(rv!=0) {
            err(TEXT("Error %d sending message, SendMessage return value is %d\n"), GetLastError(), rv);
            DestroyWindow(hwnd);
            UnregisterClass(g_szClassName,NULL);
            return 1;
        }
    }
    DestroyWindow(hwnd);
    UnregisterClass(g_szClassName,NULL);
    return 0;
}


