#include "Window.h"
#include "Application.h"

static constexpr wchar_t k_WindowClassName[] = L"GraphicsSampleWindow";
static int g_windowRefCount = 0;

Window::~Window()
{
    Destroy();
}

bool Window::Create(const Desc& desc, HINSTANCE hInstance, Application* app)
{
    m_app = app;

    if (g_windowRefCount == 0)
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = StaticWndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = k_WindowClassName;

        if (!RegisterClassExW(&wc))
            return false;
    }

    m_width = desc.Width;
    m_height = desc.Height;

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.Resizable)
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rc = { 0, 0, m_width, m_height };
    AdjustWindowRect(&rc, style, FALSE);

    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;

    m_hWnd = CreateWindowExW(
        0,
        k_WindowClassName,
        desc.Title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hWnd)
        return false;

    UpdateDpiScale();
    m_isRunning = true;
    ++g_windowRefCount;

    ShowWindow(m_hWnd, SW_SHOW);
    return true;
}

void Window::Destroy()
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        m_isRunning = false;
        --g_windowRefCount;

        if (g_windowRefCount == 0)
        {
            HINSTANCE hInstance = GetModuleHandle(nullptr);
            UnregisterClassW(k_WindowClassName, hInstance);
        }
    }
}

bool Window::PollMessage()
{
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

LRESULT CALLBACK Window::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* window = static_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hWnd = hWnd;
    }

    auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (window)
        return window->WndProc(hWnd, msg, wParam, lParam);

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);
        WindowEventData data;
        data.Event = WindowEvent::Resize;
        data.Width = m_width;
        data.Height = m_height;
        DispatchEvent(WindowEvent::Resize, data);
        return 0;
    }

    case WM_DPICHANGED:
    {
        UINT dpi = HIWORD(wParam);
        m_dpiScale = static_cast<float>(dpi) / 96.0f;

        auto* suggestedRect = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hWnd, nullptr,
            suggestedRect->left, suggestedRect->top,
            suggestedRect->right - suggestedRect->left,
            suggestedRect->bottom - suggestedRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);

        WindowEventData data;
        data.Event = WindowEvent::DpiChanged;
        data.DpiScale = m_dpiScale;
        DispatchEvent(WindowEvent::DpiChanged, data);
        return 0;
    }

    case WM_CLOSE:
    {
        DispatchEvent(WindowEvent::Close, {});
        m_isRunning = false;
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Window::UpdateDpiScale()
{
    UINT dpi = GetDpiForWindow(m_hWnd);
    if (dpi == 0)
        dpi = 96;
    m_dpiScale = static_cast<float>(dpi) / 96.0f;
}

void Window::DispatchEvent(WindowEvent event, const WindowEventData& data)
{
    if (m_app)
    {
        m_app->OnWindowEvent(event, data);
    }
}
