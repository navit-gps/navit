#include <windows.h>
#include "ceglue.h"

BOOL FAR (*SHFullScreenPtr)(HWND hwnd, DWORD state) = NULL;

void InitCeGlue (void)
{
  HINSTANCE ayg = LoadLibraryW (TEXT ("aygshell.dll"));
  if (ayg != NULL) {
    SHFullScreenPtr = (BOOL (*)(HWND, DWORD))
      GetProcAddressW (ayg, TEXT ("SHFullScreen"));
  }
}

// code to turn of screen adopted from
// http://msdn.microsoft.com/en-us/library/ms838354.aspx

// GDI Escapes for ExtEscape()
#define QUERYESCSUPPORT    8
 
// The following are unique to CE
#define GETVFRAMEPHYSICAL   6144
#define GETVFRAMELEN    6145
#define DBGDRIVERSTAT    6146
#define SETPOWERMANAGEMENT   6147
#define GETPOWERMANAGEMENT   6148
 
 
typedef enum _VIDEO_POWER_STATE {
    VideoPowerOn = 1,
    VideoPowerStandBy,
    VideoPowerSuspend,
    VideoPowerOff
} VIDEO_POWER_STATE, *PVIDEO_POWER_STATE;
 
 
typedef struct _VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;


int CeEnableBacklight(int enable)
{
  HDC gdc;
  int iESC=SETPOWERMANAGEMENT;

  gdc = GetDC(NULL);
  if (ExtEscape(gdc, QUERYESCSUPPORT, sizeof(int), (LPCSTR)&iESC, 
      0, NULL)==0)
  {
    MessageBox(NULL,
                L"Sorry, your Pocket PC does not support DisplayOff",
                L"Pocket PC Display Off Feature",
              MB_OK);
    ReleaseDC(NULL, gdc);
    return FALSE;
  }
  else
  {
    VIDEO_POWER_MANAGEMENT vpm;
    vpm.Length = sizeof(VIDEO_POWER_MANAGEMENT);
    vpm.DPMSVersion = 0x0001;
    if (enable) {
      vpm.PowerState = VideoPowerOn;
    } else {
      vpm.PowerState = VideoPowerOff;
    }
    // Power off the display
    ExtEscape(gdc, SETPOWERMANAGEMENT, vpm.Length, (LPCSTR) &vpm, 
              0, NULL);
    ReleaseDC(NULL, gdc);
    return TRUE;
  }
}

