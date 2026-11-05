#include "TextService.h"

static BOOL IsLetter(WPARAM w) { return w >= 0x41 && w <= 0x5A; }   // VK_A..VK_Z
static BOOL IsDigit(WPARAM w)  { return w >= 0x31 && w <= 0x39; }   // 主键盘 1..9
static BOOL IsModifier(WPARAM w)
{
    return w == VK_SHIFT || w == VK_CONTROL || w == VK_MENU ||
           w == VK_LWIN  || w == VK_RWIN   || w == VK_CAPITAL ||
           w == VK_LSHIFT || w == VK_RSHIFT || w == VK_LCONTROL ||
           w == VK_RCONTROL || w == VK_LMENU || w == VK_RMENU;
}

// 返回 TRUE 表示这个键被输入法吃掉。
// bTestOnly=TRUE 时只做判断、绝不改状态（OnTestKeyDown 的要求）。
BOOL CTextService::_HandleKey(ITfContext* pCtx, WPARAM w, BOOL bTestOnly, BOOL* pfEaten)
{
    *pfEaten = FALSE;
    if (!pCtx) return FALSE;

    // Ctrl / Alt / Win 组合键一律放行，别抢系统快捷键
    if ((GetKeyState(VK_CONTROL) & 0x8000) ||
        (GetKeyState(VK_MENU) & 0x8000) ||
        (GetKeyState(VK_LWIN) & 0x8000) ||
        (GetKeyState(VK_RWIN) & 0x8000))
        return FALSE;

    if (IsModifier(w)) return FALSE;

    const BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const BOOL composing = !_pinyin.empty();

    int digit = -1;
    if (IsDigit(w)) digit = static_cast<int>(w - 0x30) - 1;
    else if (w >= VK_NUMPAD1 && w <= VK_NUMPAD9) digit = static_cast<int>(w - VK_NUMPAD1);

    if (composing)
    {
        if (w == VK_ESCAPE)
        {
            *pfEaten = TRUE;
            if (!bTestOnly) _Cancel();
        }
        else if (w == VK_BACK)
        {
            *pfEaten = TRUE;
            if (!bTestOnly)
            {
                if (!_pinyin.empty()) _pinyin.pop_back();
                if (_pinyin.empty()) _Cancel();
                else { _RebuildCandidates(); _Update(); }
            }
        }
        else if (w == VK_SPACE || w == VK_RETURN || digit >= 0)
        {
            *pfEaten = TRUE;
            if (!bTestOnly) _CommitIndex(digit >= 0 ? digit : 0);
        }
        else if (IsLetter(w) && !shift)
        {
            *pfEaten = TRUE;
            if (!bTestOnly)
            {
                _pinyin.push_back(static_cast<WCHAR>(w - 0x41 + L'a'));
                _RebuildCandidates();
                _Update();
            }
        }
        else
        {
            // 标点、方向键等：先把当前候选上屏，本键吃掉（示例简化处理）
            *pfEaten = TRUE;
            if (!bTestOnly) _CommitIndex(0);
        }
    }
    else
    {
        if (IsLetter(w) && !shift)
        {
            *pfEaten = TRUE;
            if (!bTestOnly)
            {
                _pinyin.assign(1, static_cast<WCHAR>(w - 0x41 + L'a'));
                _RebuildCandidates();
                _Start();
            }
        }
    }
    return *pfEaten ? TRUE : FALSE;
}

STDMETHODIMP CTextService::OnSetFocus(BOOL fForeground)
{
    if (!fForeground) _ClearInputState();
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* pCtx, WPARAM w, LPARAM, BOOL* pfEaten)
{
    if (!pfEaten) return E_POINTER;
    _HandleKey(pCtx, w, TRUE, pfEaten);
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* pCtx, WPARAM w, LPARAM, BOOL* pfEaten)
{
    if (!pfEaten) return E_POINTER;
    _RememberContext(pCtx);
    BOOL eaten = FALSE;
    _HandleKey(pCtx, w, FALSE, &eaten);
    *pfEaten = eaten;
    if (eaten) _lastEatenKey = w;
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext*, WPARAM w, LPARAM, BOOL* pfEaten)
{
    if (!pfEaten) return E_POINTER;
    // 与刚才吃掉的 keydown 配对，避免上屏后应用又收到一个空格/字母的 keyup
    *pfEaten = (w == _lastEatenKey && _lastEatenKey != 0) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext*, WPARAM w, LPARAM, BOOL* pfEaten)
{
    if (!pfEaten) return E_POINTER;
    *pfEaten = (w == _lastEatenKey && _lastEatenKey != 0) ? TRUE : FALSE;
    _lastEatenKey = 0;
    return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* pfEaten)
{
    if (!pfEaten) return E_POINTER;
    *pfEaten = FALSE;
    return S_OK;
}
