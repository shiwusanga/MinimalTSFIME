#include "Globals.h"
#include "Register.h"
#include <objbase.h>
#include <msctf.h>
#include <new>
#include <cwchar>

// ------------------------------------------------------------
// 注册表辅助：只写 COM 必需的两项；TIP / 语言配置 / 类别全部走官方 TSF API，
// 绝不手写 HKLM\SOFTWARE\Microsoft\CTF\TIP（那是系统内部状态）。
// ------------------------------------------------------------
static bool WriteRegString(HKEY root, const std::wstring& sub,
                           const WCHAR* name, const std::wstring& data)
{
    HKEY hk = nullptr;
    if (RegCreateKeyExW(root, sub.c_str(), 0, nullptr, 0, KEY_WRITE,
                        nullptr, &hk, nullptr) != ERROR_SUCCESS)
        return false;
    LSTATUS st = RegSetValueExW(hk, name, 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(data.c_str()),
                                static_cast<DWORD>((data.size() + 1) * sizeof(WCHAR)));
    RegCloseKey(hk);
    return st == ERROR_SUCCESS;
}

static std::wstring GuidToStr(const GUID& g)
{
    WCHAR s[40] = {};
    StringFromGUID2(g, s, 40);
    return std::wstring(s);
}

// input.dll 没有对应的导入库，也没有公共头文件，动态取函数指针。
static void EnableLayout(BOOL bInstall)
{
    using PFN = BOOL (WINAPI *)(LPCWSTR, DWORD);
    HMODULE h = LoadLibraryW(L"input.dll");
    if (!h) return;
    PFN pfn = reinterpret_cast<PFN>(GetProcAddress(h, "InstallLayoutOrTip"));
    if (pfn)
    {
        // 格式： "0x0804:{TIP_CLSID}{PROFILE_GUID}"
        std::wstring s = L"0x0804:" +
                         GuidToStr(CLSID_MinimalIME) +
                         GuidToStr(GUID_MinimalProfile);
        const DWORD ILOT_UNINSTALL = 0x00000001;
        pfn(s.c_str(), bInstall ? 0 : ILOT_UNINSTALL);
    }
    FreeLibrary(h);
}

HRESULT RegisterIme(BOOL bInstall)
{
    const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const BOOL needUninit = SUCCEEDED(hrCo);   // RPC_E_CHANGED_MODE 表示 COM 已初始化，别重复 Uninitialize

    const std::wstring clsidStr = GuidToStr(CLSID_MinimalIME);
    const std::wstring baseKey  = L"CLSID\\" + clsidStr;
    const std::wstring dllPath  = GetDllPath();
    HRESULT hr = S_OK;

    // ---- 卸载时先从用户键盘列表里撤掉条目（必须在撤销 TSF 注册之前）----
    if (!bInstall) EnableLayout(FALSE);

    if (bInstall)
    {
        // ---- 2. COM 进程内服务器 ----
        if (!WriteRegString(HKEY_CLASSES_ROOT, baseKey, nullptr, MINIMAL_IME_NAME) ||
            !WriteRegString(HKEY_CLASSES_ROOT, baseKey + L"\\InprocServer32",
                            nullptr, dllPath) ||
            !WriteRegString(HKEY_CLASSES_ROOT, baseKey + L"\\InprocServer32",
                            L"ThreadingModel", L"Apartment"))
            hr = E_ACCESSDENIED;

        // ---- 3. 向 TSF 注册 TIP 与语言配置文件 ----
        ITfInputProcessorProfiles* pProfiles = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITfInputProcessorProfiles,
                                       reinterpret_cast<void**>(&pProfiles))))
        {
            if (FAILED(pProfiles->Register(CLSID_MinimalIME)))
                hr = E_FAIL;

            const std::wstring desc = MINIMAL_IME_NAME;
            if (FAILED(pProfiles->AddLanguageProfile(
                    CLSID_MinimalIME,
                    MINIMAL_LANGID,                 // 0x0804 简体中文
                    GUID_MinimalProfile,
                    desc.c_str(), static_cast<ULONG>(desc.size()),
                    dllPath.c_str(), static_cast<ULONG>(dllPath.size()),
                    0)))                             // DLL 内第一个图标资源
                hr = E_FAIL;
            pProfiles->Release();
        }
        else hr = E_FAIL;

        // ---- 4. 注册类别：告诉系统这是一个键盘型 TIP ----
        ITfCategoryMgr* pCatMgr = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITfCategoryMgr,
                                       reinterpret_cast<void**>(&pCatMgr))))
        {
            pCatMgr->RegisterCategory(CLSID_MinimalIME,
                                      GUID_TFCAT_TIP_KEYBOARD,
                                      CLSID_MinimalIME);
            pCatMgr->Release();
        }

        // ---- 5. 持久化到键盘列表（重启后仍在，且出现在"设置"里）----
        if (SUCCEEDED(hr)) EnableLayout(TRUE);
    }
    else
    {
        ITfCategoryMgr* pCatMgr = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITfCategoryMgr,
                                       reinterpret_cast<void**>(&pCatMgr))))
        {
            pCatMgr->UnregisterCategory(CLSID_MinimalIME,
                                        GUID_TFCAT_TIP_KEYBOARD,
                                        CLSID_MinimalIME);
            pCatMgr->Release();
        }

        ITfInputProcessorProfiles* pProfiles = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_ITfInputProcessorProfiles,
                                       reinterpret_cast<void**>(&pProfiles))))
        {
            pProfiles->Unregister(CLSID_MinimalIME);  // 同时删掉所有语言配置
            pProfiles->Release();
        }
        RegDeleteTreeW(HKEY_CLASSES_ROOT, baseKey.c_str());
    }

    if (needUninit) CoUninitialize();
    return hr;
}
