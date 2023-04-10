#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "main.h"
#include "resource.h"
#include "settings.h"
#include "dlgpagegeneral.h"
#include "dlgpagerules.h"
#include "dlgoptions.h"

static HWND hWndPageGeneral;
static HWND hWndPageRules;

INT_PTR CALLBACK DlgOptionsProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			TC_ITEM TI = {};
			RECT RCC = {};
			POINT PTLT = {};
			WCHAR lpTitle[64] = {};

			MoveToCenter(hWnd, 0, 0);

			swprintf(lpTitle, L"%s options", APP_NAME);
			SetWindowText(hWnd, lpTitle);

			SendMessage(hWnd, WM_SETICON, ICON_BIG,
				(LPARAM)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 32, 32, 0));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL,
				(LPARAM)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 16, 16, 0));

			TI.mask = TCIF_TEXT;
			TI.pszText = L"General";
			TabCtrl_InsertItem(GetDlgItem(hWnd, IDC_TABOPT), 0, &TI);
			TI.pszText = L"Rules";
			TabCtrl_InsertItem(GetDlgItem(hWnd, IDC_TABOPT), 1, &TI);

			hWndPageGeneral = CreateDialogParam(hAppInstance, MAKEINTRESOURCE(IDD_DLGPAGEGENERAL), hWnd,
				DlgPageGeneralProc, 0);
			hWndPageRules = CreateDialogParam(hAppInstance, MAKEINTRESOURCE(IDD_DLGPAGERULES), hWnd,
				DlgPageRulesProc, 0);

			GetWindowRect(GetDlgItem(hWnd, IDC_TABOPT), &RCC);
			TabCtrl_AdjustRect(GetDlgItem(hWnd, IDC_TABOPT), FALSE, &RCC);
			PTLT.x = RCC.left;
			PTLT.y = RCC.top;
			ScreenToClient(hWnd, &PTLT);

			MoveWindow(hWndPageGeneral, PTLT.x, PTLT.y, RCC.right - RCC.left,
				RCC.bottom - RCC.top, FALSE);
			MoveWindow(hWndPageRules, PTLT.x, PTLT.y, RCC.right - RCC.left,
				RCC.bottom - RCC.top, FALSE);

			if ((OSVI.dwMajorVersion > 5) || ((OSVI.dwMajorVersion == 5) &&
				(OSVI.dwMinorVersion >= 1)))
			{
				EnableThemeDialogTexture(hWndPageGeneral, ETDT_ENABLETAB);
				EnableThemeDialogTexture(hWndPageRules, ETDT_ENABLETAB);
			}

			ShowWindow(hWndPageGeneral, SW_SHOW);

			hWndOptions = hWnd;
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
						{
							DWORD dwHKCtrlMask = 0, dwHKAltMask = 0, dwHKShiftMask = 0, dwHKWinMask = 0;

							if (SendDlgItemMessage(hWndPageGeneral, IDC_OPTTGT1, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								Settings.m_dwTarget = 0;
							}
							else {
								Settings.m_dwTarget = 1;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKWGW, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								Settings.m_dwWindowGhostingWorkaround = 1;
							}
							else {
								Settings.m_dwWindowGhostingWorkaround = 0;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKTPTO, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								Settings.m_dwTPTimeout = 1;
							}
							else {
								Settings.m_dwTPTimeout = 0;
							}
							Settings.m_dwTPTimeoutValue = LOWORD(SendDlgItemMessage(hWndPageGeneral, IDC_SPNTO, UDM_GETPOS, 0, 0));
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKAR, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								Settings.m_dwAutorun = 1;
							}
							else {
								Settings.m_dwAutorun = 0;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKCTRL, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								dwHKCtrlMask = MOD_CONTROL;
							}
							else {
								dwHKCtrlMask = 0;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKALT, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								dwHKAltMask = MOD_ALT;
							}
							else {
								dwHKAltMask = 0;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKSFT, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								dwHKShiftMask = MOD_SHIFT;
							}
							else {
								dwHKShiftMask = 0;
							}
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKWIN, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								dwHKWinMask = MOD_WIN;
							}
							else {
								dwHKWinMask = 0;
							}
							if (dwHKCtrlMask == 0 && dwHKAltMask == 0 && dwHKShiftMask == 0 && dwHKWinMask == 0) {
								if (MessageBox(hWnd, L"Warning! No modifiers selected. Are you sure you want to use this hotkey?",
									APP_NAME, MB_YESNO | MB_DEFBUTTON1 | MB_ICONEXCLAMATION) == IDNO) break;
							}
							Settings.m_dwModifiers = dwHKCtrlMask | dwHKAltMask | dwHKShiftMask | dwHKWinMask;
							Settings.m_dwHotkey = (DWORD)GetWindowLongPtr(GetDlgItem(hWndPageGeneral, IDC_EDTKEY), GWLP_USERDATA);
							if (SendDlgItemMessage(hWndPageGeneral, IDC_CHKROH, BM_GETCHECK, TRUE, 0) == BST_CHECKED) {
								Settings.m_dwRebootOnHotkey = 1;
							}
							else {
								Settings.m_dwRebootOnHotkey = 0;
							}

							Settings.m_EkRules.clear();

							UINT uLVRItemsCnt = ListView_GetItemCount(GetDlgItem(hWndPageRules, IDC_LVRULES));
							WCHAR lpProcName[MAX_PATH];
							LV_ITEM LVI = {};
							EKRULE EKR;

							for (UINT i = 0; i < uLVRItemsCnt; i++) {
								ListView_GetItemText(GetDlgItem(hWndPageRules, IDC_LVRULES), i, 0, lpProcName, MAX_PATH);
								LVI.mask = LVIF_PARAM;
								LVI.iItem = i;
								ListView_GetItem(GetDlgItem(hWndPageRules, IDC_LVRULES), &LVI);

								wcscpy(EKR.lpProcName, lpProcName);
								EKR.dwRuleIdx = (DWORD)LVI.lParam;
								Settings.m_EkRules.push_back(EKR);
							}

							EndDialog(hWnd, 1);
							break;
						}
						case IDC_BTNCANCEL:
							EndDialog(hWnd, 0);
							break;
					}
					break;
			}
			return TRUE;
		case WM_NOTIFY:
			switch (LOWORD(wParam))
			{
				case IDC_TABOPT:
				{
					LPNMHDR pNM = (LPNMHDR)lParam;
					switch (pNM->code)
					{
						case TCN_SELCHANGING:
							//
							break;
						case TCN_SELCHANGE:
							ShowWindow(hWndPageGeneral, SW_HIDE);
							ShowWindow(hWndPageRules, SW_HIDE);
							switch (TabCtrl_GetCurSel(pNM->hwndFrom))
							{
								case 0:
									ShowWindow(hWndPageGeneral, SW_SHOW);
									break;
								case 1:
									ShowWindow(hWndPageRules, SW_SHOW);
									break;
							}
							break;
					}
					break;
				}
			}
			return TRUE;
		case WM_DESTROY:
			//DestroyWindow(hWndPageGeneral);
			//DestroyWindow(hWndPageRules);

			hWndOptions = NULL;
			return TRUE;
	}
	return FALSE;
}
