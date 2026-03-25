project "HelloTriangle"
    targetname "HelloTriangle"
    kind "WindowedApp"
    systemversion "latest"

    language "C++"
    cdialect "C17"
    cppdialect "C++20"

    files { "**.h", "**.hpp", "**.inl", "**.cpp", "**.cc" }

    vpaths
    {
        ["External/*"] = { _MAIN_SCRIPT_DIR .. "/External/**.*" },
        ["ThirdParty/*"] = {_MAIN_SCRIPT_DIR .. "/ThirdParty/**.*" }
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
