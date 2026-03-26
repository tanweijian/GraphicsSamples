project "HelloTriangle"
    targetname "HelloTriangle"
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

    links { "d3d12", "dxgi", "dxguid" }

    vpaths
    {
        ["External/*"] = { _MAIN_SCRIPT_DIR .. "/External/**.*" },
        ["ThirdParty/*"] = { _MAIN_SCRIPT_DIR .. "/ThirdParty/**.*" },
        ["Common/*"] = { _MAIN_SCRIPT_DIR .. "D3D12/Common/**.*" },
    }

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
