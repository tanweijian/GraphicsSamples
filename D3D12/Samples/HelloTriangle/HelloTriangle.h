#pragma once

#include <Common/Application.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <chrono>

class HelloTriangle : public Application
{
public:
    static constexpr UINT FrameCount = 2;

    HelloTriangle() = default;
    ~HelloTriangle();

    HelloTriangle(const HelloTriangle&) = delete;
    HelloTriangle& operator=(const HelloTriangle&) = delete;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnResize(int width, int height) override;

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain();
    bool CreateRenderTargets();
    bool CreateSynchronizationObjects();
    bool CreateRootSignature();
    bool CreatePipelineState();
    bool CreateVertexBuffer();
    bool CreateIndexBuffer();
    bool CreateConstantBuffer();
    bool CreateImgui();
    bool ResizeSwapChain(int width, int height);
    void UpdateSceneConstants();
    void BuildImgui();
    void PopulateCommandList();
    void WaitForGpu();

    struct Vertex
    {
        float Position[3];
        float Color[4];
    };

    struct alignas(256) SceneConstants
    {
        DirectX::XMFLOAT4X4 ModelViewProjection;
    };

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};
    SceneConstants* m_mappedSceneConstants = nullptr;
    UINT m_rtvDescriptorSize = 0;
    UINT m_frameIndex = 0;
    UINT64 m_fenceValue = 1;
    HANDLE m_fenceEvent = nullptr;
    float m_clearColor[4] = { 0.08f, 0.12f, 0.18f, 1.0f };
    float m_rotationAngle = 0.0f;
    float m_rotationSpeed = 1.0f;
    bool m_animate = true;
    std::chrono::steady_clock::time_point m_lastFrameTime = std::chrono::steady_clock::now();
};
