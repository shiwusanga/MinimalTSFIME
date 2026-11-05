#include "TextService.h"
#include <new>
#include <utility>
#include <algorithm>

// ============================================================
//  一个"玩具词库"：足够演示 拼音串 -> 候选 -> 上屏 的完整链路。
//  换成真实词库时，只要替换 Convert() 的实现即可。
// ============================================================
using CandList = std::vector<std::wstring>;

static const std::vector<std::pair<std::wstring, CandList>>& Dict()
{
    static const std::vector<std::pair<std::wstring, CandList>> d = {
        { L"a",        { L"啊", L"阿", L"呵" } },
        { L"ai",       { L"爱", L"矮", L"哎", L"哀" } },
        { L"de",       { L"的", L"得", L"地" } },
        { L"e",        { L"额", L"俄", L"饿" } },
        { L"fa",       { L"法", L"发", L"罚" } },
        { L"hao",      { L"好", L"号", L"毫", L"豪" } },
        { L"jie",      { L"界", L"接", L"街", L"节" } },
        { L"ma",       { L"吗", L"妈", L"码", L"马" } },
        { L"ni",       { L"你", L"尼", L"泥", L"逆" } },
        { L"shi",      { L"是", L"时", L"事", L"十", L"使" } },
        { L"wen",      { L"文", L"问", L"闻", L"稳" } },
        { L"wo",       { L"我", L"握", L"窝" } },
        { L"zi",       { L"字", L"自", L"子", L"紫" } },
        { L"zhong",    { L"中", L"种", L"重", L"众" } },
        { L"ru",       { L"入", L"如", L"儒" } },
        { L"ceshi",    { L"测试" } },
        { L"nihao",    { L"你好" } },
        { L"shuru",    { L"输入" } },
        { L"shurufa",  { L"输入法" } },
        { L"zhongwen", { L"中文" } },
        { L"woaini",   { L"我爱你" } },
    };
    return d;
}

// 整串优先，否则最长前缀词 + 递归剩余音节；未识别的拼音原样作为"上屏文本"。
static CandList Convert(const std::wstring& s)
{
    if (s.empty()) return {};

    for (const auto& kv : Dict())
        if (kv.first == s) return kv.second;

    for (size_t len = s.size() - 1; len >= 1; --len)
    {
        std::wstring head = s.substr(0, len);
        std::wstring tail = s.substr(len);
        for (const auto& kv : Dict())
        {
            if (kv.first != head) continue;

            CandList rest = Convert(tail);
            CandList out;
            for (size_t i = 0; i < kv.second.size() && i < 3; ++i)
                for (size_t j = 0; j < rest.size() && j < 3; ++j)
                    out.push_back(kv.second[i] + rest[j]);
            for (const auto& word : kv.second)
                if (out.size() < 9) out.push_back(word + tail);

            // 去重
            CandList uniq;
            for (const auto& x : out)
                if (std::find(uniq.begin(), uniq.end(), x) == uniq.end())
                    uniq.push_back(x);
            if (uniq.size() > 9) uniq.resize(9);
            return uniq;
        }
    }
    return { s };
}

// ============================================================
//  IUnknown
// ============================================================
CTextService::CTextService()  { DllAddRef(); }
CTextService::~CTextService() { _ClearInputState(); DllRelease(); }

STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown)
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    else if (riid == IID_ITfTextInputProcessor)
        *ppv = static_cast<ITfTextInputProcessor*>(this);
    else if (riid == IID_ITfTextInputProcessorEx)
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    else if (riid == IID_ITfThreadMgrEventSink)
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    else if (riid == IID_ITfKeyEventSink)
        *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (riid == IID_ITfCompositionSink)
        *ppv = static_cast<ITfCompositionSink*>(this);
    else
    {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTextService::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&_cRef));
}

STDMETHODIMP_(ULONG) CTextService::Release()
{
    LONG c = InterlockedDecrement(&_cRef);
    if (c == 0) delete this;
    return static_cast<ULONG>(c);
}

// ============================================================
//  激活 / 反激活：向 TSF 注册各种事件接收器
// ============================================================
STDMETHODIMP CTextService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    return ActivateEx(ptim, tid, 0);
}

STDMETHODIMP CTextService::ActivateEx(ITfThreadMgr* ptim, TfClientId tid, DWORD)
{
    if (!ptim) return E_INVALIDARG;
    _pThreadMgr = ptim;
    _pThreadMgr->AddRef();
    _tid = tid;

    ITfSource* pSource = nullptr;
    if (SUCCEEDED(ptim->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&pSource))))
    {
        pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                            static_cast<ITfThreadMgrEventSink*>(this),
                            &_dwThreadMgrCookie);
        pSource->Release();
    }

    ITfKeystrokeMgr* pKM = nullptr;
    if (SUCCEEDED(ptim->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&pKM))))
    {
        pKM->AdviseKeyEventSink(_tid, static_cast<ITfKeyEventSink*>(this), TRUE);
        pKM->Release();
    }

    _cand.EnsureCreated(g_hInst);
    return S_OK;
}

STDMETHODIMP CTextService::Deactivate()
{
    _ClearInputState();

    if (_pThreadMgr)
    {
        ITfSource* pSource = nullptr;
        if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource,
                         reinterpret_cast<void**>(&pSource))))
        {
            if (_dwThreadMgrCookie) pSource->UnadviseSink(_dwThreadMgrCookie);
            pSource->Release();
        }
        ITfKeystrokeMgr* pKM = nullptr;
        if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr,
                         reinterpret_cast<void**>(&pKM))))
        {
            pKM->UnadviseKeyEventSink(_tid);
            pKM->Release();
        }
        _pThreadMgr->Release();
        _pThreadMgr = nullptr;
    }
    _cand.Hide();
    if (_pContext) { _pContext->Release(); _pContext = nullptr; }
    return S_OK;
}

// ============================================================
//  ITfThreadMgrEventSink
// ============================================================
STDMETHODIMP CTextService::OnInitDocumentMgr(ITfDocumentMgr*) { return S_OK; }
STDMETHODIMP CTextService::OnUninitDocumentMgr(ITfDocumentMgr*) { return S_OK; }
STDMETHODIMP CTextService::OnPushContext(ITfContext*) { return S_OK; }
STDMETHODIMP CTextService::OnPopContext(ITfContext*) { return S_OK; }

STDMETHODIMP CTextService::OnSetFocus(ITfDocumentMgr* pDimFocus, ITfDocumentMgr* pDimPrev)
{
    if (pDimFocus != pDimPrev) _ClearInputState();
    return S_OK;
}

// ============================================================
//  ITfCompositionSink：应用主动终止组合时清理状态
// ============================================================
STDMETHODIMP CTextService::OnCompositionTerminated(TfClientId, ITfComposition*)
{
    if (_pComposition)
    {
        _pComposition->Release();
        _pComposition = nullptr;
    }
    _ClearInputUI();
    return S_OK;
}

// ============================================================
//  编辑会话投递：按键回调里不能直接改文档，只能投异步会话
// ============================================================
HRESULT CTextService::_PostEditSession(ES_Action action, const std::wstring& text)
{
    if (!_pContext) return E_FAIL;

    CEditSession* pSession =
        new (std::nothrow) CEditSession(this, _pContext, action, text);
    if (!pSession) return E_OUTOFMEMORY;

    HRESULT hrSession = S_OK;
    HRESULT hr = _pContext->RequestEditSession(
        _tid, pSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hrSession);
    pSession->Release();      // RequestEditSession 内部会自行 AddRef
    return FAILED(hr) ? hr : hrSession;
}

// 在编辑会话里执行：组合串的创建 / 更新 / 结束全部串行化
void CTextService::ApplyState(TfEditCookie ec, ITfContext* pCtx,
                              ES_Action action, const std::wstring& text)
{
    switch (action)
    {
    case ES_Action::Start:
        if (_pComposition) { _pComposition->Release(); _pComposition = nullptr; }
        _StartComposition(ec, pCtx, text);
        break;

    case ES_Action::Update:
        if (!_pComposition) { _StartComposition(ec, pCtx, text); break; }
        _SetCompositionText(ec, text);
        break;

    case ES_Action::Commit:
    case ES_Action::Cancel:
        if (!_pComposition) break;
        _SetCompositionText(ec, action == ES_Action::Commit ? text : std::wstring());
        _pComposition->EndComposition(ec);
        // EndComposition 通常会回调 OnCompositionTerminated 清掉指针；
        // 万一没回调，这里兜底释放。
        if (_pComposition) { _pComposition->Release(); _pComposition = nullptr; }
        break;
    }
}

void CTextService::_StartComposition(TfEditCookie ec, ITfContext* pCtx,
                                     const std::wstring& text)
{
    TF_SELECTION sel = {};
    ULONG fetched = 0;
    if (FAILED(pCtx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &fetched)) ||
        !sel.range)
        return;

    ITfRange* pRange = sel.range;     // GetSelection 已 AddRef，由我们释放
    pRange->Collapse(ec, TF_ANCHOR_END);

    ITfContextComposition* pCC = nullptr;
    if (FAILED(pCtx->QueryInterface(IID_ITfContextComposition,
                                    reinterpret_cast<void**>(&pCC))))
    {
        pRange->Release();
        return;
    }

    ITfComposition* pComp = nullptr;
    HRESULT hr = pCC->StartComposition(ec, pRange,
                                       static_cast<ITfCompositionSink*>(this), &pComp);
    pCC->Release();
    pRange->Release();

    if (SUCCEEDED(hr) && pComp)
    {
        _pComposition = pComp;        // 已持有引用
        _SetCompositionText(ec, text);
    }
}

void CTextService::_SetCompositionText(TfEditCookie ec, const std::wstring& text)
{
    if (!_pComposition) return;
    ITfRange* pRange = nullptr;
    if (SUCCEEDED(_pComposition->GetRange(&pRange)))
    {
        pRange->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.size()));
        pRange->Release();
    }
}

// ============================================================
//  输入状态机
// ============================================================
void CTextService::_RememberContext(ITfContext* pCtx)
{
    if (pCtx == _pContext) return;
    if (_pContext) _pContext->Release();
    _pContext = pCtx;
    if (_pContext) _pContext->AddRef();
}

void CTextService::_RebuildCandidates()
{
    _cands = Convert(_pinyin);
    _sel = 0;
    _cand.Update(_pinyin, _cands, _sel);
    _cand.ShowNearCaret();
}

void CTextService::_Start()
{
    _PostEditSession(ES_Action::Start, _pinyin);
}

void CTextService::_Update()
{
    _PostEditSession(ES_Action::Update, _pinyin);
}

void CTextService::_CommitIndex(int index)
{
    std::wstring out = (index >= 0 && index < static_cast<int>(_cands.size()))
                           ? _cands[index] : _pinyin;
    _PostEditSession(ES_Action::Commit, out);
    _ClearInputUI();
}

void CTextService::_Cancel()
{
    _PostEditSession(ES_Action::Cancel, std::wstring());
    _ClearInputUI();
}

void CTextService::_ClearInputUI()
{
    _pinyin.clear();
    _cands.clear();
    _sel = 0;
    _cand.Hide();
}

void CTextService::_ClearInputState()
{
    if (_pComposition && _pContext)
        _PostEditSession(ES_Action::Cancel, std::wstring());
    _ClearInputUI();
}
