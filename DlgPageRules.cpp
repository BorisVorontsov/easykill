#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tlhelp32.h>

#include "main.h"
#include "resource.h"
#include "tooltips.h"
#include "settings.h"
#include "dlgpagerules.h"

static CToolTips ToolTips;

INT_PTR CALLBACK DlgPageRulesProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			LV_COLUMN LVC = {};
			COMBOBOXINFO CBI = {};

			PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTNRFPROCNAMES, BN_CLICKED), 0);

			ToolTips.m_hInstance = hAppInstance/*GetModuleHandle(NULL)*/;
			ToolTips.m_hOwner = hWnd;
			ToolTips.Initialize();

			CBI.cbSize = sizeof(CBI);
			GetComboBoxInfo(GetDlgItem(hWnd, IDC_CBPROCNAME), &CBI);
			ToolTips.AddToolTip(CBI.hwndItem, L"Acceptable wildcard characters: ? — any one character, * — zero or more characters");

			LVC.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
			LVC.pszText = L"Process name";
			LVC.fmt = LVCFMT_LEFT;
			LVC.cx = 185;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_LVRULES), 0, &LVC);
			LVC.pszText = L"Rule";
			LVC.fmt = LVCFMT_LEFT;
			LVC.cx = 135;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_LVRULES), 1, &LVC);

			ListView_SetExtendedListViewStyle(GetDlgItem(hWnd, IDC_LVRULES), LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_ONECLICKACTIVATE |
				LVS_EX_DOUBLEBUFFER);

			//Based on ProcRules enum
			ComboBox_AddString(GetDlgItem(hWnd, IDC_CBRULES), L"Terminate always");
			ComboBox_AddString(GetDlgItem(hWnd, IDC_CBRULES), L"Interpret as \"crit. proc.\"");
			ComboBox_AddString(GetDlgItem(hWnd, IDC_CBRULES), L"Ignore at all");
			ComboBox_SetCurSel(GetDlgItem(hWnd, IDC_CBRULES), 0);

			if (!Settings.m_EkRules.empty())
			{
				LV_ITEM LVI = {};
				DWORD dwItemIdx = 0, dwRuleSize;
				LPWSTR lpRule;

				for (vector<EKRULE>::iterator i = Settings.m_EkRules.begin(); i < Settings.m_EkRules.end(); i++)
				{
					LVI.mask = LVIF_TEXT;
					LVI.iItem = dwItemIdx;
					LVI.pszText = i->lpProcName;
					ListView_InsertItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);

					dwRuleSize = ComboBox_GetLBTextLen(GetDlgItem(hWnd, IDC_CBRULES), i->dwRuleIdx);
					lpRule = new WCHAR[dwRuleSize + 1];
					ComboBox_GetLBText(GetDlgItem(hWnd, IDC_CBRULES), i->dwRuleIdx, lpRule);
					ListView_SetItemText(GetDlgItem(hWnd, IDC_LVRULES), dwItemIdx, 1, lpRule);
					delete[] lpRule;

					LVI.mask = LVIF_PARAM;
					LVI.lParam = i->dwRuleIdx;
					ListView_SetItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);

					dwItemIdx++;
				}
			}

			SetTimer(hWnd, PR_TMR1, 250, NULL);
			return TRUE;
		}
		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
					switch (LOWORD(wParam))
					{
						case IDC_BTNRFPROCNAMES:
							HANDLE hSnapshot;
							PROCESSENTRY32 PE;
							UINT nCBPNItemsCnt;
							WCHAR lpCBPNItem[MAX_PATH], lpProcNameLwr[MAX_PATH], lpMyName[sizeof(APP_NAME)];
							bool bProcNameFound;

							hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
							if (hSnapshot != INVALID_HANDLE_VALUE)
							{
								PE.dwSize = sizeof(PROCESSENTRY32);
								Process32First(hSnapshot, &PE);
								do {
									bProcNameFound = false;
									nCBPNItemsCnt = ComboBox_GetCount(GetDlgItem(hWnd, IDC_CBPROCNAME));
									for (UINT i = 0; i < nCBPNItemsCnt; i++) {
										ComboBox_GetLBText(GetDlgItem(hWnd, IDC_CBPROCNAME), i, lpCBPNItem);
										if (_wcsicmp(lpCBPNItem, PE.szExeFile) == 0) {
											bProcNameFound = true;
											break;
										}
									}
									wcscpy(lpProcNameLwr, PE.szExeFile);
									_wcslwr(lpProcNameLwr);
									wcscpy(lpMyName, APP_NAME);
									_wcslwr(lpMyName);
									if (!bProcNameFound && (wcsstr(lpProcNameLwr, lpMyName) == NULL) && (PE.th32ProcessID != 0))
										ComboBox_AddString(GetDlgItem(hWnd, IDC_CBPROCNAME), PE.szExeFile);
								} while (Process32Next(hSnapshot, &PE));
							}
							CloseHandle(hSnapshot);
							break;
						case IDC_BTNADDRULE:
						{
							WCHAR lpBuff1[MAX_PATH], lpBuff2[MAX_PATH], lpMyName[sizeof(APP_NAME)];
							LV_ITEM LVI = {};
							UINT uLVRItemsCnt;
							bool bRuleFound = false;

							ComboBox_GetText(GetDlgItem(hWnd, IDC_CBPROCNAME), lpBuff1, MAX_PATH);

							wcscpy(lpBuff2, lpBuff1);
							_wcslwr(lpBuff2);
							wcscpy(lpMyName, APP_NAME);
							_wcslwr(lpMyName);
							if ((wcslen(lpBuff1) == 0) || (wcsstr(lpBuff2, lpMyName) != NULL)) {
								MessageBeep(MB_ICONEXCLAMATION);
								break;
							}

							uLVRItemsCnt = ListView_GetItemCount(GetDlgItem(hWnd, IDC_LVRULES));

							for (UINT i = 0; i < uLVRItemsCnt; i++) {
								ListView_GetItemText(GetDlgItem(hWnd, IDC_LVRULES), i, 0, lpBuff2, MAX_PATH);
								if (_wcsicmp(lpBuff1, lpBuff2) == 0) {
									ListView_SetItemState(GetDlgItem(hWnd, IDC_LVRULES), i, LVIS_SELECTED, 0x0000000F);
									bRuleFound = true;
									break;
								}
							}

							if (bRuleFound) {
								MessageBeep(MB_ICONEXCLAMATION);
								break;
							}

							LVI.mask = LVIF_TEXT;
							LVI.iItem = uLVRItemsCnt;
							LVI.pszText = lpBuff1;
							ListView_InsertItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);
							ComboBox_GetText(GetDlgItem(hWnd, IDC_CBRULES), lpBuff1, MAX_PATH);
							ListView_SetItemText(GetDlgItem(hWnd, IDC_LVRULES), uLVRItemsCnt, 1, lpBuff1);
							LVI.mask = LVIF_PARAM;
							LVI.iItem = uLVRItemsCnt;
							LVI.lParam = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_CBRULES));
							ListView_SetItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);

							break;
						}
						case IDC_BTNMODRULE:
						{
							WCHAR lpBuff[MAX_PATH];
							LV_ITEM LVI = {};
							UINT uLVRSelItem, uCBRCurSel;

							uLVRSelItem = ListView_GetNextItem(GetDlgItem(hWnd, IDC_LVRULES), -1, LVNI_SELECTED);
							uCBRCurSel = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_CBRULES));

							ComboBox_GetText(GetDlgItem(hWnd, IDC_CBPROCNAME), lpBuff, MAX_PATH);
							ListView_SetItemText(GetDlgItem(hWnd, IDC_LVRULES), uLVRSelItem, 0, lpBuff);
							ComboBox_GetText(GetDlgItem(hWnd, IDC_CBRULES), lpBuff, MAX_PATH);
							ListView_SetItemText(GetDlgItem(hWnd, IDC_LVRULES), uLVRSelItem, 1, lpBuff);
							LVI.mask = LVIF_PARAM;
							LVI.iItem = uLVRSelItem;
							LVI.lParam = uCBRCurSel;
							ListView_SetItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);
							break;
						}
						case IDC_BTNREMRULE:
							UINT uLVRSelItem;

							uLVRSelItem = ListView_GetNextItem(GetDlgItem(hWnd, IDC_LVRULES), -1, LVNI_SELECTED);
							ListView_DeleteItem(GetDlgItem(hWnd, IDC_LVRULES), uLVRSelItem);
							break;
						default:
							//
							break;
					}
					break;
				default:
					//
					break;
			}
			return TRUE;
		case WM_NOTIFY:
		{
			LPNMHDR pNM = (LPNMHDR)lParam;
			switch (wParam)
			{
				case IDC_LVRULES:
					switch (pNM->code)
					{
						case LVN_ITEMACTIVATE:
						{
							LPNMITEMACTIVATE pNMIA = (LPNMITEMACTIVATE)lParam;

							LV_ITEM LVI = {};
							WCHAR lpProcName[MAX_PATH];

							ListView_GetItemText(GetDlgItem(hWnd, IDC_LVRULES), pNMIA->iItem, 0, lpProcName, MAX_PATH);
							ComboBox_SetText(GetDlgItem(hWnd, IDC_CBPROCNAME), lpProcName);
							LVI.mask = LVIF_PARAM;
							LVI.iItem = pNMIA->iItem;
							ListView_GetItem(GetDlgItem(hWnd, IDC_LVRULES), &LVI);
							ComboBox_SetCurSel(GetDlgItem(hWnd, IDC_CBRULES), LVI.lParam);
							break;
						}
						default:
							//
							break;
					}
					break;
				default:
					//
					break;
			}
			return TRUE;
		}
		case WM_TIMER:
			if (wParam == PR_TMR1)
			{
				UINT uLVRSelItem = ListView_GetNextItem(GetDlgItem(hWnd, IDC_LVRULES), -1, LVNI_SELECTED);
				
				if (uLVRSelItem != -1)
				{
					if (!IsWindowEnabled(GetDlgItem(hWnd, IDC_BTNMODRULE)))
						EnableWindow(GetDlgItem(hWnd, IDC_BTNMODRULE), TRUE);

					if (!IsWindowEnabled(GetDlgItem(hWnd, IDC_BTNREMRULE)))
						EnableWindow(GetDlgItem(hWnd, IDC_BTNREMRULE), TRUE);
				}
				else
				{
					if (IsWindowEnabled(GetDlgItem(hWnd, IDC_BTNMODRULE)))
						EnableWindow(GetDlgItem(hWnd, IDC_BTNMODRULE), FALSE);

					if (IsWindowEnabled(GetDlgItem(hWnd, IDC_BTNREMRULE)))
						EnableWindow(GetDlgItem(hWnd, IDC_BTNREMRULE), FALSE);
				}
				
			}

			return TRUE;
		case WM_DESTROY:
			KillTimer(hWnd, PR_TMR1);
			ToolTips.Destroy();
			return TRUE;
	}

	return FALSE;
}