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

#define _WIN32_WINNT 0x0500

#include "config.h"

#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#include <mmsystem.h>
#include <winreg.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include "item.h"
#include "plugin.h"
#include "speech.h"
#include "util.h"
#include "file.h"
#include "debug.h"

#include "support/espeak/speech.h"
#include "support/espeak/speak_lib.h"
#include "support/espeak/phoneme.h"
#include "support/espeak/synthesize.h"
#include "support/espeak/voice.h"
#include "support/espeak/translate.h"


#define BUFFERS 4


// ----- some stuff needed by espeak ----------------------------------
char path_home[N_PATH_HOME];    // this is the espeak-data directory
int (* uri_callback)(int, const char *, const char *) = NULL;
int (* phoneme_callback)(const char *) = NULL;
FILE *f_wave = NULL;

int GetFileLength(const char *filename) {
    struct stat statbuf;

    if(stat(filename,&statbuf) != 0)
        return(0);

    if((statbuf.st_mode & S_IFMT) == S_IFDIR)
        return(-2);  // a directory

    return(statbuf.st_size);
}

void MarkerEvent(int type, unsigned int char_position, int value, int value2, unsigned char *out_ptr) {
}

char *Alloc(int size) {
    return g_malloc(size);
}

void Free(void *ptr) {
    g_free(ptr);
}

// --------------------------------------------------------------------


enum speech_messages {
    msg_say = WM_USER,
    msg_exit
};

enum speech_state {
    state_available,
    state_speaking_phase_1,
    state_speaking_phase_2,
    state_speaking_phase_3

};

struct speech_priv {
    GList *free_buffers;
    HWAVEOUT h_wave_out;
    enum speech_state state;
    GList *phrases;
    HWND h_queue;
    HANDLE h_message_thread;
};


static void waveout_close(struct speech_priv* sp_priv) {
    waveOutClose(sp_priv->h_wave_out);
}

static BOOL waveout_open(struct speech_priv* sp_priv) {
    MMRESULT result = 0;

    HWAVEOUT hwo;
    static WAVEFORMATEX wmTemp;
    wmTemp.wFormatTag = WAVE_FORMAT_PCM;
    wmTemp.nChannels = 1;
    wmTemp.nSamplesPerSec = 22050;
    wmTemp.wBitsPerSample = 16;
    wmTemp.nBlockAlign = wmTemp.nChannels * wmTemp.wBitsPerSample / 8;
    wmTemp.nAvgBytesPerSec = wmTemp.nSamplesPerSec * wmTemp.nBlockAlign;
    wmTemp.cbSize = 0;
    result = waveOutOpen(&hwo, (UINT) WAVE_MAPPER, &wmTemp, (DWORD)sp_priv->h_queue, (DWORD)sp_priv, CALLBACK_WINDOW);
    sp_priv->h_wave_out = hwo;

    return (result==MMSYSERR_NOERROR);
}

static int wave_out(struct speech_priv* sp_priv) {
    int isDone;

    WAVEHDR *WaveHeader = g_list_first(sp_priv->free_buffers)->data;
    sp_priv->free_buffers = g_list_remove(sp_priv->free_buffers, WaveHeader);

    out_ptr = out_start = WaveHeader->lpData;
    out_end = WaveHeader->lpData + WaveHeader->dwBufferLength;

    isDone = WavegenFill(0);

    if ( out_ptr < out_end ) {
        memset ( out_ptr, 0, out_end - out_ptr );
    }
    waveOutWrite(sp_priv->h_wave_out, WaveHeader, sizeof(WAVEHDR));

    return isDone;
}

static BOOL initialise(void) {
    int param;
    int result;

    WavegenInit(22050,0);   // 22050
    if((result = LoadPhData(NULL)) != 1) {
        if(result == -1) {
            dbg(lvl_error, "Failed to load espeak-data");
            return FALSE;
        } else
            dbg(lvl_error, "Wrong version of espeak-data 0x%x (expects 0x%x) at %s",result,version_phdata,path_home);
    }
    LoadConfig();
    SetVoiceStack(NULL, "");
    SynthesizeInit();

    for(param=0; param<N_SPEECH_PARAM; param++)
        param_stack[0].parameter[param] = param_defaults[param];

    return TRUE;
}

static void fill_buffer(struct speech_priv *this) {
    while ( this->free_buffers && this->state != state_speaking_phase_3 ) {
        if(Generate(phoneme_list,&n_phoneme_list,1)==0) {
            if (!SpeakNextClause(NULL,NULL,1)) {
                this->state = state_speaking_phase_2;
            }
        }

        if ( wave_out(this)!= 0 && this->state == state_speaking_phase_2) {
            this->state = state_speaking_phase_3;
        }
    }
}

static void start_speaking(struct speech_priv* sp_priv) {
    char *phrase = g_list_first(sp_priv->phrases)->data;

    sp_priv->state = state_speaking_phase_1;

    SpeakNextClause(NULL, phrase,0);
    wave_out(sp_priv);
    fill_buffer(sp_priv);
}

static LRESULT CALLBACK speech_message_handler( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    dbg(lvl_debug, "message_handler called");

    switch (uMsg) {
    case msg_say: {
        struct speech_priv* sp_priv = (struct speech_priv*)wParam;
        sp_priv->phrases = g_list_append(sp_priv->phrases, (char*)lParam);

        if ( sp_priv->state == state_available ) {
            start_speaking(sp_priv);
        }

    }
    break;
    case MM_WOM_DONE: {
        WAVEHDR *WaveHeader = (WAVEHDR *)lParam;
        struct speech_priv* sp_priv;
        dbg(lvl_info, "Wave buffer done");

        sp_priv = (struct speech_priv*)WaveHeader->dwUser;
        sp_priv->free_buffers = g_list_append(sp_priv->free_buffers, WaveHeader);

        if ( sp_priv->state != state_speaking_phase_3) {
            fill_buffer(sp_priv);
        } else if ( g_list_length(sp_priv->free_buffers) == BUFFERS && sp_priv->state == state_speaking_phase_3 ) {
            // remove the spoken phrase from the list
            char *phrase = g_list_first(sp_priv->phrases)->data;
            g_free( phrase );
            sp_priv->phrases = g_list_remove(sp_priv->phrases, phrase);

            if ( sp_priv->phrases ) {
                start_speaking(sp_priv);
            } else {
                sp_priv->state = state_available;
            }
        }
    }
    break;
    case msg_exit:
        ExitThread(0);
        break;

    default:
        break;

    }

    return TRUE;
}

static void speech_message_dispatcher( struct speech_priv * sp_priv) {
    BOOL bRet;
    MSG msg;

    while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0) {
        if (bRet == -1) {
            dbg(lvl_error, "Error getting message from queue");
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

static void create_buffers(struct speech_priv *sp_priv) {
    int buffer_counter;
    char *buffer_head;


    SYSTEM_INFO system_info;
    GetSystemInfo (&system_info);

    buffer_head = VirtualAlloc(0, system_info.dwPageSize * BUFFERS, MEM_RESERVE, PAGE_NOACCESS);

    for (buffer_counter = 0; buffer_counter < BUFFERS; buffer_counter++) {
        WAVEHDR *WaveHeader = g_new0(WAVEHDR, 1);

        WaveHeader->dwBufferLength = system_info.dwPageSize;
        WaveHeader->lpData = (char *)VirtualAlloc(buffer_head, WaveHeader->dwBufferLength, MEM_COMMIT, PAGE_READWRITE);
        buffer_head += WaveHeader->dwBufferLength;

        WaveHeader->dwUser = (DWORD)sp_priv;
        waveOutPrepareHeader(sp_priv->h_wave_out, WaveHeader, sizeof(WAVEHDR));

        sp_priv->free_buffers = g_list_append( sp_priv->free_buffers,  WaveHeader );
    }
}

static DWORD startThread( LPVOID sp_priv) {
    struct speech_priv *this = (struct speech_priv *) sp_priv;
    // Create message queue
    TCHAR *g_szClassName  = TEXT("SpeechQueue");
    WNDCLASS wc;
    HWND hwnd;
    HWND hWndParent;


    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc	= speech_message_handler;
    wc.hInstance	= GetModuleHandle(NULL);
    wc.lpszClassName = g_szClassName;

    if (!RegisterClass(&wc)) {
        dbg(lvl_error, "Window registration for message queue failed");
        return 1;
    }

    hWndParent = NULL;
#ifndef HAVE_API_WIN32_CE
    hWndParent = HWND_MESSAGE;
#endif

    // create a message only window
    hwnd = CreateWindow(
               g_szClassName,
               TEXT("Navit"),
               0,
               0,
               0,
               0,
               0,
               hWndParent,
               NULL,
               GetModuleHandle(NULL),
               NULL);

    if (hwnd == NULL) {
        dbg(lvl_error, "Window creation failed: %d",  GetLastError());
        return 1;
    }

    this->h_queue = hwnd;
    this->phrases = NULL;
    this->state = state_available;

    if(!waveout_open(this)) {
        dbg(lvl_error, "Can't open wave output");
        return 1;
    }

    this->free_buffers = NULL;
    create_buffers(this);

    speech_message_dispatcher(this);

    return 0;
}

static int espeak_say(struct speech_priv *this, const char *text) {
    char *phrase = g_strdup(text);
    dbg(lvl_debug, "Speak: '%s'", text);

    if (!PostMessage(this->h_queue, msg_say, (WPARAM)this, (LPARAM)phrase)) {
        dbg(lvl_error, "PostThreadMessage 'say' failed");
    }

    return 0;
}

static void free_list(gpointer pointer, gpointer this ) {
    if ( this ) {
        struct speech_priv *sp_priv = (struct speech_priv *)this;
        WAVEHDR *WaveHeader = (WAVEHDR *)pointer;

        waveOutUnprepareHeader(sp_priv->h_wave_out, WaveHeader, sizeof(WAVEHDR));
        VirtualFree(WaveHeader->lpData, WaveHeader->dwBufferLength, MEM_DECOMMIT);
    }
    g_free(pointer);
}

static void espeak_destroy(struct speech_priv *this) {
    g_list_foreach( this->free_buffers, free_list, (gpointer)this );
    g_list_free( this->free_buffers );

    g_list_foreach( this->phrases, free_list, 0 );
    g_list_free(this->phrases);

    waveout_close(this);
    g_free(this);
}

static struct speech_methods espeak_meth = {
    espeak_destroy,
    espeak_say,
};

static struct speech_priv *espeak_new(struct speech_methods *meth, struct attr **attrs, struct attr *parent) {
    struct speech_priv *this = NULL;
    struct attr *path;
    struct attr *language;
    char *lang_str=NULL;

    path=attr_search(attrs, attr_path);
    if (path)
        strcpy(path_home,path->u.str);
    else
        sprintf(path_home,"%s/espeak-data",getenv("NAVIT_SHAREDIR"));
    dbg(lvl_debug,"path_home set to %s",path_home);

    if ( !initialise() ) {
        return NULL;
    }

    language=attr_search(attrs, attr_language);
    if ( language ) {
        lang_str=g_strdup(language->u.str);
    } else {
        char *lang_env=getenv("LANG");

        if (lang_env) {
            char *country,*lang,*lang_full;
            char *file1;
            char *file2;
            lang_full=g_strdup(lang_env);
            strtolower(lang_full,lang_env);
            lang=g_strdup(lang_full);
            country=strchr(lang_full,'_');
            if (country) {
                lang[country-lang_full]='\0';
                *country++='-';
            }
            file1=g_strdup_printf("%s/voices/%s",path_home,lang_full);
            file2=g_strdup_printf("%s/voices/%s/%s",path_home,lang,lang_full);
            dbg(lvl_debug,"Testing %s and %s",file1,file2);
            if (file_exists(file1) || file_exists(file2))
                lang_str=g_strdup(lang_full);
            else
                lang_str=g_strdup(lang);
            dbg(lvl_debug,"Language full %s lang %s result %s",lang_full,lang,lang_str);
            g_free(lang_full);
            g_free(lang);
            g_free(file1);
            g_free(file2);
        }
    }
    if(lang_str && SetVoiceByName(lang_str) != EE_OK) {
        dbg(lvl_error, "Error setting language to: '%s',falling back to default", lang_str);
        g_free(lang_str);
        lang_str=NULL;
    }
    if(!lang_str && SetVoiceByName("default") != EE_OK) {
        dbg(lvl_error, "Error setting language to default");
    }


    SetParameter(espeakRATE,170,0);
    SetParameter(espeakVOLUME,100,0);
    SetParameter(espeakCAPITALS,option_capitals,0);
    SetParameter(espeakPUNCTUATION,option_punctuation,0);
    SetParameter(espeakWORDGAP,0,0);

//	if(pitch_adjustment != 50)
//	{
//		SetParameter(espeakPITCH,pitch_adjustment,0);
//	}
    DoVoiceChange(voice);

    this=g_new(struct speech_priv,1);
    this->h_message_thread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)startThread, (PVOID)this, 0, NULL);

    *meth=espeak_meth;


    return this;
}

void plugin_init(void) {
    plugin_register_category_speech("espeak", espeak_new);
}
