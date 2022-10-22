#include "pch.h"

const char* NoteNameList[15] = { "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5", "B5", "C6" };
constexpr uint8_t NoteNoList[15] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84 };

uint32_t TimeStamp = 0;
union {
	uint32_t W;
	struct {
		uint8_t cnt1;
		uint8_t down : 1;
		uint8_t up : 1;
		uint8_t press : 1;
	};
} Key[15];

union USB_JoystickReport_Input_t
{
	uint8_t B[8];
	struct {
		union {
			uint16_t hw;
			struct {
				uint16_t Y : 1;
				uint16_t B : 1;
				uint16_t A : 1;
				uint16_t X : 1;
				uint16_t L : 1;
				uint16_t R : 1;
				uint16_t ZL : 1;
				uint16_t ZR : 1;
				uint16_t Minus : 1;
				uint16_t Plus : 1;
				uint16_t LClick : 1;
				uint16_t RClick : 1;
				uint16_t Home : 1;
				uint16_t Capture : 1;
			};
		} Button;
		uint8_t Hat;//0-7: 0:u 2:r 4:d 6:l 8 Neutral
		uint8_t LX;
		uint8_t LY;
		uint8_t RX;
		uint8_t RY;
		uint8_t VendorSpec;
	};
	USB_JoystickReport_Input_t() {
		Button.hw = 0;
		Hat = 8;
		LX = 128;
		LY = 128;
		RX = 128;
		RY = 128;
		VendorSpec = 0;
	}
};
USB_JoystickReport_Input_t report;

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
		data[0] = (dwParam1 & 0x000000FF) >> 0;
		data[1] = (dwParam1 & 0x0000FF00) >> 8;
		data[2] = (dwParam1 & 0x00FF0000) >> 16;
		timestamp = (uint32_t)dwParam2;
		//{
		//	printf("%10d  %02X %02X %02X\n", timestamp, data[0], data[1], data[2]);
		//}
		if (data[0] == 0x80 || (data[0] == 0x90 && data[2] == 0x00)) {//note off
			for (int i = 0; i < 15; i++) {
				if (NoteNoList[i] == data[1]) {
					Key[i].up = 1;
					Key[i].down = 0;
					TimeStamp = timestamp;
				}
			}
		}
		else if (data[0] == 0x90) {//note on
			for (int i = 0; i < 15; i++) {
				if (NoteNoList[i] == data[1]) {
					Key[i].up = 0;
					Key[i].down = 1;
					TimeStamp = timestamp;
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

void SetReport(int note)
{
	switch (note) {
	case 0: report.Button.ZL = 1; break;
	case 1: report.Button.ZR = 1; break;
	case 2: report.Hat = 4; break;
	case 3: report.Button.B = 1; break;
	case 4: report.Hat = 6; break;

	case 5: report.Button.Y = 1; break;
	case 6: report.Hat = 0; break;
	case 7: report.Button.X = 1; break;
	case 8: report.Hat = 2; break;
	case 9: report.Button.A = 1; break;

	case 10: report.Button.L = 1; break;
	case 11: report.Button.R = 1; break;
	case 12: report.LX = 0; break;
	case 13: report.RX = 0; break;
	case 14: report.LX = 255; break;
	}
}

void ClearReport(int note)
{
	switch (note) {
	case 0: report.Button.ZL = 0; break;
	case 1: report.Button.ZR = 0; break;
	case 2: report.Hat = 8; break;
	case 3: report.Button.B = 0; break;
	case 4: report.Hat = 8; break;

	case 5: report.Button.Y = 0; break;
	case 6: report.Hat = 8; break;
	case 7: report.Button.X = 0; break;
	case 8: report.Hat = 8; break;
	case 9: report.Button.A = 0; break;

	case 10: report.Button.L = 0; break;
	case 11: report.Button.R = 0; break;
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
		for (int i = 0; i < num; i++) {
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
	while (1) {
		::Sleep(10);
		bool co = false;
		for (int i = 0; i < 15; i++) {
			if (Key[i].up == 1) {
				Key[i].press = 0;
				co = true;
			}
			else if (Key[i].down == 1) {
				Key[i].press = 1;
				Key[i].cnt1 = 0;
				co = true;
			}
			Key[i].up = 0;
			Key[i].down = 0;
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
				if (Key[i].press == 1) {
					str += NoteNameList[i];
				}
				else {
					str += "  ";
				}
			}
			str += "\n";
			printf(str.c_str());
		}

		constexpr int DTC = 8;
		for (int i = 0; i < 15; i++) {
			if (Key[i].cnt1 > 0) {
				Key[i].cnt1++;
				if (Key[i].cnt1 == DTC) {
					ClearReport(i);
				}
			}
		}
		{
			bool noteon = true;
			for (int i = 0; i < 15; i++) {
				if (0 < Key[i].cnt1 && Key[i].cnt1 < DTC) {
					noteon = false;
					break;
				}
			}
			if (noteon == true) {
				for (int i = 0; i < 15; i++) {
					if (Key[i].press == 1 && Key[i].cnt1 == 0) {
						SetReport(i);
						Key[i].cnt1 = 1;
						break;
					}
				}
			}
		}
		{
			uint8_t buf[8];
			buf[0] = 0x80 + ((report.B[0] & 0xFE) >> 1);
			buf[1] = ((report.B[0] & 0x01) << 6) + ((report.B[1] & 0xFC) >> 2);
			buf[2] = ((report.B[1] & 0x03) << 5) + ((report.B[2] & 0xF8) >> 3);
			buf[3] = ((report.B[2] & 0x07) << 4) + ((report.B[3] & 0xF0) >> 4);
			buf[4] = ((report.B[3] & 0x0F) << 3) + ((report.B[4] & 0xE0) >> 5);
			buf[5] = ((report.B[4] & 0x1F) << 2) + ((report.B[5] & 0xC0) >> 6);
			buf[6] = ((report.B[5] & 0x3F) << 1) + ((report.B[6] & 0x80) >> 7);
			buf[7] = ((report.B[6] & 0x7F) << 0);
			if (sp.Write(buf, 8) != 0) {
				std::cout << "Error 02" << std::endl;
				break;
			}
		}
	}
	result = ::midiInStop(Handle);
	result = ::midiInClose(Handle);

	sp.Close();
}
