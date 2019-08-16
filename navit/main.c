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

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <sys/types.h>

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#endif

#include "file.h"
#include "debug.h"
#include "main.h"
#include "navit.h"
#include "gui.h"
#include "item.h"
#include "xmlconfig.h"
#include "coord.h"
#include "route.h"
#include "navigation.h"
#include "event.h"
#include "callback.h"
#include "navit_nls.h"
#include "util.h"
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#include <winbase.h>
#endif

#ifdef HAVE_API_WIN32_CE
#include "libc.h"
#endif

struct map_data *map_data_default;

struct callback_list *cbl;

#ifdef HAVE_API_WIN32
void setenv(char *var, char *val, int overwrite) {
    char *str=g_strdup_printf("%s=%s",var,val);
    if (overwrite || !getenv(var))
        putenv(str);
    g_free(str);
}
#endif

/*
 * @def environment_vars
 *
 * @brief Environment variables automatically added (and expanded) by navit at startup
 *
 * A NUL-terminated string array
 * environment_vars[0] is the name of the variable
 * environment_vars[1] is the value used when running from source dir
 * environment_vars[2] is the value used on Linux
 * environment_vars[3] is the value used on Windows (see main_init())
 * environment_vars[4] is the value used on Android
 * ':'  is replaced with NAVIT_PREFIX
 * '::' is replaced with NAVIT_PREFIX and LIBDIR
 * '~'  is replaced with HOME
*/
static char *environment_vars[][5]= {
    {"NAVIT_LIBDIR",      ":",          ":/"LIB_DIR,     ":\\lib",      ":/lib"},
    {"NAVIT_SHAREDIR",    ":",          ":/"SHARE_DIR,   ":",           ":/share"},
    {"NAVIT_LOCALEDIR",   ":/../locale",":/"LOCALE_DIR,  ":\\locale",   ":/locale"},
    {"NAVIT_USER_DATADIR",":",          "~/.navit",      ":\\data",     ":/home"},
    {"NAVIT_LOGFILE",     NULL,         NULL,            ":\\navit.log",NULL},
    {"NAVIT_LIBPREFIX",   "*/.libs/",   NULL,            NULL,          NULL},
    {NULL,                NULL,         NULL,            NULL,          NULL},
};

static void main_setup_environment(int mode) {
    int i=0;
    char *var,*val,*homedir;
    while ((var=environment_vars[i][0])) {
        val=environment_vars[i][mode+1];
        if (val) {
            switch (val[0]) {
            case ':':
                if (val[1] == ':')
                    val=g_strdup_printf("%s/%s%s", getenv("NAVIT_PREFIX"), LIBDIR+sizeof(PREFIX), val+2);
                else
                    val=g_strdup_printf("%s%s", getenv("NAVIT_PREFIX"), val+1);
                break;
            case '~':
                homedir=getenv("HOME");
                if (!homedir)
                    homedir="./";
                val=g_strdup_printf("%s%s", homedir, val+1);
                break;
            default:
                val=g_strdup(val);
                break;
            }
            setenv(var, val, 0);
            g_free(val);
        }
        i++;
    }
}

#ifdef HAVE_API_WIN32_BASE
char *nls_table[][3]= {
    // NLS Table compiled by Nick "Number6" Geoghegan
    // Not an exhaustive list, but supports 99% of all languages in Windows
    //{"LANGNAME", "CTRYNAME", "Language Code"},
    {"AFK", "ZAF", "af_ZA"},	// Afrikaans (South Africa)
    {"SQI", "ALB", "sq_AL"},	// Albanian (Albania)
    {"AMH", "ETH", "am_ET"},	// Amharic (Ethiopia)
    {"ARG", "DZA", "ar_DZ"},	// Arabic (Algeria)
    {"ARH", "BHR", "ar_BH"},	// Arabic (Bahrain)
    {"ARE", "EGY", "ar_EG"},	// Arabic (Egypt)
    {"ARI", "IRQ", "ar_IQ"},	// Arabic (Iraq)
    {"ARJ", "JOR", "ar_JO"},	// Arabic (Jordan)
    {"ARK", "KWT", "ar_KW"},	// Arabic (Kuwait)
    {"ARB", "LBN", "ar_LB"},	// Arabic (Lebanon)
    {"ARL", "LBY", "ar_LY"},	// Arabic (Libya)
    {"ARM", "MAR", "ar_MA"},	// Arabic (Morocco)
    {"ARO", "OMN", "ar_OM"},	// Arabic (Oman)
    {"ARQ", "QAT", "ar_QA"},	// Arabic (Qatar)
    {"ARA", "SAU", "ar_SA"},	// Arabic (Saudi Arabia)
    {"ARS", "SYR", "ar_SY"},	// Arabic (Syria)
    {"ART", "TUN", "ar_TN"},	// Arabic (Tunisia)
    {"ARU", "ARE", "ar_AE"},	// Arabic (U.A.E.)
    {"ARY", "YEM", "ar_YE"},	// Arabic (Yemen)
    {"HYE", "ARM", "hy_AM"},	// Armenian (Armenia)
    {"ASM", "IND", "as_IN"},	// Assamese (India)
    {"BAS", "RUS", "ba_RU"},	// Bashkir (Russia)
    {"EUQ", "ESP", "eu_ES"},	// Basque (Basque)
    {"BEL", "BLR", "be_BY"},	// Belarusian (Belarus)
    {"BNG", "BDG", "bn_BD"},	// Bengali (Bangladesh)
    {"BNG", "IND", "bn_IN"},	// Bengali (India)
    {"BRE", "FRA", "br_FR"},	// Breton (France)
    {"BGR", "BGR", "bg_BG"},	// Bulgarian (Bulgaria)
    {"CAT", "ESP", "ca_ES"},	// Catalan (Catalan)
    {"ZHH", "HKG", "zh_HK"},	// Chinese (Hong Kong S.A.R.)
    {"ZHM", "MCO", "zh_MO"},	// Chinese (Macao S.A.R.)
    {"CHS", "CHN", "zh_CN"},	// Chinese (People's Republic of China)
    {"ZHI", "SGP", "zh_SG"},	// Chinese (Singapore)
    {"CHT", "TWN", "zh_TW"},	// Chinese (Taiwan)
    {"COS", "FRA", "co_FR"},	// Corsican (France)
    {"HRV", "HRV", "hr_HR"},	// Croatian (Croatia)
    {"HRB", "BIH", "hr_BA"},	// Croatian (Latin, Bosnia and Herzegovina)
    {"CSY", "CZE", "cs_CZ"},	// Czech (Czech Republic)
    {"DAN", "DNK", "da_DK"},	// Danish (Denmark)
    {"NLB", "BEL", "nl_BE"},	// Dutch (Belgium)
    {"NLD", "NLD", "nl_NL"},	// Dutch (Netherlands)
    {"ENA", "AUS", "en_AU"},	// English (Australia)
    {"ENL", "BLZ", "en_BZ"},	// English (Belize)
    {"ENC", "CAN", "en_CA"},	// English (Canada)
    {"ENB", "CAR", "en_CB"},	// English (Caribbean)
    {"ENN", "IND", "en_IN"},	// English (India)
    {"ENI", "IRL", "en_IE"},	// English (Ireland)
    {"ENJ", "JAM", "en_JM"},	// English (Jamaica)
    {"ENM", "MYS", "en_MY"},	// English (Malaysia)
    {"ENZ", "NZL", "en_NZ"},	// English (New Zealand)
    {"ENP", "PHL", "en_PH"},	// English (Republic of the Philippines)
    {"ENE", "SGP", "en_SG"},	// English (Singapore)
    {"ENS", "ZAF", "en_ZA"},	// English (South Africa)
    {"ENT", "TTO", "en_TT"},	// English (Trinidad and Tobago)
    {"ENG", "GBR", "en_GB"},	// English (United Kingdom)
    {"ENU", "USA", "en_US"},	// English (United States)
    {"ENW", "ZWE", "en_ZW"},	// English (Zimbabwe)
    {"ETI", "EST", "et_EE"},	// Estonian (Estonia)
    {"FOS", "FRO", "fo_FO"},	// Faroese (Faroe Islands)
    {"FIN", "FIN", "fi_FI"},	// Finnish (Finland)
    {"FRB", "BEL", "fr_BE"},	// French (Belgium)
    {"FRC", "CAN", "fr_CA"},	// French (Canada)
    {"FRA", "FRA", "fr_FR"},	// French (France)
    {"FRL", "LUX", "fr_LU"},	// French (Luxembourg)
    {"FRM", "MCO", "fr_MC"},	// French (Principality of Monaco)
    {"FRS", "CHE", "fr_CH"},	// French (Switzerland)
    {"FYN", "NLD", "fy_NL"},	// Frisian (Netherlands)
    {"GLC", "ESP", "gl_ES"},	// Galician (Galician)
    {"KAT", "GEO", "ka_GE"},	// Georgian (Georgia)
    {"DEA", "AUT", "de_AT"},	// German (Austria)
    {"DEU", "DEU", "de_DE"},	// German (Germany)
    {"DEC", "LIE", "de_LI"},	// German (Liechtenstein)
    {"DEL", "LUX", "de_LU"},	// German (Luxembourg)
    {"DES", "CHE", "de_CH"},	// German (Switzerland)
    {"ELL", "GRC", "el_GR"},	// Greek (Greece)
    {"KAL", "GRL", "kl_GL"},	// Greenlandic (Greenland)
    {"GUJ", "IND", "gu_IN"},	// Gujarati (India)
    {"HEB", "ISR", "he_IL"},	// Hebrew (Israel)
    {"HIN", "IND", "hi_IN"},	// Hindi (India)
    {"HUN", "HUN", "hu_HU"},	// Hungarian (Hungary)
    {"ISL", "ISL", "is_IS"},	// Icelandic (Iceland)
    {"IBO", "NGA", "ig_NG"},	// Igbo (Nigeria)
    {"IND", "IDN", "id_ID"},	// Indonesian (Indonesia)
    {"IRE", "IRL", "ga_IE"},	// Irish (Ireland)
    {"XHO", "ZAF", "xh_ZA"},	// isiXhosa (South Africa)
    {"ZUL", "ZAF", "zu_ZA"},	// isiZulu (South Africa)
    {"ITA", "ITA", "it_IT"},	// Italian (Italy)
    {"ITS", "CHE", "it_CH"},	// Italian (Switzerland)
    {"JPN", "JPN", "ja_JP"},	// Japanese (Japan)
    {"KDI", "IND", "kn_IN"},	// Kannada (India)
    {"KKZ", "KAZ", "kk_KZ"},	// Kazakh (Kazakhstan)
    {"KHM", "KHM", "km_KH"},	// Khmer (Cambodia)
    {"KIN", "RWA", "rw_RW"},	// Kinyarwanda (Rwanda)
    {"SWK", "KEN", "sw_KE"},	// Kiswahili (Kenya)
    {"KOR", "KOR", "ko_KR"},	// Korean (Korea)
    {"KYR", "KGZ", "ky_KG"},	// Kyrgyz (Kyrgyzstan)
    {"LAO", "LAO", "lo_LA"},	// Lao (Lao P.D.R.)
    {"LVI", "LVA", "lv_LV"},	// Latvian (Latvia)
    {"LTH", "LTU", "lt_LT"},	// Lithuanian (Lithuania)
    {"LBX", "LUX", "lb_LU"},	// Luxembourgish (Luxembourg)
    {"MKI", "MKD", "mk_MK"},	// Macedonian (Former Yugoslav Republic of Macedonia)
    {"MSB", "BRN", "ms_BN"},	// Malay (Brunei Darussalam)
    {"MSL", "MYS", "ms_MY"},	// Malay (Malaysia)
    {"MYM", "IND", "ml_IN"},	// Malayalam (India)
    {"MLT", "MLT", "mt_MT"},	// Maltese (Malta)
    {"MRI", "NZL", "mi_NZ"},	// Maori (New Zealand)
    {"MAR", "IND", "mr_IN"},	// Marathi (India)
    {"MON", "MNG", "mn_MN"},	// Mongolian (Cyrillic, Mongolia)
    {"NEP", "NEP", "ne_NP"},	// Nepali (Nepal)
    {"NOR", "NOR", "nb_NO"},	// Norwegian, Bokmå(Norway)
    {"NON", "NOR", "nn_NO"},	// Norwegian, Nynorsk (Norway)
    {"OCI", "FRA", "oc_FR"},	// Occitan (France)
    {"ORI", "IND", "or_IN"},	// Oriya (India)
    {"PAS", "AFG", "ps_AF"},	// Pashto (Afghanistan)
    {"FAR", "IRN", "fa_IR"},	// Persian
    {"PLK", "POL", "pl_PL"},	// Polish (Poland)
    {"PTB", "BRA", "pt_BR"},	// Portuguese (Brazil)
    {"PTG", "PRT", "pt_PT"},	// Portuguese (Portugal)
    {"PAN", "IND", "pa_IN"},	// Punjabi (India)
    {"ROM", "ROM", "ro_RO"},	// Romanian (Romania)
    {"RMC", "CHE", "rm_CH"},	// Romansh (Switzerland)
    {"RUS", "RUS", "ru_RU"},	// Russian (Russia)
    {"SMG", "FIN", "se_FI"},	// Sami, Northern (Finland)
    {"SME", "NOR", "se_NO"},	// Sami, Northern (Norway)
    {"SMF", "SWE", "se_SE"},	// Sami, Northern (Sweden)
    {"SAN", "IND", "sa_IN"},	// Sanskrit (India)
    {"TSN", "ZAF", "tn_ZA"},	// Setswana (South Africa)
    {"SIN", "LKA", "si_LK"},	// Sinhala (Sri Lanka)
    {"SKY", "SVK", "sk_SK"},	// Slovak (Slovakia)
    {"SLV", "SVN", "sl_SI"},	// Slovenian (Slovenia)
    {"ESS", "ARG", "es_AR"},	// Spanish (Argentina)
    {"ESB", "BOL", "es_BO"},	// Spanish (Bolivia)
    {"ESL", "CHL", "es_CL"},	// Spanish (Chile)
    {"ESO", "COL", "es_CO"},	// Spanish (Colombia)
    {"ESC", "CRI", "es_CR"},	// Spanish (Costa Rica)
    {"ESD", "DOM", "es_DO"},	// Spanish (Dominican Republic)
    {"ESF", "ECU", "es_EC"},	// Spanish (Ecuador)
    {"ESE", "SLV", "es_SV"},	// Spanish (El Salvador)
    {"ESG", "GTM", "es_GT"},	// Spanish (Guatemala)
    {"ESH", "HND", "es_HN"},	// Spanish (Honduras)
    {"ESM", "MEX", "es_MX"},	// Spanish (Mexico)
    {"ESI", "NIC", "es_NI"},	// Spanish (Nicaragua)
    {"ESA", "PAN", "es_PA"},	// Spanish (Panama)
    {"ESZ", "PRY", "es_PY"},	// Spanish (Paraguay)
    {"ESR", "PER", "es_PE"},	// Spanish (Peru)
    {"ESU", "PRI", "es_PR"},	// Spanish (Puerto Rico)
    {"ESN", "ESP", "es_ES"},	// Spanish (Spain)
    {"EST", "USA", "es_US"},	// Spanish (United States)
    {"ESY", "URY", "es_UY"},	// Spanish (Uruguay)
    {"ESV", "VEN", "es_VE"},	// Spanish (Venezuela)
    {"SVF", "FIN", "sv_FI"},	// Swedish (Finland)
    {"SVE", "SWE", "sv_SE"},	// Swedish (Sweden)
    {"TAM", "IND", "ta_IN"},	// Tamil (India)
    {"TTT", "RUS", "tt_RU"},	// Tatar (Russia)
    {"TEL", "IND", "te_IN"},	// Telugu (India)
    {"THA", "THA", "th_TH"},	// Thai (Thailand)
    {"BOB", "CHN", "bo_CN"},	// Tibetan (PRC)
    {"TRK", "TUR", "tr_TR"},	// Turkish (Turkey)
    {"TUK", "TKM", "tk_TM"},	// Turkmen (Turkmenistan)
    {"UIG", "CHN", "ug_CN"},	// Uighur (PRC)
    {"UKR", "UKR", "uk_UA"},	// Ukrainian (Ukraine)
    {"URD", "PAK", "ur_PK"},	// Urdu (Islamic Republic of Pakistan)
    {"VIT", "VNM", "vi_VN"},	// Vietnamese (Vietnam)
    {"CYM", "GBR", "cy_GB"},	// Welsh (United Kingdom)
    {"WOL", "SEN", "wo_SN"},	// Wolof (Senegal)
    {"III", "CHN", "ii_CN"},	// Yi (PRC)
    {"YOR", "NGA", "yo_NG"},	// Yoruba (Nigeria)
    {NULL,NULL,NULL},		// Default - Can't find the language / Language not listed above
};

static void win_set_nls(void) {
    char country[32],lang[32];
    int i=0;

#ifdef HAVE_API_WIN32_CE
    wchar_t wcountry[32],wlang[32];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, wlang, sizeof(wlang));
    WideCharToMultiByte(CP_ACP,0,wlang,-1,lang,sizeof(lang),NULL,NULL);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, wcountry, sizeof(wcountry));
    WideCharToMultiByte(CP_ACP,0,wcountry,-1,country,sizeof(country),NULL,NULL);
#else
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, country, sizeof(country));
#endif
    while (nls_table[i][0]) {
        if (!strcmp(nls_table[i][0], lang) && !(strcmp(nls_table[i][1], country))) {
            dbg(lvl_info,"Setting LANG=%s for Lang %s Country %s",nls_table[i][2], lang, country);
            setenv("LANG",nls_table[i][2],0);
            return;
        }
        i++;
    }
    dbg(lvl_error,"Lang %s Country %s not found",lang,country);
}
#endif

void main_init(const char *program) {
    char *s;
#ifdef _UNICODE		/* currently for wince */
    wchar_t wfilename[MAX_PATH + 1];
#endif

    spawn_process_init();

    cbl=callback_list_new();
#ifdef HAVE_API_WIN32_BASE
    win_set_nls();
#endif
#ifndef HAVE_API_ANDROID
    setenv("LC_NUMERIC","C",1);
    setlocale(LC_ALL,"");
    setlocale(LC_NUMERIC,"C");
#endif
#if !defined _WIN32 && !defined _WIN32_WCE
    if (file_exists("navit.c") || file_exists("navit.o") || file_exists("navit.lo") || file_exists("version.h")) {
        char buffer[PATH_MAX];
        printf("%s",_("Running from source directory\n"));
        getcwd(buffer, PATH_MAX);		/* libc of navit returns "dummy" */
        setenv("NAVIT_PREFIX", buffer, 0);
        main_setup_environment(0);
    } else {
        if (!getenv("NAVIT_PREFIX")) {
            int l;
            int progpath_len;
            char *progpath="/bin/navit";
            l=strlen(program);
            progpath_len=strlen(progpath);
            if (l > progpath_len && !strcmp(program+l-progpath_len,progpath)) {
                s=g_strdup(program);
                s[l-progpath_len]='\0';
                if (strcmp(s, PREFIX))
                    printf(_("setting '%s' to '%s'\n"), "NAVIT_PREFIX", s);
                setenv("NAVIT_PREFIX", s, 0);
                g_free(s);
            } else
                setenv("NAVIT_PREFIX", PREFIX, 0);
        }
#ifdef HAVE_API_ANDROID
        main_setup_environment(3);
#else
        main_setup_environment(1);
#endif
    }

#else		/* _WIN32 || _WIN32_WCE */
    if (!getenv("NAVIT_PREFIX")) {
        char  filename[MAX_PATH + 1],
              *end;
        int len;

        *filename = '\0';
#ifdef _UNICODE		/* currently for wince */
        if (GetModuleFileNameW(NULL, wfilename, MAX_PATH)) {
            wcstombs(filename, wfilename, MAX_PATH);
#else
        if (GetModuleFileName(NULL, filename, MAX_PATH)) {
#endif
            end = strrchr(filename, L'\\');	/* eliminate the file name which is on the right side */
            if(end)
                *end = '\0';
        }
        len=strlen(filename);
        if (len > 4 && !strcmp(filename+len-4,"\\bin")) {
            filename[len-4]='\0';
        }
        setenv("NAVIT_PREFIX", filename, 0);
    }
    if (!getenv("HOME"))
        setenv("HOME", getenv("NAVIT_PREFIX"), 0);
    main_setup_environment(2);
#endif	/* _WIN32 || _WIN32_WCE */

    s = getenv("NAVIT_WID");
    if (s) {
        setenv("SDL_WINDOWID", s, 0);
    }
}

