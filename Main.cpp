////////////////////////////////////////////////////////////////////////////
//		EasyKill (x86/64)
//		© Boris Vorontsov aka BV (borisvorontsov@gmail.com), 2007 - 2017
////////////////////////////////////////////////////////////////////////////

//System requirements: Windows NT 5.0 (5.1 or higher for XP/Aero themes support)

#define _WIN32_WINNT	0x0501

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>

#include "resource.h"
#include "settings.h"
#include "dlgoptions.h"
#include "dlgabout.h"
#include "main.h"

OSVERSIONINFO OSVI = {};
HANDLE hMutex;
HINSTANCE hAppInstance;
WCHAR lpAppPath[MAX_PATH] = {};
WCHAR lpAppPathNoQts[MAX_PATH] = {};
WCHAR lpAppVersion[16] = {};

HWND hWndOptions = NULL;
HWND hWndAbout = NULL;

#ifdef _WIN64
static WCHAR lpEKSFileName[MAX_PATH] = {};
#endif
static UINT WM_TASKBARCREATED;
static NOTIFYICONDATA NID = {};
static HWND hWndCallback;

static HMENU hTrayMenu;
static MSG Message;

#ifndef _WIN64
//x86
static BOOL (__stdcall * EK_InstallHook)();
static BOOL (__stdcall * EK_RemoveHook)();
#else
//x64
static BOOL (__stdcall * EK_InstallHook_x64)();
static BOOL (__stdcall * EK_RemoveHook_x64)();
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
	__try
	{
#endif
		OSVI.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&OSVI);

		hMutex = CreateMutex(0, TRUE, APP_NAME);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			ReleaseMutex(hMutex);
			CloseHandle(hMutex);
			return 0;
		}

		hAppInstance = hInstance;

		LoadAdditionalFunctions();

		InitCommonControls();

		GetAppPath(lpAppPath, MAX_PATH, TRUE);
		GetAppPath(lpAppPathNoQts, MAX_PATH);
		ReadFileVersion(lpAppPathNoQts, lpAppVersion);

		Settings.GetSettings();

		if (ReadCommandLine(lpCmdLine)) return 0;

#ifdef _WIN64
		wcscpy(lpEKSFileName, lpAppPathNoQts);
		*(wcsrchr(lpEKSFileName, L'\\') + 1) = L'\0';
		wcscat(lpEKSFileName, L"EKSurrogatex86.exe");
		RunSurrogatex86();
#endif

		if (Settings.m_dwWindowGhostingWorkaround)
		{
#ifndef _WIN64
			_SC(EK_InstallHook);
#else
			PostMessageToSurrogatex86(EKSM_INSTALL_X86_HOOK);
			_SC(EK_InstallHook_x64);
#endif
		}

		AdjustPrivilege(L"SeShutdownPrivilege");
		AdjustPrivilege(L"SeDebugPrivilege");

		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

		WNDCLASSEX ClbkWnd = {};

		ClbkWnd.cbSize = sizeof(WNDCLASSEX);
		ClbkWnd.hCursor = LoadCursor(0, (LPWSTR)IDC_ARROW);
		ClbkWnd.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON));
		ClbkWnd.lpfnWndProc = ClbkWndProc;
		ClbkWnd.hInstance = hAppInstance;
		ClbkWnd.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		ClbkWnd.lpszClassName = L"EasyKill_CallbackWindow";
		ClbkWnd.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;

		RegisterClassEx(&ClbkWnd);

		WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

		hWndCallback = CreateWindowEx(0, ClbkWnd.lpszClassName, 0, WS_SYSMENU, 64, 64, 128, 128, 0, 0, hAppInstance, 0);

		hTrayMenu = CreatePopupMenu();
		AppendMenu(hTrayMenu, MF_STRING, IDM_ABOUT, L"&About");
		AppendMenu(hTrayMenu, MF_SEPARATOR, IDM_SEP1, 0);
		AppendMenu(hTrayMenu, MF_STRING, IDM_OPT, L"&Options...");
		AppendMenu(hTrayMenu, MF_SEPARATOR, IDM_SEP2, 0);
		AppendMenu(hTrayMenu, MF_STRING, IDM_EXIT, L"&Exit");
		SetMenuDefaultItem(hTrayMenu, IDM_ABOUT, 0);

		NID.cbSize = sizeof(NOTIFYICONDATA);
		NID.hIcon = (HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 16, 16, 0);
		NID.hWnd = hWndCallback;
		NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		NID.uCallbackMessage = WM_NICALLBACK;
		NID.uID = NID_ID;

		Shell_NotifyIcon(NIM_ADD, &NID);

		Hotkey(false);

		if (!IsUserAdmin())
			ShowBalloon(APP_NAME, L"You log on using a limited user account. Program may not work correctly.", BI_WARNING);

		while (GetMessage(&Message, 0, 0, 0))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
#ifndef _DEBUG
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		WCHAR lpMsg[512] = {};
		swprintf(lpMsg, L"Unexpected error: 0x%08x", GetExceptionCode());
		MessageBox(0, lpMsg, APP_NAME, MB_ICONSTOP);
	}
#endif

	Shell_NotifyIcon(NIM_DELETE, &NID);

	Hotkey(true);
#ifndef _WIN64
	_SC(EK_RemoveHook);
#else
	PostMessageToSurrogatex86(EKSM_REMOVE_X86_HOOK);
	PostMessageToSurrogatex86(EKSM_EXIT);
	if (PathFileExists(lpEKSFileName)) {
		while (!DeleteFile(lpEKSFileName)){};
	}
	_SC(EK_RemoveHook_x64);
#endif

	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	return (int)Message.wParam;
}

LRESULT CALLBACK ClbkWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_TASKBARCREATED)
	{
		Shell_NotifyIcon(NIM_ADD, &NID);
		return 0;
	}
	switch(Message)
	{
		case WM_NICALLBACK:
			switch(lParam)
			{
				case WM_LBUTTONDBLCLK:
					ShowAbout();
					break;
				case WM_RBUTTONUP:
					SetForegroundWindow(hWnd);
					POINT pt;
					GetCursorPos(&pt);
					TrackPopupMenu(hTrayMenu, 0, pt.x, pt.y, 0, hWnd, 0);
					PostMessage(hWnd, WM_NULL, 0, 0);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDM_ABOUT:
					ShowAbout();
					break;
				case IDM_OPT:
					ShowOptions();
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
			}
			break;
		case WM_HOTKEY:
			ExtendedKillProcess();
			CheckAndKillBkgndProcesses();

			if (Settings.m_dwRebootOnHotkey)
				ExitWindowsEx(EWX_FORCE | EWX_REBOOT, 0);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);
	}
	return 0;
}

void AdjustPrivilege(LPWSTR lpPrivilege)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES TKP = {};
	TOKEN_PRIVILEGES TKPOLD = {};
	DWORD dwRetLen;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		LookupPrivilegeValue(0, lpPrivilege, &TKP.Privileges[0].Luid);
		TKP.PrivilegeCount = 1;
		TKP.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, 0, &TKP, sizeof(TOKEN_PRIVILEGES), &TKPOLD, &dwRetLen);
	}
}

void Hotkey(BOOL bUnregister)
{
	if (bUnregister)
	{
		UnregisterHotKey(hWndCallback, RHK_ID);
	}
	else
	{
		WCHAR lpKey[16] = {};
		WCHAR lpModifiers[24] = {};
		WCHAR lpTip[64] = {};
		GetKeyName(Settings.m_dwHotkey, lpKey, sizeof(lpKey) / sizeof(WCHAR));
		wcscpy(lpTip, L"EasyKill ");
		wcscat(lpTip, lpAppVersion);
		wcscat(lpTip, L"\nHotkey: ");
		if ((Settings.m_dwModifiers & MOD_CONTROL) == MOD_CONTROL) wcscat(lpModifiers, L"Ctrl+");
		if ((Settings.m_dwModifiers & MOD_ALT) == MOD_ALT) wcscat(lpModifiers, L"Alt+");
		if ((Settings.m_dwModifiers & MOD_SHIFT) == MOD_SHIFT) wcscat(lpModifiers, L"Shift+");
		if ((Settings.m_dwModifiers & MOD_WIN) == MOD_WIN) wcscat(lpModifiers, L"Win+");
		wcscat(lpTip, lpModifiers);
		wcscat(lpTip, lpKey);
		wcscpy(NID.szTip, lpTip);

		if (NID.uFlags == NIF_INFO)
			NID.uTimeout = 0;
		NID.uFlags = NIF_TIP;

		Shell_NotifyIcon(NIM_MODIFY, &NID);

		if (RegisterHotKey(hWndCallback, RHK_ID, Settings.m_dwModifiers, Settings.m_dwHotkey) == 0)
		{
			WCHAR lpMsg[255] = {};
			wcscpy(lpMsg, L"Unable to register '");
			wcscat(lpMsg, lpModifiers);
			wcscat(lpMsg, lpKey);
			wcscat(lpMsg, L"' hotkey.");
			ShowBalloon(APP_NAME, lpMsg, BI_WARNING);
		}
	}
}

void ShowAbout()
{
	if (!hWndAbout) {
		DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DLGABOUT), 0, DlgAboutProc);
	}
	else {
		SetForegroundWindow(hWndAbout);
	}
}

void ShowOptions()
{
	if (!hWndOptions)
	{
		if (DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DLGOPTIONS), 0,
			DlgOptionsProc) != 0)
		{
			Settings.SaveSettings();
			if (Settings.m_dwWindowGhostingWorkaround)
			{
#ifndef _WIN64
				_SC(EK_InstallHook);
#else
				PostMessageToSurrogatex86(EKSM_INSTALL_X86_HOOK);
				_SC(EK_InstallHook_x64);
#endif
			}
			else
			{
#ifndef _WIN64
				_SC(EK_RemoveHook);
#else
				PostMessageToSurrogatex86(EKSM_REMOVE_X86_HOOK);
				_SC(EK_RemoveHook_x64);
#endif
			}
			if (Settings.m_dwAutorun)
			{
				AddToAutorun();
			}
			else
			{
				RemoveFromAutorun();
			}
			Hotkey(true);
			Hotkey(false);
		}
	}
	else
	{
		SetForegroundWindow(hWndOptions);
	}
}

void AddToAutorun()
{
	HKEY hAR;
	RegOpenKeyEx(HKEY_CURRENT_USER, EK_REG_AUTORUN_PATH, 0, KEY_ALL_ACCESS, &hAR);
	RegSetValueEx(hAR, APP_NAME, 0, REG_SZ, (CONST BYTE *)lpAppPath, MAX_PATH);
	RegCloseKey(hAR);
}

void RemoveFromAutorun()
{
	HKEY hAR;
	RegOpenKeyEx(HKEY_CURRENT_USER, EK_REG_AUTORUN_PATH, 0, KEY_ALL_ACCESS, &hAR);
	RegDeleteValue(hAR, APP_NAME);
	RegCloseKey(hAR);
}

BOOL IsUserAdmin()
{
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	BOOL bResult = FALSE;
	AllocateAndInitializeSid(&siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators);
	CheckTokenMembership(0, psidAdministrators, &bResult);
	FreeSid(psidAdministrators);
	return bResult;
}

BOOL ReadCommandLine(LPWSTR lpCmdLine)
{
	LPWSTR *ppCmdLineArgs = NULL;
	UINT nArgsCnt = 0;
	ppCmdLineArgs = CommandLineToArgvW(lpCmdLine, (int *)&nArgsCnt);
	BOOL bResult = FALSE;

	if (ppCmdLineArgs)
	{
		for (UINT i = 0; i < nArgsCnt; i++)
		{
			if (wcsicmp(ppCmdLineArgs[i], EK_CMD_TERMINATE) == 0)
			{
				if (i == (nArgsCnt - 1)) {
					ExtendedKillProcess();
				}
				else {
					ExtendedKillProcess(ppCmdLineArgs[i + 1]);
				}

				bResult = TRUE;
			}
		}
	}

	LocalFree(ppCmdLineArgs);

	return bResult;
}

BOOL GetAppPath(LPWSTR lpPath, size_t szPathLen, BOOL bAddQuotes)
{
	LPWSTR lpBuff = new WCHAR[szPathLen];

	if (GetModuleFileName(GetModuleHandle(NULL), lpBuff, (DWORD)szPathLen) == 0)
	{
		delete[] lpBuff;
		return FALSE;
	}

	if (wcschr(lpBuff, L' ') && bAddQuotes) {
		swprintf(lpPath, L"\"%s\"", lpBuff);
	}
	else {
		wcscpy(lpPath, lpBuff);
	}

	delete[] lpBuff;
	return TRUE;
}

void GetProcessName(DWORD dwPID, LPWSTR lpResult)
{
	HANDLE hProcess;
	DWORD cbNeeded;
	HMODULE hModule;
	WCHAR lpProcessName[MAX_PATH] = {};

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
	if (hProcess != 0)
	{
		if(EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded))
		{
			GetModuleFileNameEx(hProcess, hModule, lpProcessName, MAX_PATH);
		}
	}
	CloseHandle(hProcess);
	wcscpy(lpResult, lpProcessName);
}

BOOL IsCriticalProcess(LPWSTR lpProcessName)
{
	GetName(lpProcessName);

	if (wcsicmp(lpProcessName, L"csrss.exe") == 0) return TRUE;
	if (wcsicmp(lpProcessName, L"winlogon.exe") == 0) return TRUE;

	SC_HANDLE hSCM;
	DWORD dwBuffSize;
	LPENUM_SERVICE_STATUS ESS = NULL;
	SC_HANDLE hService;
	LPWSTR lpServiceFileName = new WCHAR[MAX_PATH];
	LPQUERY_SERVICE_CONFIG QSC = NULL;
	DWORD dwBytesNeeded = 0, dwServicesReturned = 0, dwResumeHandle = 0;
	BOOL bResult = FALSE;

	GetTitle(lpProcessName);

	hSCM = OpenSCManager(0, 0, SC_MANAGER_ENUMERATE_SERVICE);

	EnumServicesStatus(hSCM, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0, &dwBytesNeeded, &dwServicesReturned,
		&dwResumeHandle);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		dwBuffSize = dwBytesNeeded;
		ESS = (LPENUM_SERVICE_STATUS)LocalAlloc(LMEM_FIXED, dwBytesNeeded);
		EnumServicesStatus(hSCM, SERVICE_WIN32, SERVICE_ACTIVE, ESS, dwBuffSize, &dwBytesNeeded, &dwServicesReturned,
			&dwResumeHandle);
	}

	CloseServiceHandle(hSCM);

	hSCM = OpenSCManager(0, 0, SC_MANAGER_QUERY_LOCK_STATUS);

	for (DWORD i = 0; i < dwServicesReturned; i++)
	{
		hService = OpenService(hSCM, ESS[i].lpServiceName, SERVICE_QUERY_CONFIG);
		QueryServiceConfig(hService, NULL, 0, &dwBytesNeeded);

		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwBuffSize = dwBytesNeeded;
			QSC = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, dwBuffSize);
			QueryServiceConfig(hService, QSC, dwBuffSize, &dwBytesNeeded);
		}
		wcscpy(lpServiceFileName, QSC->lpBinaryPathName);
		LocalFree(QSC);
		GetName(lpServiceFileName);
		GetTitle(lpServiceFileName);
		if (wcsicmp(lpServiceFileName, lpProcessName) == 0)
		{
			CloseServiceHandle(hService);
			bResult = TRUE;
			break;
		}
		CloseServiceHandle(hService);
	}

	CloseServiceHandle(hSCM);

	LocalFree(ESS);
	delete[] lpServiceFileName;
	return bResult;
}

//IsHungAppWindow alternative
BOOL IsHungWindow(HWND hWnd, DWORD dwTimeout)
{
	DWORD_PTR dwResult;

	if (SendMessageTimeout(hWnd, WM_PAINT, 0, 0, SMTO_NORMAL, dwTimeout, &dwResult) == 0)
	{
		DWORD dwErr = GetLastError();
		if ((dwErr == 0) || (dwErr == ERROR_TIMEOUT))
			return TRUE;
	}

	return FALSE;
}

size_t GetName(LPWSTR lpPath)
{
	size_t szPathSize = wcslen(lpPath);
	if (szPathSize <= 1) return 0;
	if (wcschr(lpPath, '\\') == 0) return 0;
	LPWSTR lpName = lpPath;
	lpName = wcsrchr(lpPath, '\\');
	lpName++;
	wcscpy(lpPath, lpName);
	return wcslen(lpPath);
}

size_t GetTitle(LPWSTR lpFileName)
{
	size_t szFNSize = wcslen(lpFileName);
	if (szFNSize <= 1) return 0;

	if (wcschr(lpFileName, '.') == 0) return 0;
	LPWSTR lpTitle = lpFileName;
	lpTitle = wcsrchr(lpTitle, '.');
	lpFileName[lpTitle - lpFileName] = '\0';

	return wcslen(lpFileName);
}

size_t TrimEx(LPCWSTR lpString, LPWSTR lpResult, WCHAR ch)
{
	size_t szStrSize = wcslen(lpString);
	if (szStrSize <= 1) return 0;

	LPWSTR lpOrigTemp = new TCHAR[szStrSize + 1];
	memset(lpOrigTemp, 0, (szStrSize + 1) * sizeof(WCHAR));
	LPWSTR lpTemp = lpOrigTemp;
	wcscpy(lpTemp, lpString);
	size_t i = 0;
	for (; lpTemp[0] == ch && i < szStrSize; i++)
		lpTemp++;
	for (i = (wcslen(lpTemp) - 1); lpTemp[i] == ch && i > 0; i--)
		lpTemp[i] = L'\0';
	wcscpy(lpResult, lpTemp);
	delete[] lpOrigTemp;

	return wcslen(lpResult);
}

int _wcswildicmp(LPCWSTR lpWildStr, LPCWSTR lpStrToComp)
{
	int intResult;
	LPWSTR lpWildStrLocal, lpStrToCompLocal;
	LPWSTR pB1, pB2;
	
	lpWildStrLocal = pB1 = new WCHAR[wcslen(lpWildStr) + 1];
	wcscpy(lpWildStrLocal, lpWildStr);
	_wcslwr(lpWildStrLocal);

	lpStrToCompLocal = pB2 = new WCHAR[wcslen(lpStrToComp) + 1];
	wcscpy(lpStrToCompLocal, lpStrToComp);
	_wcslwr(lpStrToCompLocal);

	while ((*lpStrToCompLocal) && (*lpWildStrLocal != L'*')) {
		if ((*lpWildStrLocal != *lpStrToCompLocal) && (*lpWildStrLocal != L'?')) {
			intResult = -1;
			goto _wcswildicmp_Exit;
		}

		lpWildStrLocal++;
		lpStrToCompLocal++;
	}

	while (*lpStrToCompLocal)
	{
		if (*lpWildStrLocal == L'*')
		{
			if (*++lpWildStrLocal == L'\0') {
				intResult = 0;
				goto _wcswildicmp_Exit;
			}
		}
		else if ((*lpWildStrLocal == *lpStrToCompLocal) || (*lpWildStrLocal == L'?'))
		{
			lpWildStrLocal++;
			lpStrToCompLocal++;
		}
		else
			lpStrToCompLocal++;
	}

	while (*lpWildStrLocal == L'*')
		lpWildStrLocal++;
	intResult = (*lpWildStrLocal == L'\0')?0:1;

_wcswildicmp_Exit:

	delete[] pB1;
	delete[] pB2;

	return intResult;
}

void MoveToCenter(HWND hWnd, LONG lXOffset, LONG lYOffset)
{
	RECT RC = {};
	int intSX = 0, intSY = 0;
	int intNL = 0, intNT = 0;

	GetWindowRect(hWnd, &RC); 
	intSX = GetSystemMetrics(SM_CXSCREEN);
	intSY = GetSystemMetrics(SM_CYSCREEN);
	intNL = ((intSX / 2) - ((RC.right - RC.left) / 2)) + lXOffset;
	intNT = ((intSY / 2) - ((RC.bottom - RC.top) / 2)) + lYOffset;
	SetWindowPos(hWnd, HWND_TOP, intNL, intNT, 0, 0, SWP_NOSIZE);
}

DWORD ReadFileVersion(LPWSTR lpFileName, LPWSTR lpResult)
{
	DWORD dwDummy;
	BYTE *bBuffer;
	VS_FIXEDFILEINFO *VS_BUFF;
	DWORD dwBufferLen;
	UINT uVerBufferLen;
	WCHAR lpTmp[16] = {};

	dwBufferLen = GetFileVersionInfoSize(lpFileName, &dwDummy);
	if (dwBufferLen < 1)
	{
		lpResult = L"Unknown";
		return 1;
	}

	bBuffer = new BYTE[dwBufferLen];
	GetFileVersionInfo(lpFileName, 0, dwBufferLen, &bBuffer[0]);
	VerQueryValue(&bBuffer[0], L"\\", (PVOID *)&VS_BUFF, &uVerBufferLen);

	swprintf(lpResult, L"%d.%d.%d.%d", HIWORD(VS_BUFF->dwProductVersionMS), LOWORD(VS_BUFF->dwProductVersionMS), HIWORD(VS_BUFF->dwProductVersionLS),
		LOWORD(VS_BUFF->dwProductVersionLS));
	delete[] bBuffer;

	return ERROR_SUCCESS;
}

DWORD GetKeyName(DWORD VK, LPWSTR lpKey, DWORD dwKeyNameLen)
{
	UINT SC;

	SC = MapVirtualKey(VK, 0);
	if (SC == 0)
	{
		if (dwKeyNameLen >= 4)
		{
			WCHAR lpVK[4] = {};
			_itow(VK, lpVK, 10);
			wcscpy(lpKey, L"#");
			wcscat(lpKey, lpVK);
			return 4;
		}
		else
		{
			return 0;
		}
	}
	SC <<= 16; //ScanCode, LOWORD -> HIWORD
	switch (VK)
	{
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 45:
		case 46:
		case 91:
		case 92:
		case 93:
		case 144:
			SC |= 0x1 << 24; //Extended-key flag
			break;
		default:
			//
			break;
	}
	SC |= 0x1 << 25; //"Don't care" bit

	return GetKeyNameTextW(SC, lpKey, dwKeyNameLen);
}

void CheckAndKillBkgndProcesses()
{
	HANDLE hSnapshot;
	PROCESSENTRY32 PE;
	HANDLE hProcess;
	const DWORD dwErrMsgSize = 384;
	vector<wstring> vErrors;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		PE.dwSize = sizeof(PROCESSENTRY32);
		Process32First(hSnapshot, &PE);
		do {
			for (vector<EKRULE>::iterator i = Settings.m_EkRules.begin(); i < Settings.m_EkRules.end(); i++) {
				if (_wcswildicmp(i->lpProcName, PE.szExeFile) == 0)
				{
					if (i->dwRuleIdx == prTerminateAlways)
					{
						if (PE.th32ProcessID && (PE.th32ProcessID != GetCurrentProcessId()))
						{
							hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PE.th32ProcessID);
							if (TerminateProcess(hProcess, 0) == 0)
							{
								wstring strMsg;
								strMsg.resize(dwErrMsgSize);
								swprintf((LPWSTR)strMsg.c_str(), L"Error terminating background process '%s' (0x%08x)\n", PE.szExeFile, GetLastError());
								vErrors.push_back(strMsg);
							}
							CloseHandle(hProcess);
						}
					}
				}
			}
		} while (Process32Next(hSnapshot, &PE));

		if (!vErrors.empty())
		{
			LPWSTR lpOut = new WCHAR[vErrors.size() * dwErrMsgSize];
			for (vector<wstring>::iterator i = vErrors.begin(); i < vErrors.end(); i++) {
				wcscat(lpOut, (LPCWSTR)i._Ptr);
			}
			ShowBalloon(APP_NAME, lpOut, BI_ERROR);
			delete[] lpOut;
		}
	}

	CloseHandle(hSnapshot);
}

BOOL ExtendedKillProcess(LPWSTR lpWndTitle)
{
	POINT pt;
	HWND hTarget;
	DWORD dwPID;
	HANDLE hProcess;
	WCHAR lpFullProcName[MAX_PATH] = {};
	WCHAR lpProcName[MAX_PATH] = {};
	bool bForceKill = false, bMarkedAsCritByUser = false;

	if (lpWndTitle)
	{
		hTarget = FindWindow(0, lpWndTitle);
	}
	else
	{
		if (Settings.m_dwTarget == 0)
		{
			hTarget = GetForegroundWindow();
		}
		else
		{
			GetCursorPos(&pt);
			hTarget = WindowFromPoint(pt);
		}
	}

	if (Settings.m_dwTPTimeout)
		if (!IsHungWindow(hTarget, Settings.m_dwTPTimeoutValue * 1000)) return FALSE;

	GetWindowThreadProcessId(hTarget, &dwPID);
	GetProcessName(dwPID, lpFullProcName);
	wcscpy(lpProcName, lpFullProcName);
	GetName(lpProcName);

	for (vector<EKRULE>::iterator i = Settings.m_EkRules.begin(); i < Settings.m_EkRules.end(); i++) {
		if (_wcswildicmp(i->lpProcName, lpProcName) == 0)
		{
			switch (i->dwRuleIdx)
			{
				case prTerminateAlways:
					bForceKill = true;
					break;
				case prInterpretAsCriticalProcess:
					bMarkedAsCritByUser = true;
					break;
				case prIgnoreAtAll:
					return FALSE;
			}
		}
	}

	if (!bForceKill)
	{
		if (IsCriticalProcess(lpFullProcName) || bMarkedAsCritByUser)
		{
			WCHAR lpMsg[384] = {};
			swprintf(lpMsg, L"'%s' is a critical system process. Are you sure you want to terminate it?", lpProcName);
			if (MessageBox(0, lpMsg, APP_NAME, MB_YESNO | MB_DEFBUTTON1 | MB_ICONEXCLAMATION) == IDNO) return FALSE;
		}
	}

	if (dwPID != GetCurrentProcessId())
	{
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPID);
		if (TerminateProcess(hProcess, 0) == 0)
		{
			WCHAR lpMsg[384] = {};
			swprintf(lpMsg, L"Unable to terminate '%s'.\nError: 0x%08x", lpProcName, GetLastError());
			ShowBalloon(APP_NAME, lpMsg, BI_ERROR);
		}
		CloseHandle(hProcess);
	}

	return TRUE;
}

#ifdef _WIN64

void RunSurrogatex86()
{
	HRSRC hRes;
	HGLOBAL hResLoad;
	LPVOID lpResLock;
	HANDLE hFile;
	DWORD dwResSize, dwWritten;
	STARTUPINFO SI = {};
	PROCESS_INFORMATION PI = {};

	hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_EKSURROGATEX86), L"RT_APP");
	hResLoad = LoadResource(NULL, hRes);
	lpResLock = LockResource(hResLoad);

	hFile = CreateFile(lpEKSFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
	dwResSize = SizeofResource(NULL, hRes);
	WriteFile(hFile, lpResLock, dwResSize, &dwWritten, NULL);
	CloseHandle(hFile);

	SI.cb = sizeof(SI);
	CreateProcess(NULL, lpEKSFileName, NULL, NULL, FALSE, 0, NULL, NULL, &SI, &PI);
	WaitForInputIdle(PI.hProcess, INFINITE);
}

BOOL PostMessageToSurrogatex86(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hEKSWnd = FindWindowEx(HWND_MESSAGE, NULL, L"EKS_MessageOnlyWindow", NULL);
	if (!hEKSWnd) return FALSE;
	PostMessage(hEKSWnd, uMsg, wParam, lParam);
	return TRUE;
}

#endif

/*void InjectLibrary(DWORD dwPID, LPWSTR lpLibName)
{
	HANDLE hProcess;
	HANDLE hThread;
	HMODULE hKernel32;
	PTHREAD_START_ROUTINE pfnThreadRtn;
	LPWSTR lpRemoteLibName;
	DWORD dwLibNameLen;

	hKernel32 = GetModuleHandle(L"kernel32");
	pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	dwLibNameLen = wcslen(lpLibName);
	lpRemoteLibName = (LPWSTR)VirtualAllocEx(hProcess, 0, dwLibNameLen, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(hProcess, lpRemoteLibName, (PVOID)lpLibName, dwLibNameLen , 0);
	hThread = CreateRemoteThread(hProcess, 0, 0, pfnThreadRtn, lpRemoteLibName, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, lpRemoteLibName, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
}*/

/*void EjectLibrary(DWORD dwPID, LPWSTR lpLibName)
{
	HANDLE hProcess;
	HANDLE hThread;
	HMODULE hKernel32;
	HANDLE hSnapshot;
	MODULEENTRY32 ME = {};
	PTHREAD_START_ROUTINE pfnThreadRtn;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	BOOL bFound = FALSE;
	ME.dwSize = sizeof(MODULEENTRY32);
	BOOL bMoreMods = Module32First(hSnapshot, &ME);
	for (; bMoreMods; bMoreMods = Module32Next(hSnapshot, &ME)) 
	{
		bFound = (wcsicmp(ME.szModule,  lpLibName) == 0) || 
			(wcsicmp(ME.szExePath, lpLibName) == 0);
		if (bFound) break;
	}
	if (!bFound) return;
	hKernel32 = GetModuleHandle(L"kernel32");
	pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary");
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	hThread = CreateRemoteThread(hProcess, 0, 0, pfnThreadRtn, ME.modBaseAddr, 0, 0);
	DebugMsgStr(ME.szModule);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hSnapshot);
	CloseHandle(hThread);
	CloseHandle(hProcess);
}*/

BOOL ShowBalloon(LPCWSTR lpTitle, LPCWSTR lpText, BALLOONICON bIcon, DWORD dwTimeout)
{
	if (!NID.cbSize) return FALSE;
	wcsncpy(NID.szInfoTitle, lpTitle, (sizeof(NID.szInfoTitle) / sizeof(WCHAR)) - 1);
	wcsncpy(NID.szInfo, lpText, (sizeof(NID.szInfo) / sizeof(WCHAR)) - 1);
	switch (bIcon)
	{
		case BI_NONE:
			NID.dwInfoFlags = NIIF_NONE;
			break;
		case BI_INFORMATION:
			NID.dwInfoFlags = NIIF_INFO;
			break;
		case BI_WARNING:
			NID.dwInfoFlags = NIIF_WARNING;
			break;
		case BI_ERROR:
			NID.dwInfoFlags = NIIF_ERROR;
			break;
		default:
			return FALSE;
	}
	NID.uTimeout = (dwTimeout < 10000)?10000:dwTimeout;
	NID.uFlags = NIF_INFO;
	return Shell_NotifyIcon(NIM_MODIFY, &NID);
}

UINT LoadAdditionalFunctions()
{
	UINT nNumOfFunctions = 0;
#ifndef _WIN64
	//x86
	HINSTANCE hEKDLL = LoadLibrary(L"EasyKill.dll");
	if (hEKDLL) {
		if ((FARPROC &)EK_InstallHook = GetProcAddress(hEKDLL, "EK_InstallHook")) nNumOfFunctions++;
		if ((FARPROC &)EK_RemoveHook = GetProcAddress(hEKDLL, "EK_RemoveHook")) nNumOfFunctions++;
	}
#else
	//x64
	HINSTANCE hEKDLL_x64 = LoadLibrary(L"EasyKill_x64.dll");
	if (hEKDLL_x64) {
		if ((FARPROC &)EK_InstallHook_x64 = GetProcAddress(hEKDLL_x64, "EK_InstallHook")) nNumOfFunctions++;
		if ((FARPROC &)EK_RemoveHook_x64 = GetProcAddress(hEKDLL_x64, "EK_RemoveHook")) nNumOfFunctions++;
	}
#endif
	return nNumOfFunctions;
}

//Debug
DWORD DebugMsgInt(int Value) {
	WCHAR lpDbgMsg[64] = {};
	_itow(Value, lpDbgMsg, 10);
	return MessageBox(0, lpDbgMsg, L"Debug", MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONEXCLAMATION);
}

DWORD DebugMsgStr(LPWSTR String) {
	return MessageBox(0, String, L"Debug", MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONEXCLAMATION);
}