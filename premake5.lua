workspace "GraphicsSamplesD3D12"
    configurations { "Debug", "Development", "Release" }

    architecture "x86_64"
    system "Windows"

    include("D3D12")

workspace "GraphicsSamplesVulkan"
    configurations { "Debug", "Development", "Release" }

    architecture "x86_64"
    system "Windows"

    include("Vulkan")