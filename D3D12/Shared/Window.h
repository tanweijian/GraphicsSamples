#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Window
{
public:
    struct Desc
    {
        const wchar_t* Title = L"Graphics Sample";
        int Width = 1280;
        int Height = 720;
        bool Resizable = true;
    };

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const Desc& desc, HINSTANCE hInstance);
    void Destroy();

    bool PollMessage();

    HWND GetHandle() const { return m_hWnd; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetDpiScale() const { return m_dpiScale; }

private:
    static LRESULT CALLBACK StaticWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
    void UpdateDpiScale();

    HWND m_hWnd = nullptr;
    int m_width = 1280;
    int m_height = 720;
    float m_dpiScale = 1.0f;
    bool m_isRunning = false;
};
