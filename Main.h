#ifndef EASYKILL_H_
#define EASYKILL_H_

LRESULT CALLBACK ClbkWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
void AdjustPrivilege(LPWSTR lpPrivilege);
void Hotkey(BOOL bUnregister);
void ShowAbout();
void ShowOptions();
void AddToAutorun();
void RemoveFromAutorun();
BOOL IsUserAdmin();
BOOL ReadCommandLine(LPWSTR lpCmdLine);
BOOL GetAppPath(LPWSTR lpPath, size_t szPathLen, BOOL bAddQuotes = FALSE);
void GetProcessName(DWORD dwPID, LPWSTR lpResult);
BOOL IsCriticalProcess(LPWSTR lpProcessName);
BOOL IsHungWindow(HWND hWnd, DWORD dwTimeout);
size_t GetName(LPWSTR lpPath);
size_t GetTitle(LPWSTR lpFileName);
size_t TrimEx(LPCWSTR lpString, LPWSTR lpResult, WCHAR ch);
int _wcswildicmp(LPCWSTR lpWildStr, LPCWSTR lpStrToComp);
void MoveToCenter(HWND hWnd, LONG lXOffset, LONG lYOffset);
DWORD ReadFileVersion(LPWSTR lpFileName, LPWSTR lpResult);
DWORD GetKeyName(DWORD VK, LPWSTR lpKey, DWORD dwKeyNameLen);
void CheckAndKillBkgndProcesses();
BOOL ExtendedKillProcess(LPWSTR lpWndTitle = NULL);

#ifdef _WIN64

void RunSurrogatex86();
BOOL PostMessageToSurrogatex86(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

#endif

//void InjectLibrary(DWORD dwPID, LPWSTR lpLibName);
//void EjectLibrary(DWORD dwPID, LPWSTR lpLibName);

typedef enum _BALLOONICON
{
	BI_NONE = 0,
	BI_INFORMATION,
	BI_WARNING,
	BI_ERROR
} BALLOONICON;

BOOL ShowBalloon(LPCWSTR lpTitle, LPCWSTR lpText, BALLOONICON bIcon, DWORD dwTimeout = 0);

DWORD DebugMsgInt(int Value);
DWORD DebugMsgStr(LPWSTR String);

#define _SC(f)					{ if (f) { f(); } }

#define APP_NAME				L"EasyKill"
#define APP_COPYRIGHT			L"Copyright © Boris Vorontsov, 2007 - 2012\nAll rights reserved."

#define CMD_SENDEMAIL			L"mailto: borisvorontsov@gmail.com"

#define EK_CMD_TERMINATE		L"-t"

#define EKSM_INSTALL_X86_HOOK	(WM_APP + 0x70)
#define EKSM_REMOVE_X86_HOOK	(WM_APP + 0x71)
#define EKSM_EXIT				(WM_APP + 0x72)

#define WM_NICALLBACK			(WM_APP + 0x80)

#define IDM_ABOUT				0x00000001
#define IDM_SEP1				0x00000002
#define IDM_OPT					0x00000003
#define IDM_SEP2				0x00000004
#define IDM_EXIT				0x00000005

#define RHK_ID					3007
#define NID_ID					3008

//EasyKill Hook Library
UINT LoadAdditionalFunctions();

extern OSVERSIONINFO OSVI;
extern HANDLE hMutex;
extern HINSTANCE hAppInstance;
extern WCHAR lpAppPath[MAX_PATH];
extern WCHAR lpAppPathNoQts[MAX_PATH];
extern WCHAR lpAppVersion[16];

extern HWND hWndOptions;
extern HWND hWndAbout;

#endif