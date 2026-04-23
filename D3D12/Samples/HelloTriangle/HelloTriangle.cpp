#include "HelloTriangle.h"

#include <d3dx12/d3dx12.h>
#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr char k_VertexShaderSource[] = R"(
cbuffer SceneConstants : register(b0)
{
    float4x4 ModelViewProjection;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), ModelViewProjection);
    output.color = input.color;
    return output;
}
)";

    constexpr char k_PixelShaderSource[] = R"(
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

    constexpr std::uint16_t k_TriangleIndices[] = { 0, 1, 2 };
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HelloTriangle::~HelloTriangle()
{
    if (m_constantBuffer)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_mappedSceneConstants = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

bool HelloTriangle::OnInit()
{
    if (!CreateDevice())
        return false;

    if (!CreateCommandObjects())
        return false;

    if (!CreateSwapChain())
        return false;

    if (!CreateRenderTargets())
        return false;

    if (!CreateRootSignature())
        return false;

    if (!CreatePipelineState())
        return false;

    if (!CreateVertexBuffer())
        return false;

    if (!CreateIndexBuffer())
        return false;

    if (!CreateConstantBuffer())
        return false;

    if (!CreateSynchronizationObjects())
        return false;

    if (!CreateImgui())
        return false;

    UpdateSceneConstants();
    m_lastFrameTime = std::chrono::steady_clock::now();
    return true;
}

void HelloTriangle::OnShutdown()
{
    WaitForGpu();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void HelloTriangle::OnUpdate()
{
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<float> delta = now - m_lastFrameTime;
    m_lastFrameTime = now;

    if (m_animate)
        m_rotationAngle += delta.count() * m_rotationSpeed;

    UpdateSceneConstants();
}

void HelloTriangle::OnRender()
{
    if (!m_swapChain)
        return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    BuildImgui();
    ImGui::Render();

    PopulateCommandList();

    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    if (SUCCEEDED(m_swapChain->Present(1, 0)))
        WaitForGpu();
}

void HelloTriangle::OnResize(int width, int height)
{
    if (!m_swapChain || width <= 0 || height <= 0)
        return;

    ResizeSwapChain(width, height);
}

bool HelloTriangle::CreateDevice()
{
    UINT factoryFlags = 0;

#if defined(_DEBUG) || defined(DEVELOPMENT)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
    {
        debugInterface->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory))))
        return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
            break;

        adapter.Reset();
    }

    if (adapter)
    {
        if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
            return false;
    }
    else
    {
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
            return false;
    }

    return true;
}

bool HelloTriangle::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
        return false;

    if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator))))
        return false;

    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList))))
        return false;

    if (FAILED(m_commandList->Close()))
        return false;

    return true;
}

bool HelloTriangle::CreateSwapChain()
{
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = static_cast<UINT>(GetWidth());
    swapChainDesc.Height = static_cast<UINT>(GetHeight());
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    if (FAILED(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        GetWindow().GetHandle(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain)))
    {
        return false;
    }

    if (FAILED(factory->MakeWindowAssociation(GetWindow().GetHandle(), DXGI_MWA_NO_ALT_ENTER)))
        return false;

    if (FAILED(swapChain.As(&m_swapChain)))
        return false;

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool HelloTriangle::CreateRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap))))
        return false;

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
            return false;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(GetWidth());
    m_viewport.Height = static_cast<float>(GetHeight());
    m_viewport.MinDepth = D3D12_MIN_DEPTH;
    m_viewport.MaxDepth = D3D12_MAX_DEPTH;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = GetWidth();
    m_scissorRect.bottom = GetHeight();

    return true;
}

bool HelloTriangle::CreateSynchronizationObjects()
{
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
        return false;

    return true;
}

bool HelloTriangle::CreateRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsConstantBufferView(0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        _countof(rootParameters),
        rootParameters,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error)))
    {
        return false;
    }

    if (FAILED(m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature))))
    {
        return false;
    }

    return true;
}

bool HelloTriangle::CreatePipelineState()
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG) || defined(DEVELOPMENT)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
    Microsoft::WRL::ComPtr<ID3DBlob> error;

    if (FAILED(D3DCompile(
        k_VertexShaderSource,
        sizeof(k_VertexShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        compileFlags,
        0,
        &vertexShader,
        &error)))
    {
        return false;
    }

    error.Reset();
    if (FAILED(D3DCompile(
        k_PixelShaderSource,
        sizeof(k_PixelShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        compileFlags,
        0,
        &pixelShader,
        &error)))
    {
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, Position)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, Color)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState))))
        return false;

    return true;
}

bool HelloTriangle::CreateVertexBuffer()
{
    const Vertex triangleVertices[] =
    {
        { { 0.0f, 0.35f, 0.0f }, { 1.0f, 0.2f, 0.2f, 1.0f } },
        { { 0.3f, -0.25f, 0.0f }, { 0.2f, 1.0f, 0.2f, 1.0f } },
        { { -0.3f, -0.25f, 0.0f }, { 0.2f, 0.4f, 1.0f, 1.0f } }
    };

    const UINT vertexBufferSize = sizeof(triangleVertices);

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    if (FAILED(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer))))
    {
        return false;
    }

    UINT8* vertexDataBegin = nullptr;
    const D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin))))
        return false;

    memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    return true;
}

bool HelloTriangle::CreateIndexBuffer()
{
    const UINT indexBufferSize = sizeof(k_TriangleIndices);

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    if (FAILED(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBuffer))))
    {
        return false;
    }

    UINT8* indexDataBegin = nullptr;
    const D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin))))
        return false;

    memcpy(indexDataBegin, k_TriangleIndices, sizeof(k_TriangleIndices));
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.SizeInBytes = indexBufferSize;
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

bool HelloTriangle::CreateConstantBuffer()
{
    const UINT constantBufferSize = (sizeof(SceneConstants) + 255u) & ~255u;

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    if (FAILED(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer))))
    {
        return false;
    }

    const D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedSceneConstants))))
        return false;

    return true;
}

bool HelloTriangle::CreateImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_imguiSrvHeap))))
        return false;

    if (!ImGui_ImplWin32_Init(GetWindow().GetHandle()))
        return false;

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = m_device.Get();
    initInfo.CommandQueue = m_commandQueue.Get();
    initInfo.NumFramesInFlight = FrameCount;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = m_imguiSrvHeap.Get();
    initInfo.LegacySingleSrvCpuDescriptor = m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
    initInfo.LegacySingleSrvGpuDescriptor = m_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart();

    if (!ImGui_ImplDX12_Init(&initInfo))
        return false;

    return true;
}

bool HelloTriangle::ResizeSwapChain(int width, int height)
{
    WaitForGpu();
    ImGui_ImplDX12_InvalidateDeviceObjects();

    for (UINT i = 0; i < FrameCount; ++i)
    {
        m_renderTargets[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    if (FAILED(m_swapChain->GetDesc(&swapChainDesc)))
        return false;

    if (FAILED(m_swapChain->ResizeBuffers(
        FrameCount,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        swapChainDesc.BufferDesc.Format,
        swapChainDesc.Flags)))
    {
        return false;
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
            return false;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_scissorRect.right = width;
    m_scissorRect.bottom = height;

    return ImGui_ImplDX12_CreateDeviceObjects();
}

void HelloTriangle::UpdateSceneConstants()
{
    using namespace DirectX;

    if (!m_mappedSceneConstants)
        return;

    const float aspectRatio = GetHeight() > 0 ? static_cast<float>(GetWidth()) / static_cast<float>(GetHeight()) : 1.0f;
    const XMMATRIX model = XMMatrixRotationZ(m_rotationAngle);
    const XMMATRIX view = XMMatrixIdentity();
    const XMMATRIX projection = XMMatrixScaling(1.0f / aspectRatio, 1.0f, 1.0f);
    const XMMATRIX mvp = XMMatrixTranspose(model * view * projection);

    XMStoreFloat4x4(&m_mappedSceneConstants->ModelViewProjection, mvp);
}

void HelloTriangle::BuildImgui()
{
    ImGui::Begin("HelloTriangle");
    ImGui::Text("Resolution: %d x %d", GetWidth(), GetHeight());
    ImGui::Checkbox("Animate", &m_animate);
    ImGui::SliderFloat("Rotation Speed", &m_rotationSpeed, -6.0f, 6.0f, "%.2f rad/s");
    ImGui::ColorEdit3("Clear Color", m_clearColor);

    if (ImGui::Button("Reset View"))
    {
        m_rotationAngle = 0.0f;
        m_rotationSpeed = 1.0f;
        m_clearColor[0] = 0.08f;
        m_clearColor[1] = 0.12f;
        m_clearColor[2] = 0.18f;
    }

    ImGui::Text("Angle: %.2f rad", m_rotationAngle);
    ImGui::End();
}

void HelloTriangle::PopulateCommandList()
{
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<INT>(m_frameIndex),
        m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    m_commandList->DrawIndexedInstanced(_countof(k_TriangleIndices), 1, 0, 0, 0);

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_imguiSrvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->Close();
}

void HelloTriangle::WaitForGpu()
{
    if (!m_commandQueue || !m_fence || !m_fenceEvent)
        return;

    if (FAILED(m_commandQueue->Signal(m_fence.Get(), m_fenceValue)))
        return;

    if (FAILED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent)))
        return;

    WaitForSingleObject(m_fenceEvent, INFINITE);
    if (m_swapChain)
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    ++m_fenceValue;
}
