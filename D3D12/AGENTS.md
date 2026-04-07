# D3D12 模块

## 概述

D3D12 渲染示例目录，包含 Common 公共工具和 Samples 示例代码。

```
D3D12/
  premake5.lua    # 辅助函数定义
  Common/         # 公共工具类 (Window, Application)
  Samples/        # 渲染示例
```

## 构建命令

```batch
:: 生成工程文件
..\GenerateProjectFiles.bat

:: 编译单个示例
msbuild Samples\HelloTriangle\HelloTriangle.vcxproj /p:Configuration=Development /p:Platform=x64

:: 编译整个解决方案
msbuild ..\GraphicsSamplesD3D12.slnx /p:Configuration=Development /p:Platform=x64
```

| 配置        | 调试层 | 优化 | 符号 |
|-------------|--------|------|------|
| Debug       | 启用   | 关闭 | 开启 |
| Development | 启用   | 开启 | 开启 |
| Release     | 关闭   | 开启 | 开启 |

## 公共工具

### Application 基类

继承 `Application` 类创建示例：

```cpp
class MySample : public Application
{
protected:
    bool OnInit() override;      // 初始化 D3D12 资源
    void OnShutdown() override;  // 清理资源
    void OnRender() override;    // 每帧渲染
    void OnResize(int w, int h) override;  // 窗口大小变化
};
```

### Window 类

Win32 窗口封装，支持 DPI 感知。通过 `PollMessage()` 驱动消息循环。

## 示例模板

### Main.cpp

```cpp
#include "MySample.h"

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
    MySample app;
    if (!app.Initialize({ L"My Sample", 1280, 720 }, hInstance))
        return 1;
    return app.Run();
}
```

### premake5.lua

```lua
project "MySample"
    targetname "MySample"
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

## 辅助函数 (premake5.lua)

| 函数                   | 说明                                        |
|------------------------|---------------------------------------------|
| `AgilitySDK()`         | 引入 AgilitySDK，复制 DLL，定义宏          |
| `WinPixEventRuntime()` | 引入 PIX，复制 DLL (Debug/Development)     |
| `Imgui()`              | 引入 imgui + DX12/Win32 backend            |
| `RuntimeDependency()`  | 复制运行时依赖到输出目录                    |

## 代码规范

### 头文件包含顺序

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <wrl/client.h>
```

### 命名约定

| 类型     | 风格        | 示例                |
|----------|-------------|---------------------|
| 类       | PascalCase  | `HelloTriangle`     |
| 函数     | PascalCase  | `CreateDevice`      |
| 成员变量 | `m_` 前缀   | `m_device`          |
| 全局变量 | `g_` 前缀   | `g_windowRefCount`  |
| 常量     | UPPER_SNAKE | `FrameCount`        |

### 格式化

- Allman 大括号风格
- 4 空格缩进
- 逻辑块间空行分隔

### 错误处理

- 初始化函数返回 `bool`，失败返回 `false`
- 使用 `FAILED()` 宏检查 `HRESULT`
- 使用 `nullptr` 而非 `NULL`

### 资源管理

- COM 对象使用 `Microsoft::WRL::ComPtr<T>`
- 不可复制类型使用 `= delete`
- 常量优先使用 `static constexpr`

## 调试

Debug 和 Development 配置自动启用 D3D12 调试层，用于检测 API 误用。

## 依赖

- **External/AgilitySDK**: D3D12 SDK
- **External/WinPixEventRuntime**: PIX 性能分析
- **ThirdParty/imgui**: GUI 库
- **ThirdParty/spdlog**: 日志库
