function VulkanSDK()
    includedirs (_MAIN_SCRIPT_DIR .. "/External/VulkanSDK/Include")
    libdirs (_MAIN_SCRIPT_DIR .. "/External/VulkanSDK/Binaries")
    links { "vulkan-1" }
end

function Imgui()
    includedirs (_MAIN_SCRIPT_DIR .. "/ThirdParty/imgui")
    files
    {
        _MAIN_SCRIPT_DIR .. "/ThirdParty/imgui/*.h", _MAIN_SCRIPT_DIR ..  "/ThirdParty/imgui/*.cpp", 
        _MAIN_SCRIPT_DIR ..  "/ThirdParty/imgui/backends/imgui_impl_vulkan.*", _MAIN_SCRIPT_DIR ..  "/ThirdParty/imgui/backends/imgui_impl_win32.*"
    }
end

for _, dir in ipairs(os.matchdirs("Samples/*")) do
    include (dir)
end
