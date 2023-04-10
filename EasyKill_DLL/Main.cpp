///////////////////////////////////////////////////////////////////
//	© Boris Vorontsov aka BV (borisvorontsov@gmail.com)
///////////////////////////////////////////////////////////////////

#include <windows.h>

#include "main.h"

HANDLE hDLL;
HHOOK hHook;
OSVERSIONINFO OSVI = {};

BOOL WINAPI DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			hDLL = hModule;
			OSVI.dwOSVersionInfoSize = sizeof(OSVI);
			GetVersionEx(&OSVI);
			if (OSVI.dwMajorVersion >= 5 && OSVI.dwMinorVersion >= 1)
			{
				HINSTANCE hUser32;
				hUser32 = GetModuleHandle(L"user32.dll");
				if (hUser32 == NULL)
					hUser32 = LoadLibrary(L"user32.dll");
				void (__stdcall * DisableProcessWindowsGhosting)(void);
				(FARPROC &)DisableProcessWindowsGhosting = GetProcAddress(hUser32, 
					"DisableProcessWindowsGhosting");
				DisableProcessWindowsGhosting();
			}
			return TRUE;
		case DLL_PROCESS_DETACH:
			hDLL = NULL;
			return TRUE;
	}
	return TRUE;
}

extern "C" BOOL __stdcall EK_InstallHook(void)
{
	hHook = SetWindowsHookEx(WH_GETMESSAGE, GMHookProc, (HINSTANCE)hDLL, 0);
	if (hHook == NULL) return FALSE;
	return TRUE;
}

extern "C" BOOL __stdcall EK_RemoveHook(void)
{
	if (UnhookWindowsHookEx(hHook) == 0) return FALSE;
	return TRUE;
}

LRESULT CALLBACK GMHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}