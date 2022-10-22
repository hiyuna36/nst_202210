#include "pch.h"

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

int main()
{
	std::wstring IniFileName;
	{
		wchar_t ModulePath[MAX_PATH];
		DWORD r = ::GetModuleFileNameW(nullptr, ModulePath, MAX_PATH);
		for (int i = r - 1; i < 4; i--) {
			if (ModulePath[i] == L'\\') {
				ModulePath[i + 1] = 0;
				break;
			}
		}
		IniFileName = ModulePath;
		IniFileName += L"settings.ini";
	}
	uint32_t PN;
	PN = ::GetPrivateProfileIntW(L"settings", L"PortNumber", 3, IniFileName.c_str());

	API::CSerialPort sp;
	if (sp.Open(PN) != 0) {
		std::cout << "Error 01" << std::endl;
		return 1;
	}

	while (true) {
		XINPUT_STATE State;
		USB_JoystickReport_Input_t JoystickReport;
		if (ERROR_SUCCESS == ::XInputGetState(0, &State)) {
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_Y) JoystickReport.Button.X = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_B) JoystickReport.Button.A = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_A) JoystickReport.Button.B = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_X) JoystickReport.Button.Y = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) JoystickReport.Button.L = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) JoystickReport.Button.R = 1;
			if (State.Gamepad.bLeftTrigger > 128) JoystickReport.Button.ZL = 1;
			if (State.Gamepad.bRightTrigger > 128) JoystickReport.Button.ZR = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) JoystickReport.Button.Minus = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_START) JoystickReport.Button.Plus = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) JoystickReport.Button.LClick = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) JoystickReport.Button.RClick = 1;
			constexpr WORD XINPUT_GAMEPAD_DPAD = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT;
			if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == XINPUT_GAMEPAD_DPAD_UP) JoystickReport.Hat = 0;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT)) JoystickReport.Hat = 1;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == XINPUT_GAMEPAD_DPAD_RIGHT) JoystickReport.Hat = 2;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == (XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN)) JoystickReport.Hat = 3;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == XINPUT_GAMEPAD_DPAD_DOWN) JoystickReport.Hat = 4;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == (XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT)) JoystickReport.Hat = 5;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == XINPUT_GAMEPAD_DPAD_LEFT) JoystickReport.Hat = 6;
			else if ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD) == (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP)) JoystickReport.Hat = 7;
			JoystickReport.LX = (uint8_t)((State.Gamepad.sThumbLX >> 8) + 128);
			JoystickReport.LY = (uint8_t)(255 - ((State.Gamepad.sThumbLY >> 8) + 128));
			JoystickReport.RX = (uint8_t)((State.Gamepad.sThumbRX >> 8) + 128);
			JoystickReport.RY = (uint8_t)(255 - ((State.Gamepad.sThumbRY >> 8) + 128));
		}
		{
			if (::GetAsyncKeyState(VK_RCONTROL) & 0x8000) {
				JoystickReport.Button.Home = 1;
			}
		}
		{
			uint8_t buf[8];
			buf[0] = 0x80 + ((JoystickReport.B[0] & 0xFE) >> 1);
			buf[1] = ((JoystickReport.B[0] & 0x01) << 6) + ((JoystickReport.B[1] & 0xFC) >> 2);
			buf[2] = ((JoystickReport.B[1] & 0x03) << 5) + ((JoystickReport.B[2] & 0xF8) >> 3);
			buf[3] = ((JoystickReport.B[2] & 0x07) << 4) + ((JoystickReport.B[3] & 0xF0) >> 4);
			buf[4] = ((JoystickReport.B[3] & 0x0F) << 3) + ((JoystickReport.B[4] & 0xE0) >> 5);
			buf[5] = ((JoystickReport.B[4] & 0x1F) << 2) + ((JoystickReport.B[5] & 0xC0) >> 6);
			buf[6] = ((JoystickReport.B[5] & 0x3F) << 1) + ((JoystickReport.B[6] & 0x80) >> 7);
			buf[7] = ((JoystickReport.B[6] & 0x7F) << 0);
			if (sp.Write(buf, 8) != 0) {
				std::cout << "Error 02" << std::endl;
				break;
			}
		}
		::Sleep(10);
	}
	sp.Close();

	return 0;
}
