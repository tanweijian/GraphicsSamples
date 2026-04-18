# AGENTS.md

## 项目概述

C++ 图形渲染示例，支持 D3D12 和 Vulkan，Windows x64 平台。使用 Premake5 生成 VS2026 解决方案。

```
GraphicsSamples/
  premake5.lua              # 根工作区定义
  GenerateProjectFiles.bat  # 运行 premake5 vs2026
  D3D12/                    # D3D12 示例 + 公共工具
  Vulkan/                   # Vulkan 示例（开发中）
  External/                 # SDK（AgilitySDK, WinPixEventRuntime, VulkanSDK）
  ThirdParty/               # Git 子模块（spdlog, imgui）
```

每个示例：`{API}/Samples/{SampleName}/Main.cpp` + `premake5.lua`

## 构建命令

```batch
GenerateProjectFiles.bat   # 重新生成 .vcxproj 文件

:: 编译单个示例（需要 /p:Platform=x64）
msbuild D3D12\Samples\HelloTriangle\HelloTriangle.vcxproj /p:Configuration=Development /p:Platform=x64

:: 编译完整解决方案
msbuild GraphicsSamplesD3D12.slnx /p:Configuration=Development /p:Platform=x64
```

| 配置        | 调试层   | 优化 | 符号 |
|-------------|----------|------|------|
| Debug       | 启用     | 关闭 | 开启 |
| Development | 启用     | 开启 | 开启 |
| Release     | 关闭     | 开启 | 开启 |

输出目录：`.vcxproj` 同级的 `bin/{Config}/`，AgilitySDK DLL 复制到 `bin/{Config}/D3D12/`

## 测试与验证

无单元测试。验证方式：
1. 使用 Development 配置编译
2. 运行并检查渲染输出
3. D3D12 调试层（Debug/Development）检测 API 误用

## Lint 与类型检查

无自动化工具。使用 Development 配置编译，处理所有 MSVC 警告，不要抑制。

## 代码风格

### 语言与标准

- C++20，C 文件使用 C17
- Windows Win32 API，COM 对象使用 `<wrl/client.h>` 的 `ComPtr<T>`
- 使用 `#pragma once` 作为头文件保护

### 命名约定

| 元素         | 风格        | 示例                          |
|--------------|-------------|-------------------------------|
| 类/结构体    | PascalCase  | `Vertex`, `Window`, `HelloTriangle` |
| 函数         | PascalCase  | `InitWindow`, `Render`, `PollMessage` |
| 全局/静态变量 | `g_` 前缀   | `g_device`, `g_windowRefCount` |
| 常量         | UPPER_SNAKE | `FrameCount`, `k_WindowClassName` |
| 成员变量     | `m_` 前缀   | `m_width`, `m_hWnd`, `m_dpiScale` |
| 文件         | PascalCase  | `Main.cpp`, `Window.h` |

### 格式化

- Allman 大括号风格（左大括号独占一行）
- 4 空格缩进
- 逻辑块之间空行分隔

### 头文件包含顺序

Win32 → D3D12/DXGI → d3dx12 → WRL → STL

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <wrl/client.h>
```

### 错误处理

- 初始化函数失败返回 `false`，调用者必须检查返回值
- 使用 `FAILED()` 宏检查 COM `HRESULT` 返回值
- 使用 `nullptr`（而非 `NULL` 或 `0`）初始化指针
- Debug/Development 配置启用 D3D12 调试层

### 资源管理

- COM 对象：`Microsoft::WRL::ComPtr<T>`，输出参数使用 `.ReleaseAndGetAddressOf()`
- 不可复制类型：`= delete` 拷贝构造和赋值运算符
- 常量优先使用 `static constexpr` 而非 `#define`

## D3D12 示例样板

每个 D3D12 示例必须在 Main.cpp 导出 GPU 厂商标志和 Agility SDK 版本：

```cpp
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef USING_D3D12_AGILITY_SDK
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
#endif
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    HelloTriangle app;
    if (!app.Initialize({ L"Hello Triangle", 1280, 720 }, hInstance))
        return 1;
    return app.Run();
}
```

## 公共工具

- `D3D12/Common/Window.h` — Win32 窗口封装，支持 DPI 感知 v2，`PollMessage()` 返回 `false` 表示 `WM_QUIT`
- `D3D12/Common/Application.h` — 基类，提供虚函数：`OnInit()`, `OnShutdown()`, `OnRender()`, `OnResize()`, `OnDpiChanged()`, `OnWindowEvent()`

## 添加新示例

1. 创建 `{API}/Samples/{SampleName}/`
2. 添加 `Main.cpp`，包含 `wWinMain` 入口和 GPU 厂商导出（仅 D3D12）
3. 添加 `premake5.lua`（复制现有示例，修改 `targetname`）
4. 调用辅助函数：`AgilitySDK()`, `Imgui()`, `WinPixEventRuntime()`
5. 运行 `GenerateProjectFiles.bat`

### premake5.lua 模板（D3D12）

```lua
project "SampleName"
    targetname "SampleName"
    kind "WindowedApp"
    systemversion "latest"
    language "C++"
    cdialect "C17"
    cppdialect "C++20"
    includedirs (_MAIN_SCRIPT_DIR .. "/D3D12")
    files { "**.h", "**.hpp", "**.inl", "**.cpp", "**.cc" }
    files { _MAIN_SCRIPT_DIR .. "/D3D12/Common/**.h", _MAIN_SCRIPT_DIR .. "/D3D12/Common/**.cpp" }
    Imgui()
    AgilitySDK()
    WinPixEventRuntime()
    links { "d3d12", "dxgi", "dxguid", "dwmapi" }
    vpaths {
        ["Common/*"] = { _MAIN_SCRIPT_DIR .. "/D3D12/Common/**.*" },
        ["External/*"] = { _MAIN_SCRIPT_DIR .. "/External/**.*" },
        ["ThirdParty/*"] = { _MAIN_SCRIPT_DIR .. "/ThirdParty/**.*" }
    }
    filter "configurations:Debug"
        runtime "Debug" optimize "Off" symbols "On"
    filter "configurations:Development"
        runtime "Release" optimize "On" symbols "On"
    filter "configurations:Release"
        runtime "Release" optimize "On" symbols "On"
```

## 辅助函数

| 函数                   | 说明                                              |
|------------------------|---------------------------------------------------|
| `AgilitySDK()`         | 引入 AgilitySDK，复制 DLL，定义 `USING_D3D12_AGILITY_SDK` |
| `Imgui()`              | 引入 imgui + DX12/Win32 或 Vulkan/Win32 backend  |
| `WinPixEventRuntime()` | 引入 PIX，复制 DLL（仅 Debug/Development），定义 `USE_PIX` |
| `RuntimeDependency()`  | 复制运行时依赖到输出目录                          |

## 依赖

```batch
git submodule init && git submodule update
```

- **ThirdParty/spdlog**: 日志库
- **ThirdParty/imgui**: 即时模式 GUI
- **External/AgilitySDK**: D3D12 头文件 + 二进制
- **External/WinPixEventRuntime**: GPU 性能分析（Debug/Development）
- **External/VulkanSDK**: Vulkan 头文件 + 二进制

## 禁止修改

- `ThirdParty/*` — Git 子模块（只读）
- `External/*` — 第三方 SDK
- 生成的文件（`*.vcxproj`, `*.filters`, `*.slnx`）— 由 premake5 重新生成
