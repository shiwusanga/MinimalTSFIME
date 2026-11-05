#include "EditSession.h"
#include "TextService.h"

CEditSession::CEditSession(CTextService* pTS, ITfContext* pContext,
                           ES_Action action, const std::wstring& text)
    : _pTS(pTS), _pContext(pContext), _action(action), _text(text)
{
    if (_pContext) _pContext->AddRef();
    DllAddRef();
}

CEditSession::~CEditSession()
{
    if (_pContext) _pContext->Release();
    DllRelease();
}

STDMETHODIMP CEditSession::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ITfEditSession)
    {
        *ppv = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSession::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&_cRef));
}

STDMETHODIMP_(ULONG) CEditSession::Release()
{
    LONG c = InterlockedDecrement(&_cRef);
    if (c == 0) delete this;
    return static_cast<ULONG>(c);
}

STDMETHODIMP CEditSession::DoEditSession(TfEditCookie ec)
{
    if (_pTS && _pContext)
        _pTS->ApplyState(ec, _pContext, _action, _text);
    return S_OK;
}
