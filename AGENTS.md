# AGENTS.md

## Project Overview

C++ graphics programming samples targeting D3D12 and Vulkan on Windows x64.
Uses Premake5 to generate Visual Studio 2026 solutions.

## Repository Structure

```
GraphicsSamples/
  premake5.lua              # Root workspace definition (two workspaces)
  GenerateProjectFiles.bat  # Runs premake5 vs2026
  D3D12/Samples/            # D3D12 sample projects
  Vulkan/Samples/           # Vulkan sample projects
  External/                 # External SDKs (AgilitySDK)
  ThirdParty/               # Git submodules (spdlog, imgui, Sharpmake)
```

Each sample lives in `{API}/Samples/{SampleName}/` with `Main.cpp` (wWinMain entry) and `premake5.lua`.

## Build System

### Generate Solution

```batch
GenerateProjectFiles.bat
```

Runs `premake5.exe vs2026` to produce `.slnx`/`.vcxproj` files in `Projects/`.

### Configurations

| Config      | Runtime | Optimize | Symbols | Notes                     |
|-------------|---------|----------|---------|---------------------------|
| Debug       | Debug   | Off      | On      | Full debug, SDK debug layer DLLs |
| Development | Release | On       | On      | Optimized, SDK debug layer DLLs |
| Release     | Release | On       | On      | Final builds, no debug layer |

### Build Commands

```batch
:: Full solution (D3D12 or Vulkan)
msbuild Projects\GraphicsSamplesD3D12.slnx /p:Configuration=Development

:: Single project
msbuild Projects\D3D12\HelloTriangle.vcxproj /p:Configuration=Development
```

No `/p:Platform` needed — architecture is `x86_64` set in premake.

### Running Samples

Samples are `WindowedApp` kind. Output goes to `bin/` directory adjacent to the `.vcxproj`.
AgilitySDK DLLs are copied to `bin/{Config}/D3D12/` via post-build.

## Adding New Samples

1. Create directory: `{API}/Samples/{SampleName}/`
2. Add `Main.cpp` with `wWinMain` entry point and GPU vendor exports
3. Add `premake5.lua` (copy from existing sample, adjust `targetname`)
4. Call `{API}SDK()` function (e.g., `AgilitySDK()`) to link SDK
5. Run `GenerateProjectFiles.bat` to regenerate

### Per-Project premake5.lua Template

```lua
project "SampleName"
    targetname "SampleName"
    kind "WindowedApp"
    systemversion "latest"

    language "C++"
    cppdialect "C++20"

    files { "**.h", "**.hpp", "**.inl", "**.cpp", "**.cc" }
    AgilitySDK()

    filter "configurations:Debug"
        runtime "Debug"
        optimize "Off"
        symbols "On"

    filter "configurations:Development"
        runtime "Release"
        optimize "On"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        symbols "On"
```

## RuntimeDependency Function

Defined in `D3D12/premake5.lua`. Copies DLLs to output directory after build:

```lua
RuntimeDependency("External/AgilitySDK/Binaries/D3D12Core.dll", "D3D12")
```

- First arg: source path relative to workspace root
- Second arg: destination subdirectory under `$(OutDir)`
- Can be placed inside `filter` blocks for config-specific copying

## Dependencies

- **Git submodules**: `git submodule init && git submodule update`
- **ThirdParty/spdlog**: Logging library
- **ThirdParty/imgui**: Immediate-mode GUI
- **External/AgilitySDK**: D3D12 headers + binaries

## Code Style

### Language & Standard

- C++20 (`cppdialect "C++20"`)
- Windows-specific Win32 API usage is expected
- Use `ComPtr<T>` from `<wrl/client.h>` for COM objects

### Naming Conventions

- **Classes/Structs**: PascalCase (e.g., `Vertex`, `HelloTriangle`)
- **Functions**: PascalCase (e.g., `InitWindow`, `Render`, `PopulateCommandList`)
- **Global/static variables**: `g_` prefix (e.g., `g_device`, `g_swapChain`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `FrameCount`, `WindowWidth`)
- **Files**: PascalCase (e.g., `Main.cpp`)

### Includes

Order: Win32 → D3D12/DXGI → d3dx12 helpers → WRL → STL

```cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>
#include <wrl/client.h>
```

### D3D12 Entry Point Boilerplate

Every D3D12 sample must export GPU vendor flags and Agility SDK version:

```cpp
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef USING_D3D12_AGILITY_SDK
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
#endif
}
```

### Error Handling

- Return `false` from init functions on failure; check all COM `HRESULT` returns
- Use `FAILED()` macro for HRESULT checks
- Enable D3D12 debug layer in Debug/Development configurations

## Lint & Typecheck

No automated lint or typecheck tooling. Verify by building in Development config.
Target MSVC; warnings should be addressed, not suppressed.

## What NOT to Modify

- `ThirdParty/*` — Git submodules, read-only
- `External/AgilitySDK/*` — Vendored SDK
- Generated files (`*.slnx`, `*.vcxproj`, `*.filters`) — gitignored, regenerated by premake5
