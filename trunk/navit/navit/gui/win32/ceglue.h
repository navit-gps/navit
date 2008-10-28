#ifndef CEGLUE_H
#define CEGLUE_H
#ifdef __cplusplus
extern "C" {
#endif

extern BOOL (*SHFullScreenPtr)(HWND hwnd, DWORD state);
void InitCeGlue (void);

int CeEnableBacklight(int enable);

#ifdef __cplusplus
}
#endif

#endif
