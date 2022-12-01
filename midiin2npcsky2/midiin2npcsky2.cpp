#include "pch.h"

const char* NoteNameList[15] = { "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5", "B5", "C6" };
constexpr uint8_t NoteNoList[15] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84 };

uint32_t TimeStamp = 0;
union {
	uint32_t W;
	struct {
		uint8_t Press0 : 1;//note on信号が来た
		uint8_t Press1 : 1;//note on信号が来た
		uint8_t DownCounter;//ボタンを押している時間をカウントダウン
	};
} Key[15];

namespace API {
	class CSerialPort {
	private:

		HANDLE hHandle;

	public:

		CSerialPort()
		{
			hHandle = INVALID_HANDLE_VALUE;
		}

		int32_t Open(uint32_t PortNumber)
		{
			wchar_t buf[8];
			std::wstring path;
			path = L"\\\\.\\COM";
			path += ::_itow(PortNumber, buf, 10);
			hHandle = ::CreateFileW(path.c_str(), (GENERIC_READ | GENERIC_WRITE), 0, nullptr, OPEN_EXISTING, 0, nullptr);
			if (hHandle == INVALID_HANDLE_VALUE) {
				return -1;
			}

			::SetupComm(hHandle, 1024, 1024);

			::PurgeComm(hHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

			DCB dcb;
			GetCommState(hHandle, &dcb);
			dcb.BaudRate = 115200;
			dcb.fBinary = 1;
			dcb.fParity = NOPARITY;
			dcb.fOutxCtsFlow = 0;
			dcb.fOutxDsrFlow = 0;
			dcb.fDtrControl = 1;
			dcb.fDsrSensitivity = 0;
			dcb.fTXContinueOnXoff = 0;
			dcb.fOutX = 1;
			dcb.fInX = 1;
			dcb.ErrorChar = 0;
			dcb.fNull = 0;
			dcb.fRtsControl = 0;
			dcb.fAbortOnError = 0;
			dcb.fDummy2 = 0;
			dcb.wReserved = 0;
			dcb.XonLim = 0;
			dcb.XoffChar = 0;
			dcb.ByteSize = 8;
			dcb.Parity = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.XonChar = 0;
			dcb.XoffChar = 0;
			dcb.ErrorChar = 0;
			dcb.EofChar = 0;
			dcb.EvtChar = 0;
			dcb.wReserved1 = 0;
			SetCommState(hHandle, &dcb);

			return 0;
		}

		int Close()
		{
			if (hHandle != INVALID_HANDLE_VALUE) {
				::CloseHandle(hHandle);
			}
			return 0;
		}

		int Write(const void* data, const int32_t size)
		{
			DWORD writeSize;
			BOOL ret = ::WriteFile(hHandle, data, size, &writeSize, nullptr);
			if (ret != 0) return 0;
			else return -1;
		}
	};
}

//Gamepad button mapping
// this XInput Switch PS3    DInput
// B1   A      B      ×     2
// B2   B      A      ○     3
// B3   X      Y      □     1
// B4   Y      X      △     4
// L1   LB     L      L1     5
// R1   RB     R      R1     6
// L2   LT     ZL     L2     7
// R2   RT     ZR     R2     8
// S1   Back   -      Select 9
// S2   Start  +      Start  10
// L3   LS     LS     L3     11
// R3   RS     Rs     R3     12
// A1   Guide  Home          13
// A2          Capture       14
union GamepadState {
	uint8_t B[11];
	struct {
		union {
			uint16_t B;
			struct {
				uint16_t B1 : 1;
				uint16_t B2 : 1;
				uint16_t B3 : 1;
				uint16_t B4 : 1;
				uint16_t L1 : 1;
				uint16_t R1 : 1;
				uint16_t L2 : 1;
				uint16_t R2 : 1;
				uint16_t S1 : 1;
				uint16_t S2 : 1;
				uint16_t L3 : 1;
				uint16_t R3 : 1;
				uint16_t A1 : 1;
				uint16_t A2 : 1;
				uint16_t : 2;
			};
		} Button;
		uint16_t Aux;
		union {
			uint8_t B;
			struct {
				uint8_t Up : 1;
				uint8_t Down : 1;
				uint8_t Left : 1;
				uint8_t Right : 1;
				uint8_t : 4;
			};
		} DPad;
		uint8_t LX;
		uint8_t LY;
		uint8_t RX;
		uint8_t RY;
		uint8_t LT;
		uint8_t RT;
	};
	void Clear() {
		DPad.B = 0;
		Button.B = 0;
		Aux = 0;
		LX = 128;
		LY = 128;
		RX = 128;
		RY = 128;
		LT = 0;
		RT = 0;
	}
	GamepadState() {
		Clear();
	}
};
GamepadState report;

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (wMsg) {
	case MIM_OPEN:
		std::printf("MIDI device is opened.\n");
		break;
	case MIM_CLOSE:
		printf("MIDI device is closed.\n");
		break;
	case MIM_DATA:
	{
		uint8_t data[3];
		uint32_t timestamp;
		data[0] = (uint8_t)((dwParam1 & 0x000000FF) >> 0);
		data[1] = (uint8_t)((dwParam1 & 0x0000FF00) >> 8);
		data[2] = (uint8_t)((dwParam1 & 0x00FF0000) >> 16);
		timestamp = (uint32_t)dwParam2;

		constexpr uint8_t ChMax = 15;
		if ((data[0] & 0xF0) == 0x90 && data[2] != 0x00) {
			uint8_t ch = data[0] & 0x0F;
			if (ch <= ChMax) {
				for (int i = 0; i < 15; i++) {
					if (NoteNoList[i] == data[1]) {
						Key[i].Press0 = 1;
						Key[i].Press1 = 1;
						TimeStamp = timestamp;
					}
				}
			}
		}
		break;
	}
	case MIM_LONGDATA:
		break;
	case MIM_ERROR:
	case MIM_LONGERROR:
	case MIM_MOREDATA:
	default:
		break;
	}
}

//レポートに押すボタンを反映する
//戻り値 true:反映した false:同時押し不可能なキーのため反映できなかった
bool Press(int note)
{
	bool ret = true;

	switch (note) {
	case 0: report.LT = 255; break;
	case 1: report.RT = 255; break;
	case 2: if (report.DPad.Up == 0) report.DPad.Down = 1; else ret = false;  break;
	case 3: report.Button.B1 = 1; break;
	case 4: if (report.DPad.Right == 0) report.DPad.Left = 1; else ret = false;  break;

	case 5: report.Button.B3 = 1; break;
	case 6: if (report.DPad.Down == 0) report.DPad.Up = 1; else ret = false;  break;
	case 7: report.Button.B4 = 1; break;
	case 8: if (report.DPad.Left == 0) report.DPad.Right = 1; else ret = false;  break;
	case 9: report.Button.B2 = 1; break;

	case 10: report.Button.L1 = 1; break;
	case 11: report.Button.R1 = 1; break;
	case 12: if (report.LX == 128) report.LX = 0; else ret = false; break;
	case 13: report.RX = 0; break;
	case 14: if (report.LX == 128) report.LX = 255; else ret = false;  break;
	}

	return ret;
}

//reportに離すボタンを反映する
void Unpress(int note)
{
	switch (note) {
	case 0: report.LT = 0; break;
	case 1: report.RT = 0; break;
	case 2: report.DPad.Down = 0; break;
	case 3: report.Button.B1 = 0; break;
	case 4: report.DPad.Left = 0; break;

	case 5: report.Button.B3 = 0; break;
	case 6: report.DPad.Up = 0; break;
	case 7: report.Button.B4 = 0; break;
	case 8: report.DPad.Right = 0; break;
	case 9: report.Button.B2 = 0; break;

	case 10: report.Button.L1 = 0; break;
	case 11: report.Button.R1 = 0; break;
	case 12: report.LX = 128; break;
	case 13: report.RX = 128; break;
	case 14: report.LX = 128; break;
	}
}

int main()
{
	std::printf("MidiIN Device List\n");
	{
		UINT num = ::midiInGetNumDevs();
		for (int i = 0; (UINT)i < num; i++) {
			MIDIINCAPS caps;
			if (MMSYSERR_NOERROR == ::midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS))) {
				std::wprintf(L"%4d  %s\n", i, caps.szPname);
			}
		}
	}

	std::wstring IniFileName;
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
	uint32_t MPN;
	MPN = ::GetPrivateProfileIntW(L"settings", L"MidiInPortNumber", 0, IniFileName.c_str());
	uint32_t PN;
	PN = ::GetPrivateProfileIntW(L"settings", L"PortNumber", 3, IniFileName.c_str());

	API::CSerialPort sp;
	if (sp.Open(PN) != 0) {
		std::cout << "Error 01" << std::endl;
		return 1;
	}

	::memset(Key, 0, sizeof(Key));

	HMIDIIN Handle;
	MMRESULT result;
	result = ::midiInOpen(&Handle, MPN, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);
	result = ::midiInStart(Handle);
	::timeBeginPeriod(1);

	while (1) {
		::Sleep(10);
		{//log
			bool co = false;
			for (int i = 0; i < 15; i++) {
				if (Key[i].Press0 == 1) {
					co = true;
				}
			}
			if (co) {
				std::string str = "";
				{
					char buf[16];
					std::sprintf(buf, "%8.3f", (double)TimeStamp / 100.0);
					str += buf;
				}
				for (int i = 0; i < 15; i++) {
					str += "  ";
					if (Key[i].Press0 == 1) {
						str += NoteNameList[i];
						Key[i].Press0 = 0;
					}
					else {
						str += "  ";
					}
				}
				str += "\n";
				printf(str.c_str());
			}
		}
		{
			constexpr int DTC = 8;
			//離すボタンの処理
			for (int i = 0; i < 15; i++) {
				if (Key[i].DownCounter > 0) {
					Key[i].DownCounter--;
					if (Key[i].DownCounter == 0) {
						Unpress(i);
					}
				}
			}
			//押すボタンの処理
			for (int i = 0; i < 15; i++) {
				if (Key[i].Press1 == 1) {
					if (Key[i].DownCounter == 0) {
						if (Press(i) == true) {
							Key[i].Press1 = 0;
							Key[i].DownCounter = DTC;
						}
					}
				}
			}
		}

		{
			static uint8_t cnt = 0;
			uint8_t buf[14];//Sync[1] Size[1] Header[1] Data[11] ※Size=Header+Data
			buf[0] = 0b1010'0000 + (cnt & 0x0F); cnt++;
			buf[1] = 12;
			buf[2] = 0xC0;
			buf[3] = report.B[0];
			buf[4] = report.B[1];
			buf[5] = report.B[2];
			buf[6] = report.B[3];
			buf[7] = report.B[4];
			buf[8] = report.B[5];
			buf[9] = report.B[6];
			buf[10] = report.B[7];
			buf[11] = report.B[8];
			buf[12] = report.B[9];
			buf[13] = report.B[10];
			if (sp.Write(buf, 14) != 0) {
				std::cout << "Error 02" << std::endl;
				break;
			}
		}
	}

	timeEndPeriod(1);
	result = ::midiInStop(Handle);
	result = ::midiInClose(Handle);

	sp.Close();
}
