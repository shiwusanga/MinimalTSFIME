#include "Globals.h"

HINSTANCE        g_hInst = nullptr;
volatile LONG    g_cRefDll = 0;

void DllAddRef()  { InterlockedIncrement(&g_cRefDll); }
void DllRelease() { InterlockedDecrement(&g_cRefDll); }

std::wstring GetDllPath()
{
    WCHAR sz[MAX_PATH] = {};
    GetModuleFileNameW(g_hInst, sz, MAX_PATH);
    return std::wstring(sz);
}
