// settings.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "pch.h"
#include "framework.h"
#include "settings.h"

CAppModule _Module;

class CMainDlg : public CDialogImpl<CMainDlg>
{
private:

	bool SelSP = false;
	std::vector<int> SerialPortNoList;

	bool SelMP = false;
	std::wstring IniFileName;

public:
	enum { IDD = IDD_DIALOG1 };

	BEGIN_MSG_MAP(CMainDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
		// スクリーンの中央に配置
		CenterWindow();

		//現在の設定の読込
		{
			wchar_t ModulePath[MAX_PATH];
			DWORD r = ::GetModuleFileNameW(nullptr, ModulePath, MAX_PATH);
			for (int i = r - 1; i > 4; i--) {
				if (ModulePath[i] == L'\\') {
					ModulePath[i + 1] = 0;
					break;
				}
			}
			IniFileName = ModulePath;
			IniFileName += L"settings.ini";
		}
		uint32_t PN;
		PN = ::GetPrivateProfileIntW(L"settings", L"PortNumber", 1, IniFileName.c_str());
		uint32_t MPN;
		MPN = ::GetPrivateProfileIntW(L"settings", L"MidiInPortNumber", 0, IniFileName.c_str());

		//Serial Port
		{
			CComboBox hCombo = (CComboBox)GetDlgItem(IDC_COMBO1);

			//SETUPCLASS ポート (COM と LPT)
			//{4D36E978-E325-11CE-BFC1-08002BE10318}
			const GUID GUID_SETUPCLASS_COMPORT =
			{ 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } };

			HDEVINFO hDevInfo = 0;

			hDevInfo = ::SetupDiGetClassDevs(&GUID_SETUPCLASS_COMPORT, NULL, m_hWnd, (DIGCF_PRESENT));
			if (hDevInfo != 0) {
				SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA) };

				for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
					DWORD Length = 0;
					WCHAR Buffer[MAX_PATH];
					SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)Buffer, sizeof(Buffer), &Length);
					std::wregex regex(L"\\(COM(\\d+)\\)$");
					std::wcmatch match;
					if (std::regex_search(Buffer, match, regex)) {
						int pno = ::_wtoi(match[1].str().c_str());
						SerialPortNoList.push_back(pno);
						hCombo.AddString(Buffer);
						SelSP = true;
					}
				}
				::SetupDiDestroyDeviceInfoList(hDevInfo);
				if (SerialPortNoList.size() == 0) {
					hCombo.EnableWindow(false);
				}
				else {
					int ComboBoxCurSel = 0;
					for (int i = 0; i < SerialPortNoList.size(); i++) {
						if (PN == SerialPortNoList[i]) {
							ComboBoxCurSel = i;
							break;
						}
					}
					hCombo.SetCurSel(ComboBoxCurSel);
				}
			}
			else {
				hCombo.EnableWindow(false);
			}
		}

		//MIDI IN Port
		{
			CComboBox hCombo = (CComboBox)GetDlgItem(IDC_COMBO2);

			UINT num = ::midiInGetNumDevs();
			if (num > 0) {
				for (UINT i = 0; i < num; i++) {
					MIDIINCAPS caps;
					if (MMSYSERR_NOERROR == ::midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS))) {
						hCombo.AddString(caps.szPname);
					}
				}
				if (MPN >= num - 1) MPN = 0;
				hCombo.SetCurSel(MPN);
				SelMP = true;
			}
			else {
				hCombo.EnableWindow(false);
			}
		}

		return TRUE;
	}

	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl) {
		//Serial Port
		if (SelSP == true) {
			CComboBox hCombo = (CComboBox)GetDlgItem(IDC_COMBO1);
			int SPN = hCombo.GetCurSel();
			SPN = SerialPortNoList[SPN];
			wchar_t buffer[16];
			::WritePrivateProfileStringW(L"settings", L"PortNumber", _itow(SPN, buffer, 10), IniFileName.c_str());
		}
		//MIDI IN Port
		if (SelMP == true) {
			CComboBox hCombo = (CComboBox)GetDlgItem(IDC_COMBO2);
			int MPN = hCombo.GetCurSel();
			wchar_t buffer[16];
			::WritePrivateProfileStringW(L"settings", L"MidiInPortNumber", _itow(MPN, buffer, 10), IniFileName.c_str());
		}
		EndDialog(nID);
	}

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl) {
		EndDialog(nID);
	}
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	_Module.Init(NULL, hInstance);

	CMainDlg dlg;
	int nRet = (int)dlg.DoModal();

	_Module.Term();

	return nRet;
}
