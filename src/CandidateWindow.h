#pragma once

#include <windows.h>
#include <string>
#include <vector>

// 极简候选窗：一个不抢焦点的置顶弹窗，GDI 自绘。
// 真实产品里应换成 ITfCandidateListUIElement + DirectWrite + UIA 支持。
class CCandidateWindow
{
public:
    CCandidateWindow();
    ~CCandidateWindow();

    void EnsureCreated(HINSTANCE hInst);
    void Update(const std::wstring& pinyin,
                const std::vector<std::wstring>& candidates,
                int selected);
    void ShowNearCaret();
    void Hide();

private:
    static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l);
    void Paint(HDC hdc);

    HWND  _hwnd = nullptr;
    HINSTANCE _hInst = nullptr;
    std::wstring _pinyin;
    std::vector<std::wstring> _cands;
    int   _sel = 0;
    HFONT _font = nullptr;
};
