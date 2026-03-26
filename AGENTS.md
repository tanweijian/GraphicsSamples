# AGENTS.md

## Project Overview

C++ graphics samples for D3D12 and Vulkan on Windows x64. Uses Premake5 to generate VS2026 solutions.

## Repository Structure

```
GraphicsSamples/
  premake5.lua              # Root workspace definition (2 workspaces: D3D12, Vulkan)
  GenerateProjectFiles.bat  # Runs premake5 vs2026
  D3D12/                    # D3D12 samples + Common utilities
  Vulkan/                   # Vulkan samples (stubbed, WIP)
  External/                 # SDKs (AgilitySDK, WinPixEventRuntime, VulkanSDK)
  ThirdParty/               # Git submodules (spdlog, imgui)
```

Each sample: `{API}/Samples/{SampleName}/Main.cpp` + `premake5.lua`.

## Build System

### Generate & Build

```batch
GenerateProjectFiles.bat   # Regenerate .vcxproj files alongside source

:: Single project (requires /p:Platform=x64)
msbuild D3D12\Samples\HelloTriangle\HelloTriangle.vcxproj /p:Configuration=Development /p:Platform=x64

:: Full solution
msbuild GraphicsSamplesD3D12.slnx /p:Configuration=Development /p:Platform=x64
```

### Configurations

| Config      | Runtime | Optimize | Symbols | Notes                     |
|-------------|---------|----------|---------|---------------------------|
| Debug       | Debug   | Off      | On      | Full debug, SDK debug layer |
| Development | Release | On       | On      | Optimized, SDK debug layer |
| Release     | Release | On       | On      | Final builds, no debug layer |

### Running Samples

Samples are `WindowedApp`. Output to `bin/{Config}/` adjacent to `.vcxproj`. AgilitySDK DLLs copied to `bin/{Config}/D3D12/` via post-build.

## Testing

No unit tests. Verify by:
1. Build in Development config
2. Run visually — check rendering output
3. Enable D3D12 debug layer (Debug/Development) to catch API misuse

## Lint & Typecheck

No automated tools. Verify by building in Development config:
```batch
msbuild D3D12\Samples\HelloTriangle\HelloTriangle.vcxproj /p:Configuration=Development /p:Platform=x64
```
Target MSVC; address warnings, don't suppress.

## Common Utilities

`D3D12/Common/Window.h` / `Window.cpp` — Win32 window wrapper:
- Per-monitor DPI aware v2
- External message loop via `PollMessage()` (returns `false` on `WM_QUIT`)
- `Desc` struct for configuration (title, size, resizable)

```cpp
#include <Window.h>
Window window;
if (!window.Create({ L"Sample", 1280, 720 }, hInstance)) return 1;
while (window.PollMessage()) { /* render */ }
```

Include in sample's `premake5.lua`:
```lua
includedirs (_MAIN_SCRIPT_DIR .. "/D3D12")
files { _MAIN_SCRIPT_DIR .. "/D3D12/Common/**.h", _MAIN_SCRIPT_DIR .. "/D3D12/Common/**.cpp" }
```

## Adding New Samples

1. Create `{API}/Samples/{SampleName}/`
2. Add `Main.cpp` with `wWinMain` entry + GPU vendor exports (D3D12 only)
3. Add `premake5.lua` (copy existing, adjust `targetname`)
4. Call helper functions: `AgilitySDK()`, `Imgui()`, include Common files
5. Run `GenerateProjectFiles.bat`

### premake5.lua Template (D3D12)

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
    links { "d3d12", "dxgi", "dxguid" }
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

## Helper Functions (D3D12/premake5.lua)

- `AgilitySDK()` — includes AgilitySDK headers/source, copies DLLs post-build, defines `USING_D3D12_AGILITY_SDK`
- `Imgui()` — includes imgui + DX12/Win32 backends
- `WinPixEventRuntime()` — includes PIX headers/libs, copies DLL post-build, defines `USE_PIX` (Debug/Development only)
- `RuntimeDependency(src, dest)` — copies file to `$(OutDir)/dest` post-build

## Dependencies

- `git submodule init && git submodule update`
- **ThirdParty/spdlog**: Logging
- **ThirdParty/imgui**: Immediate-mode GUI
- **External/AgilitySDK**: D3D12 headers + binaries
- **External/WinPixEventRuntime**: GPU profiling (Debug/Development)

## Code Style

### Language & Standard

- C++20 (`cppdialect "C++20"`), C17 for C files (`cdialect "C17"`)
- Windows Win32 API; use `ComPtr<T>` from `<wrl/client.h>` for COM objects
- Prefer `#pragma once` for include guards

### Naming Conventions

| Element              | Style        | Example                          |
|----------------------|--------------|----------------------------------|
| Classes/Structs      | PascalCase   | `Vertex`, `Window`, `HelloTriangle` |
| Functions            | PascalCase   | `InitWindow`, `Render`, `PollMessage` |
| Globals/statics      | `g_` prefix  | `g_device`, `g_windowRefCount`   |
| Constants            | UPPER_SNAKE  | `FrameCount`, `WindowWidth`, `k_WindowClassName` |
| Files                | PascalCase   | `Main.cpp`, `Window.h`           |
| Member variables     | `m_` prefix  | `m_width`, `m_hWnd`, `m_dpiScale` |

### Formatting

- Allman brace style (opening brace on its own line)
- 4-space indentation
- Blank line between logical sections

### Include Order

Win32 → D3D12/DXGI → d3dx12 → WRL → STL

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <wrl/client.h>
```

### D3D12 Entry Boilerplate

Every D3D12 sample must export GPU vendor flags and Agility SDK version:

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
```

### Error Handling

- Return `false` from init functions on failure; callers must check returns
- Check all COM `HRESULT` returns with `FAILED()` macro
- Enable D3D12 debug layer in Debug/Development configs
- Use `nullptr` (not `NULL` or `0`) for pointer initialization

### Resource Management

- COM objects: `Microsoft::WRL::ComPtr<T>`, call `.ReleaseAndGetAddressOf()` for out-params
- Non-copyable types: `= delete` copy constructor and assignment operator
- Prefer `static constexpr` over `#define` for constants

## Do NOT Modify

- `ThirdParty/*` — Git submodules (read-only)
- `External/AgilitySDK/*` — Vendored SDK
- `External/WinPixEventRuntime/*` — Vendored SDK
- Generated files (`*.vcxproj`, `*.filters`, `*.slnx`) — regenerated by premake5
