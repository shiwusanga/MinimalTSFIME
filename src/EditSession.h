#pragma once

#include "Globals.h"
#include <msctf.h>

class CTextService;

// ------------------------------------------------------------
// TSF 规定：所有对文档/组合串的读写必须发生在"编辑会话"里。
// 按键回调禁止同步改写文档，因此每次按键只投递一个异步会话。
// 同一 context 的会话按投递顺序执行，所以把动作快照塞进会话里，
// 就能保证组合串的创建/更新/结束严格串行，不会出现竞态。
// ------------------------------------------------------------
enum class ES_Action { Start, Update, Commit, Cancel };

class CEditSession : public ITfEditSession
{
public:
    CEditSession(CTextService* pTS, ITfContext* pContext,
                 ES_Action action, const std::wstring& text);
    virtual ~CEditSession();

    // IUnknown
    STDMETHODIMP            QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG)    AddRef() override;
    STDMETHODIMP_(ULONG)    Release() override;

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    LONG            _cRef = 1;
    CTextService*   _pTS = nullptr;      // 不加引用：TSF 卸载时会先撤销文本服务
    ITfContext*     _pContext = nullptr; // AddRef 保活
    ES_Action       _action;
    std::wstring    _text;
};
