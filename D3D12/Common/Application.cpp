#include "Application.h"

Application::~Application()
{
}

bool Application::Initialize(const Desc& desc, HINSTANCE hInstance)
{
    if (!m_window.Create({ desc.Title, desc.Width, desc.Height, desc.Resizable }, hInstance, this))
        return false;

    if (!OnInit())
        return false;

    return true;
}

int Application::Run()
{
    while (m_window.PollMessage())
    {
        OnUpdate();
        OnRender();
    }

    OnShutdown();
    return 0;
}

void Application::OnWindowEvent(WindowEvent event, const WindowEventData& data)
{
    switch (event)
    {
    case WindowEvent::Resize:
        OnResize(data.Width, data.Height);
        break;

    case WindowEvent::DpiChanged:
        OnDpiChanged(data.DpiScale);
        break;

    case WindowEvent::Close:
        break;

    default:
        break;
    }
}
