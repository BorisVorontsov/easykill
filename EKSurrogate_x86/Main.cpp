#include <windows.h>

#include "main.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	HINSTANCE hEKDLL;
	WNDCLASSEX WCEX = {};
	MSG msg;
	
	if ((hEKDLL = LoadLibrary(L"EasyKill.dll")))
	{
		(FARPROC &)EK_InstallHook = GetProcAddress(hEKDLL, "EK_InstallHook");
		(FARPROC &)EK_RemoveHook = GetProcAddress(hEKDLL, "EK_RemoveHook");
	}
	else return 0;

	WCEX.cbSize = sizeof(WCEX);
	WCEX.lpfnWndProc = MainWndProc;
	WCEX.hInstance = hInstance;
	WCEX.lpszClassName = L"EKS_MessageOnlyWindow";

	if (RegisterClassEx(&WCEX)) {
		CreateWindowEx(0, WCEX.lpszClassName, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 128, 128,
			HWND_MESSAGE, NULL, WCEX.hInstance, 0);
	}

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			//
			break;
		case EKSM_INSTALL_X86_HOOK:
			EK_InstallHook();
			break;
		case EKSM_REMOVE_X86_HOOK:
			EK_RemoveHook();
			break;
		case EKSM_EXIT:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;
	}

	return 0;
}