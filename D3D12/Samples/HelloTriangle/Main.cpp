#include <d3dx12/d3dx12.h>
#include "Shared/Window.h"

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef USING_D3D12_AGILITY_SDK
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
#endif
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    Window window;
    if (!window.Create({ L"Hello Triangle", 1280, 720 }, hInstance))
        return 1;

    while (window.PollMessage())
    {
        // TODO: render frame
    }

    return 0;
}
