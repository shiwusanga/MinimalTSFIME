#pragma once
#include <windows.h>

// TRUE = 安装注册，FALSE = 卸载反注册。由 DllRegisterServer / DllUnregisterServer 调用。
HRESULT RegisterIme(BOOL bInstall);
