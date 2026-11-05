#include "Globals.h"
#include "Register.h"
#include "TextService.h"
#include <objbase.h>
#include <new>

// ------------------------------------------------------------
//  COM 类工厂：TSF 通过 DllGetClassObject 拿到它，再创建 CTextService。
// ------------------------------------------------------------
class CClassFactory : public IClassFactory
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&_cRef));
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        LONG c = InterlockedDecrement(&_cRef);
        if (c == 0) delete this;
        return static_cast<ULONG>(c);
    }
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj) override
    {
        if (!ppvObj) return E_POINTER;
        *ppvObj = nullptr;
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;

        CTextService* p = new (std::nothrow) CTextService();
        if (!p) return E_OUTOFMEMORY;
        HRESULT hr = p->QueryInterface(riid, ppvObj);
        p->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL fLock) override
    {
        if (fLock) DllAddRef(); else DllRelease();
        return S_OK;
    }
private:
    LONG _cRef = 1;
};

// ------------------------------------------------------------
//  DLL 入口与四个标准导出（导出表见 MinimalTSFIME.def）
//  注意：DllMain 里绝不要调 COM / LoadLibrary，只做最轻量的事。
// ------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    if (!IsEqualCLSID(rclsid, CLSID_MinimalIME))
        return CLASS_E_CLASSNOTAVAILABLE;

    CClassFactory* p = new (std::nothrow) CClassFactory();
    if (!p) return E_OUTOFMEMORY;
    HRESULT hr = p->QueryInterface(riid, ppv);
    p->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer()
{
    return RegisterIme(TRUE);
}

STDAPI DllUnregisterServer()
{
    return RegisterIme(FALSE);
}
