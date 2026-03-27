#pragma once

#include <Common/Application.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

class HelloTriangle : public Application
{
public:
    HelloTriangle() = default;
    ~HelloTriangle();

    HelloTriangle(const HelloTriangle&) = delete;
    HelloTriangle& operator=(const HelloTriangle&) = delete;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnRender() override;

private:
    bool CreateDevice();
    bool CreateCommandObjects();

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
};
