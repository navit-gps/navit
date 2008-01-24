#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <stdarg.h>
#include "config.h"
#include "plugin.h"
#include "speech.h"
#include <windows.h>
#define COBJMACROS
#include <sapi.h>
#include <objbase.h>
#include <glib.h>

const IID UUID_OF_ISPVOICE; // = {6C44DF74-72B9-4992-A1EC-EF996E0422D4};


struct speech_priv {
    ISpVoice* pIspVoice;
};

static int
speechd_say(struct speech_priv *this_, const char *text) {
    int err = 1;

    if ( this_->pIspVoice )
    {
        WCHAR* pWideString = g_utf8_to_utf16( text, -1, NULL, NULL, NULL );
        ISpVoice_Speak( this_->pIspVoice, pWideString, 0, NULL);
        g_free( pWideString );
        err = 0;
    }
	return err;
}

static void
speechd_destroy(struct speech_priv *this) {
	g_free(this);
}

static struct speech_methods speechd_meth = {
	speechd_destroy,
	speechd_say,
};

static struct speech_priv *
speechd_new(char *data, struct speech_methods *meth) {
	struct speech_priv *this_;

	this_=g_new(struct speech_priv,1);

    this_->pIspVoice = NULL;


    CLSID clsid_sape;
    CLSID* pw = &clsid_sape;

    CoInitialize(NULL);

    HRESULT hr = CLSIDFromProgID(L"SAPI.SpVoice", &clsid_sape);

    if ( 0 == hr )
    {
        IID UUID_OF_ISPVOICE; // = {6C44DF74-72B9-4992-A1EC-EF996E0422D4};
        UUID_OF_ISPVOICE.Data1 = 0x6C44DF74;
        UUID_OF_ISPVOICE.Data2 = 0x72B9;
        UUID_OF_ISPVOICE.Data3 = 0x4992;
        UUID_OF_ISPVOICE.Data4[0] = 161;
        UUID_OF_ISPVOICE.Data4[1] = 236;
        UUID_OF_ISPVOICE.Data4[2] = 239;
        UUID_OF_ISPVOICE.Data4[3] = 153;
        UUID_OF_ISPVOICE.Data4[4] = 110;
        UUID_OF_ISPVOICE.Data4[5] = 4;
        UUID_OF_ISPVOICE.Data4[6] = 34;
        UUID_OF_ISPVOICE.Data4[7] = 212;

        ISpVoice* pIspVoice;
        hr = CoCreateInstance( &clsid_sape, NULL, CLSCTX_ALL, &UUID_OF_ISPVOICE, &pIspVoice );

        if ( ( 0 == hr ) && ( this_ != NULL ) )
        {
            this_->pIspVoice = pIspVoice;
        }
    }

    // ISpVoice_Speak( pIspVoice, L"If you can hear this, SAPI is working", 0, NULL);
    *meth=speechd_meth;
	return this_;
}


void plugin_init(void)
{
	plugin_register_speech_type("speech_dispatcher", speechd_new);
}
