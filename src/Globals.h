#pragma once

#include <windows.h>
#include <msctf.h>
#include <objbase.h>
#include <string>
#include <vector>

// ============================================================
//  本示例专用的 GUID。正式发布时请重新生成，避免与别人冲突。
//  使用 DECLSPEC_SELECTANY，头文件被多个 .cpp 包含也不会重复定义。
// ============================================================
EXTERN_C const GUID DECLSPEC_SELECTANY CLSID_MinimalIME =
{ 0xB7901AAF, 0xFED6, 0x4E40, { 0x90, 0x5F, 0x6D, 0x13, 0x53, 0x64, 0x0E, 0xCB } };

EXTERN_C const GUID DECLSPEC_SELECTANY GUID_MinimalProfile =
{ 0x791D57B7, 0xDC58, 0x4809, { 0xB8, 0xBB, 0xA5, 0xD8, 0xEC, 0x81, 0x86, 0xEA } };

#define MINIMAL_IME_NAME  L"极简拼音 (TSF 示例)"
#define MINIMAL_LANGID    0x0804          // 简体中文

extern HINSTANCE g_hInst;
extern volatile LONG g_cRefDll;

void        DllAddRef();
void        DllRelease();
std::wstring GetDllPath();
