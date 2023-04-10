#include <windows.h>

#include "main.h"
#include "settings.h"

CSettings Settings;

CSettings::CSettings()
{
	/* none */
};

CSettings::~CSettings()
{
	/* none */
};

void CSettings::SetDefaultValues()
{
	m_dwTarget = 0;
	m_dwWindowGhostingWorkaround = 1;
	m_dwTPTimeout = 1;
	m_dwTPTimeoutValue = 5;
	m_dwAutorun = 0;
	m_dwRebootOnHotkey = 0;
	m_dwHotkey = VK_F4;
	m_dwModifiers = MOD_WIN;
	m_EkRules.clear();
}

void CSettings::GetSettings()
{
	HKEY hGSAppKey, hGSGeneralSubKey, hGSRulesSubKey;
	DWORD dwDWORDSIZE = sizeof(DWORD);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, EK_REG_APP_KEY, 0, KEY_ALL_ACCESS, &hGSAppKey) == ERROR_SUCCESS)
	{
		if (RegOpenKeyEx(hGSAppKey, EK_REG_GENERAL_SUBKEY, 0, KEY_ALL_ACCESS, &hGSGeneralSubKey) == ERROR_SUCCESS)
		{
			RegQueryValueEx(hGSGeneralSubKey, L"Target", 0, 0, (BYTE *)&m_dwTarget, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"WindowGhostingWorkaround", 0, 0, (BYTE *)&m_dwWindowGhostingWorkaround, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"TPTimeout", 0, 0, (BYTE *)&m_dwTPTimeout, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"TPTimeoutValue", 0, 0, (BYTE *)&m_dwTPTimeoutValue, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"Autorun", 0, 0, (BYTE *)&m_dwAutorun, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"Modifiers", 0, 0, (BYTE *)&m_dwModifiers, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"Key", 0, 0, (BYTE *)&m_dwHotkey, &dwDWORDSIZE);
			RegQueryValueEx(hGSGeneralSubKey, L"RebootOnHotkey", 0, 0, (BYTE *)&m_dwRebootOnHotkey, &dwDWORDSIZE);

			RegCloseKey(hGSGeneralSubKey);
		}

		if (RegOpenKeyEx(hGSAppKey, EK_REG_RULES_SUBKEY, 0, KEY_ALL_ACCESS, &hGSRulesSubKey) == ERROR_SUCCESS)
		{
			DWORD dwKeyIdx = 0;
			EKRULE EKR;
			DWORD dwNameSize = sizeof(EKR.lpProcName) / sizeof(WCHAR);

			while (RegEnumValue(hGSRulesSubKey, dwKeyIdx++, EKR.lpProcName, &dwNameSize, NULL, NULL, (BYTE *)&EKR.dwRuleIdx, &dwDWORDSIZE) == ERROR_SUCCESS) {
				m_EkRules.push_back(EKR);

				dwNameSize = sizeof(EKR.lpProcName) / sizeof(WCHAR);
			}

			RegCloseKey(hGSRulesSubKey);
		}

		RegCloseKey(hGSAppKey);

		return;
	}

	SetDefaultValues();
	SaveSettings();
}

void CSettings::SaveSettings()
{
	HKEY hSSAppKey, hSSGeneralSubKey, hSSRulesSubKey, hStub;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, EK_REG_APP_KEY, 0, L"REG_DWORD", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hSSAppKey, &dwDisposition);

	RegCreateKeyEx(hSSAppKey, EK_REG_GENERAL_SUBKEY, 0, L"REG_DWORD", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &hSSGeneralSubKey, &dwDisposition);

	RegSetValueEx(hSSGeneralSubKey, L"Target", 0, REG_DWORD, (CONST BYTE *)&m_dwTarget, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"WindowGhostingWorkaround", 0, REG_DWORD, (CONST BYTE *)&m_dwWindowGhostingWorkaround, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"TPTimeout", 0, REG_DWORD, (CONST BYTE *)&m_dwTPTimeout, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"TPTimeoutValue", 0, REG_DWORD, (CONST BYTE *)&m_dwTPTimeoutValue, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"Autorun", 0, REG_DWORD, (CONST BYTE *)&m_dwAutorun, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"Modifiers", 0, REG_DWORD, (CONST BYTE *)&m_dwModifiers, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"Key", 0, REG_DWORD, (CONST BYTE *)&m_dwHotkey, sizeof(DWORD));
	RegSetValueEx(hSSGeneralSubKey, L"RebootOnHotkey", 0, REG_DWORD, (CONST BYTE *)&m_dwRebootOnHotkey, sizeof(DWORD));
	
	RegCloseKey(hSSGeneralSubKey);

	if (RegOpenKeyEx(hSSAppKey, EK_REG_RULES_SUBKEY, 0, KEY_ALL_ACCESS, &hStub) == ERROR_SUCCESS) {
		RegDeleteKey(hSSAppKey, EK_REG_RULES_SUBKEY);
		RegCloseKey(hStub);
	}

	RegCreateKeyEx(hSSAppKey, EK_REG_RULES_SUBKEY, 0, L"REG_DWORD", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
		0, &hSSRulesSubKey, &dwDisposition);

	for (vector<EKRULE>::iterator i = m_EkRules.begin(); i < m_EkRules.end(); i++) {
		RegSetValueEx(hSSGeneralSubKey, i->lpProcName, 0, REG_DWORD, (CONST BYTE *)&i->dwRuleIdx, sizeof(DWORD));
	}

	RegCloseKey(hSSRulesSubKey);

	RegCloseKey(hSSAppKey);
}