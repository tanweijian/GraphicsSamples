#pragma once

#include "Window.h"

class Application
{
public:
    struct Desc
    {
        const wchar_t* Title = L"D3D12 Application";
        int Width = 1280;
        int Height = 720;
        bool Resizable = true;
    };

    Application() = default;
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize(const Desc& desc, HINSTANCE hInstance);
    int Run();

protected:
    virtual bool OnInit() { return true; }
    virtual void OnShutdown() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    virtual void OnResize(int width, int height) { (void)width; (void)height; }
    virtual void OnDpiChanged(float dpiScale) { (void)dpiScale; }

    // 窗口事件处理
    virtual void OnWindowEvent(WindowEvent event, const WindowEventData& data);

    Window& GetWindow() { return m_window; }
    int GetWidth() const { return m_window.GetWidth(); }
    int GetHeight() const { return m_window.GetHeight(); }

private:
    Window m_window;
};
