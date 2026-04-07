# Vulkan 模块

## 概述

Vulkan 渲染示例目录（开发中）。包含 Samples 示例代码。

```
Vulkan/
  premake5.lua    # 辅助函数定义
  Samples/        # 渲染示例
```

## 构建命令

```batch
:: 生成工程文件
..\GenerateProjectFiles.bat

:: 编译单个示例
msbuild Samples\HelloTriangle\HelloTriangle.vcxproj /p:Configuration=Development /p:Platform=x64

:: 编译整个解决方案
msbuild ..\GraphicsSamplesVulkan.slnx /p:Configuration=Development /p:Platform=x64
```

| 配置        | 验证层 | 优化 | 符号 |
|-------------|--------|------|------|
| Debug       | 启用   | 关闭 | 开启 |
| Development | 启用   | 开启 | 开启 |
| Release     | 关闭   | 开启 | 开启 |

## 示例模板

### Main.cpp

```cpp
#include <Windows.h>
#include <vulkan/vulkan.h>

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    // 初始化 Vulkan
    // 创建窗口
    // 渲染循环
    // 清理资源
    return 0;
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
    files { "**.h", "**.hpp", "**.inl", "**.cpp", "**.cc" }
    VulkanSDK()
    Imgui()
    links { "vulkan-1" }
    filter "configurations:Debug"
        runtime "Debug" optimize "Off" symbols "On"
    filter "configurations:Development"
        runtime "Release" optimize "On" symbols "On"
    filter "configurations:Release"
        runtime "Release" optimize "On" symbols "On"
```

## 辅助函数 (premake5.lua)

| 函数          | 说明                              |
|---------------|-----------------------------------|
| `VulkanSDK()` | 引入 Vulkan SDK 头文件和链接库    |
| `Imgui()`     | 引入 imgui + Vulkan/Win32 backend |

## 代码规范

### 头文件包含顺序

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan.h>
```

### 命名约定

| 类型     | 风格        | 示例              |
|----------|-------------|-------------------|
| 类       | PascalCase  | `VulkanRenderer`  |
| 函数     | PascalCase  | `CreateInstance`  |
| 成员变量 | `m_` 前缀   | `m_instance`      |
| 全局变量 | `g_` 前缀   | `g_device`        |
| 常量     | UPPER_SNAKE | `MaxFramesInFlight` |

### 格式化

- Allman 大括号风格
- 4 空格缩进
- 逻辑块间空行分隔

### 错误处理

- 使用 `VkResult` 返回值检查操作结果
- 使用 `VK_SUCCESS` 判断成功
- 使用 `nullptr` 而非 `NULL`

### 资源管理

- Vulkan 句柄使用 RAII 包装
- 不可复制类型使用 `= delete`
- 常量优先使用 `static constexpr`

## 调试

建议在 Debug 和 Development 配置启用 Vulkan 验证层：

```cpp
VkValidationFeaturesEXT features = {};
// 配置验证层
```

## 依赖

- **External/VulkanSDK**: Vulkan SDK
- **ThirdParty/imgui**: GUI 库
- **ThirdParty/spdlog**: 日志库

## 状态

当前为占位状态，待完善。
