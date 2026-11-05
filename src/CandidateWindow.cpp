#include "CandidateWindow.h"
#include <algorithm>
#include <new>

static const WCHAR* kCandClass = L"MinimalTSFIME_Candidate";

CCandidateWindow::CCandidateWindow()
{
    _font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                        L"Microsoft YaHei");
}

CCandidateWindow::~CCandidateWindow()
{
    if (_font) DeleteObject(_font);
    if (_hwnd) DestroyWindow(_hwnd);
}

LRESULT CALLBACK CCandidateWindow::WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    CCandidateWindow* self =
        reinterpret_cast<CCandidateWindow*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        if (self) self->Paint(dc);
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    return DefWindowProcW(h, m, w, l);
}

void CCandidateWindow::EnsureCreated(HINSTANCE hInst)
{
    if (_hwnd) return;
    _hInst = hInst;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kCandClass;
    RegisterClassExW(&wc);   // 同类多实例注册失败可忽略

    _hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                            kCandClass, L"", WS_POPUP | WS_DISABLED,
                            0, 0, 320, 140, nullptr, nullptr, hInst, nullptr);
    if (_hwnd) SetWindowLongPtrW(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void CCandidateWindow::Update(const std::wstring& pinyin,
                              const std::vector<std::wstring>& candidates,
                              int selected)
{
    _pinyin = pinyin;
    _cands  = candidates;
    _sel    = selected;
    if (!_hwnd) return;

    if (_cands.size() > 9) _cands.resize(9);
    const int rows = 1 + static_cast<int>(_cands.size());
    SetWindowPos(_hwnd, nullptr, 0, 0, 320, rows * 22 + 12,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(_hwnd, nullptr, TRUE);
}

void CCandidateWindow::ShowNearCaret()
{
    if (!_hwnd) return;
    if (_cands.empty()) { Hide(); return; }

    POINT pt = { 0, 0 };
    HWND fg = GetForegroundWindow();
    if (fg)
    {
        GetCaretPos(&pt);                    // 本 DLL 就跑在应用进程/线程内
        ClientToScreen(fg, &pt);
    }
    else
    {
        GetCursorPos(&pt);
    }
    pt.y += 24;

    RECT rc;
    GetWindowRect(_hwnd, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    const int sw = GetSystemMetrics(SM_CXSCREEN);
    const int sh = GetSystemMetrics(SM_CYSCREEN);
    if (pt.x + w > sw) pt.x = sw - w - 4;
    if (pt.y + h > sh) pt.y = pt.y - h - 44;
    if (pt.x < 0) pt.x = 0;
    if (pt.y < 0) pt.y = 0;

    SetWindowPos(_hwnd, HWND_TOPMOST, pt.x, pt.y, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CCandidateWindow::Hide()
{
    if (_hwnd) ShowWindow(_hwnd, SW_HIDE);
}

void CCandidateWindow::Paint(HDC hdc)
{
    RECT rc;
    GetClientRect(_hwnd, &rc);
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH border = CreateSolidBrush(RGB(0, 120, 215));
    HBRUSH hi     = CreateSolidBrush(RGB(220, 236, 255));
    FillRect(hdc, &rc, white);
    FrameRect(hdc, &rc, border);

    HGDIOBJ oldFont = SelectObject(hdc, _font);
    SetBkMode(hdc, TRANSPARENT);

    std::wstring head = _pinyin + L"    空格/1 上屏 · Esc 取消";
    SetTextColor(hdc, RGB(90, 90, 90));
    TextOutW(hdc, 8, 5, head.c_str(), static_cast<int>(head.size()));

    for (size_t i = 0; i < _cands.size(); ++i)
    {
        const int y = 5 + 22 * (static_cast<int>(i) + 1);
        RECT line = { 4, y - 1, rc.right - 4, y + 21 };
        if (static_cast<int>(i) == _sel) FillRect(hdc, &line, hi);
        std::wstring s = std::to_wstring(i + 1) + L". " + _cands[i];
        SetTextColor(hdc, static_cast<int>(i) == _sel ? RGB(0, 90, 180) : RGB(20, 20, 20));
        TextOutW(hdc, 10, y, s.c_str(), static_cast<int>(s.size()));
    }

    SelectObject(hdc, oldFont);
    DeleteObject(white);
    DeleteObject(border);
    DeleteObject(hi);
}
