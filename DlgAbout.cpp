#include <windows.h>
#include <stdio.h>

#include "main.h"
#include "resource.h"
#include "dlgabout.h"

INT_PTR CALLBACK DlgAboutProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			WCHAR lpTitle[64] = {};
			WCHAR lpApp[64] = {};

			MoveToCenter(hWnd, 0, 0);

			swprintf(lpTitle, L"About %s", APP_NAME);
			SetWindowText(hWnd, lpTitle);

			SendMessage(hWnd, WM_SETICON, ICON_BIG,
				(LPARAM)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 32, 32, 0));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL,
				(LPARAM)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 16, 16, 0));

			wcscpy(lpApp, APP_NAME);
#ifdef _WIN64
			wcscat(lpApp, L" (x64)");
#endif
			wcscat(lpApp, L", version ");
			wcscat(lpApp, lpAppVersion);
			SetDlgItemText(hWnd, IDC_STCNAME, lpApp);
			SetDlgItemText(hWnd, IDC_STCCOP, APP_COPYRIGHT);

			hWndAbout = hWnd;
			return TRUE;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hWnd, 0);
				return TRUE;
			}
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
					switch (LOWORD(wParam))
					{
						case IDC_BTNOK:
							EndDialog(hWnd, 0);
							break;
						case IDC_BTNSE:
						{
							WCHAR lpCmd[128] = {};
							swprintf(lpCmd, L"%s?subject=%s %s", CMD_SENDEMAIL, APP_NAME, lpAppVersion);
							ShellExecute(hWnd, L"Open", lpCmd, 0, 0, SW_NORMAL);
							break;
						}
					}
					break;
			}
			return TRUE;
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
		case WM_DESTROY:
			hWndAbout = NULL;
			return TRUE;
	}
	return FALSE;
}