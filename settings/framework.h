#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する
// Windows ヘッダー ファイル
#include <windows.h>
#include <SetupAPI.h>
#include <mmeapi.h>

//ATL
#include <atlbase.h>
#include <atlwin.h>

//WTL
#include <atlapp.h>
extern CAppModule _Module;
#include <atlcrack.h>
#include <atlmisc.h>
#include <atlctrls.h> 

//C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

//C++
#include <string>
#include <regex>

//視覚スタイルを有効にする
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
