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

int main()
{
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
	uint32_t PN;
	PN = ::GetPrivateProfileIntW(L"settings", L"PortNumber", 3, IniFileName.c_str());

	API::CSerialPort sp;
	if (sp.Open(PN) != 0) {
		std::cout << "Error 01" << std::endl;
		return 1;
	}

	while (true) {
		XINPUT_STATE State;
		GamepadState state;
		if (ERROR_SUCCESS == ::XInputGetState(0, &State)) {
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_A) state.Button.B1 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_B) state.Button.B2 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_X) state.Button.B3 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_Y) state.Button.B4 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) state.Button.L1 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) state.Button.R1 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) state.Button.S1 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_START) state.Button.S2 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) state.Button.L3 = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) state.Button.R3 = 1;
			state.LT = State.Gamepad.bLeftTrigger;
			state.RT = State.Gamepad.bRightTrigger;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) state.DPad.Up = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) state.DPad.Down = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) state.DPad.Left = 1;
			if (State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) state.DPad.Right = 1;
			state.LX = (uint8_t)((State.Gamepad.sThumbLX >> 8) + 128);
			state.LY = (uint8_t)(255 - ((State.Gamepad.sThumbLY >> 8) + 128));
			state.RX = (uint8_t)((State.Gamepad.sThumbRX >> 8) + 128);
			state.RY = (uint8_t)(255 - ((State.Gamepad.sThumbRY >> 8) + 128));
		}
		{
			if (::GetAsyncKeyState(VK_RCONTROL) & 0x8000) {//XInputでは取得できないからキーボードに割り当てる
				state.Button.A1 = 1;
			}
		}
		{
			static uint8_t cnt = 0;
			uint8_t buf[14];//Sync[1] Size[1] Header[1] Data[11] ※Size=Header+Data
			buf[0] = 0b1010'0000 + (cnt & 0x0F); cnt++;
			buf[1] = 12;
			buf[2] = 0xC0;
			buf[3] = state.B[0];
			buf[4] = state.B[1];
			buf[5] = state.B[2];
			buf[6] = state.B[3];
			buf[7] = state.B[4];
			buf[8] = state.B[5];
			buf[9] = state.B[6];
			buf[10] = state.B[7];
			buf[11] = state.B[8];
			buf[12] = state.B[9];
			buf[13] = state.B[10];
			if (sp.Write(buf, 14) != 0) {
				std::cout << "Error 02" << std::endl;
				break;
			}
		}
		::Sleep(10);
	}
	sp.Close();

	return 0;
}
