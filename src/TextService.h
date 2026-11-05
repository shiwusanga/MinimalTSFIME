#pragma once

#include "Globals.h"
#include "CandidateWindow.h"
#include "EditSession.h"
#include <msctf.h>

// 一个最小但真实可用的 TSF 文本服务（TIP）。
class CTextService
    : public ITfTextInputProcessorEx
    , public ITfThreadMgrEventSink
    , public ITfKeyEventSink
    , public ITfCompositionSink
{
public:
    CTextService();
    virtual ~CTextService();

    // ---- IUnknown ----
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ---- ITfTextInputProcessor / Ex ----
    STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
    STDMETHODIMP ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD dwFlags) override;
    STDMETHODIMP Deactivate() override;

    // ---- ITfThreadMgrEventSink ----
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDim) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDim) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDimFocus, ITfDocumentMgr* pDimPrev) override;
    STDMETHODIMP OnPushContext(ITfContext* pCtx) override;
    STDMETHODIMP OnPopContext(ITfContext* pCtx) override;

    // ---- ITfKeyEventSink ----
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pCtx, WPARAM w, LPARAM l, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pCtx, WPARAM w, LPARAM l, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pCtx, WPARAM w, LPARAM l, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pCtx, WPARAM w, LPARAM l, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pCtx, REFGUID rguid, BOOL* pfEaten) override;

    // ---- ITfCompositionSink ----
    STDMETHODIMP OnCompositionTerminated(TfClientId tidWrite, ITfComposition* pComp) override;

    // 由 CEditSession 在编辑会话内调用：所有文档改动都集中在这里，顺序安全。
    void ApplyState(TfEditCookie ec, ITfContext* pCtx,
                    ES_Action action, const std::wstring& text);

private:
    HRESULT _PostEditSession(ES_Action action, const std::wstring& text);
    void    _StartComposition(TfEditCookie ec, ITfContext* pCtx, const std::wstring& text);
    void    _SetCompositionText(TfEditCookie ec, const std::wstring& text);

    void    _Start();
    void    _Update();
    void    _CommitIndex(int index);
    void    _Cancel();
    void    _RebuildCandidates();
    void    _ClearInputUI();
    void    _ClearInputState();
    BOOL    _HandleKey(ITfContext* pCtx, WPARAM w, BOOL bTestOnly, BOOL* pfEaten);
    void    _RememberContext(ITfContext* pCtx);

private:
    LONG                _cRef = 1;
    ITfThreadMgr*       _pThreadMgr = nullptr;
    TfClientId          _tid = 0;
    DWORD               _dwThreadMgrCookie = 0;
    ITfComposition*     _pComposition = nullptr;
    ITfContext*         _pContext = nullptr;

    CCandidateWindow    _cand;
    std::wstring        _pinyin;      // 当前拼音串（组合串里显示的文本）
    std::vector<std::wstring> _cands;
    int                 _sel = 0;
    WPARAM              _lastEatenKey = 0;  // 与 keyup 配对，避免上屏后漏一个空格
};
