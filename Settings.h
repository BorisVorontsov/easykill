#ifndef SETTINGS_H
#define SETTINGS_H

#ifndef _VECTOR_
#include <vector>
using namespace std;
#endif

#define EK_REG_AUTORUN_PATH		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

#define EK_REG_APP_KEY			L"Software\\EasyKill\\"
#define EK_REG_GENERAL_SUBKEY	L"General"
#define EK_REG_RULES_SUBKEY		L"Rules"

typedef enum tagProcRules
{
	prTerminateAlways = 0,
	prInterpretAsCriticalProcess,
	prIgnoreAtAll
} ProcRules;

typedef struct tagEKRULE
{
	WCHAR lpProcName[MAX_PATH];
	DWORD dwRuleIdx;
} EKRULE, *LPEKRULE;

class CSettings
{
public:
	CSettings();
	~CSettings();

	DWORD m_dwTarget;
	DWORD m_dwWindowGhostingWorkaround;
	DWORD m_dwTPTimeout;
	DWORD m_dwTPTimeoutValue;
	DWORD m_dwAutorun;
	DWORD m_dwRebootOnHotkey;
	DWORD m_dwHotkey;
	DWORD m_dwModifiers;
	vector<EKRULE> m_EkRules;

	void SetDefaultValues();
	void GetSettings();
	void SaveSettings();
};

extern CSettings Settings;

#endif