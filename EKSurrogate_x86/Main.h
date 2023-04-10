#ifndef EKSURROGATE_H_
#define EKSURROGATE_H_

#define EKSM_INSTALL_X86_HOOK	(WM_APP + 0x70)
#define EKSM_REMOVE_X86_HOOK	(WM_APP + 0x71)
#define EKSM_EXIT				(WM_APP + 0x72)

BOOL (__stdcall * EK_InstallHook)();
BOOL (__stdcall * EK_RemoveHook)();

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif