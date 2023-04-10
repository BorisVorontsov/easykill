#include <windows.h>
#include <commctrl.h>

#include "main.h"
#include "resource.h"
#include "settings.h"
#include "dlgpagegeneral.h"

static LONG_PTR lOldHKEditProc;

INT_PTR CALLBACK DlgPageGeneralProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
			if (Settings.m_dwTarget == 0)
			{
				SendDlgItemMessage(hWnd, IDC_OPTTGT1, BM_SETCHECK, TRUE, 0);
			}
			else
			{
				SendDlgItemMessage(hWnd, IDC_OPTTGT2, BM_SETCHECK, TRUE, 0);
			}
			if (Settings.m_dwWindowGhostingWorkaround)
				SendDlgItemMessage(hWnd, IDC_CHKWGW, BM_SETCHECK, TRUE, 0);
			if (Settings.m_dwTPTimeout)
				SendDlgItemMessage(hWnd, IDC_CHKTPTO, BM_SETCHECK, TRUE, 0);
			SendDlgItemMessage(hWnd, IDC_SPNTO, UDM_SETRANGE, 0, MAKELPARAM(60, 0));
			SendDlgItemMessage(hWnd, IDC_SPNTO, UDM_SETPOS, 0, MAKELPARAM(Settings.m_dwTPTimeoutValue, 0));
			if (Settings.m_dwAutorun)
				SendDlgItemMessage(hWnd, IDC_CHKAR, BM_SETCHECK, TRUE, 0);

			if ((Settings.m_dwModifiers & MOD_CONTROL) == MOD_CONTROL)
				SendDlgItemMessage(hWnd, IDC_CHKCTRL, BM_SETCHECK, TRUE, 0);
			if ((Settings.m_dwModifiers & MOD_ALT) == MOD_ALT)
				SendDlgItemMessage(hWnd, IDC_CHKALT, BM_SETCHECK, TRUE, 0);
			if ((Settings.m_dwModifiers & MOD_SHIFT) == MOD_SHIFT)
				SendDlgItemMessage(hWnd, IDC_CHKSFT, BM_SETCHECK, TRUE, 0);
			if ((Settings.m_dwModifiers & MOD_WIN) == MOD_WIN)
				SendDlgItemMessage(hWnd, IDC_CHKWIN, BM_SETCHECK, TRUE, 0);

			lOldHKEditProc = SetWindowLongPtr(GetDlgItem(hWnd, IDC_EDTKEY), GWLP_WNDPROC, (LONG_PTR)HKEditProc);
			SetWindowLongPtr(GetDlgItem(hWnd, IDC_EDTKEY), GWLP_USERDATA, (LONG_PTR)Settings.m_dwHotkey);
			SendDlgItemMessage(hWnd, IDC_EDTKEY, WM_KEYUP, (WPARAM)Settings.m_dwHotkey, 0);

			if (Settings.m_dwRebootOnHotkey)
				SendDlgItemMessage(hWnd, IDC_CHKROH, BM_SETCHECK, TRUE, 0);

			return TRUE;
		case WM_DESTROY:
			SetWindowLongPtr(GetDlgItem(hWnd, IDC_EDTKEY), GWLP_WNDPROC, lOldHKEditProc);
			return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK HKEditProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_KEYUP)
	{
		switch (wParam)
		{
			case VK_TAB:
			case VK_RETURN:
			case VK_ESCAPE:
			case VK_CONTROL:
				//VK_RCONTROL
				//VK_LCONTROL
			case VK_MENU:
				//VK_RMENU
				//VK_LMENU
			case VK_SHIFT:
				//VK_RSHIFT
				//VK_LSHIFT
			case VK_RWIN:
			case VK_LWIN:
				break;
			default:
			{
				WCHAR lpKey[16] = {};
				GetKeyName((DWORD)wParam, lpKey, sizeof(lpKey) / sizeof(WCHAR));
				SetWindowText(hWnd, lpKey);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wParam);
				break;
			}
		}
	}

	return CallWindowProc((WNDPROC)lOldHKEditProc, hWnd, Message, wParam, lParam);
}
