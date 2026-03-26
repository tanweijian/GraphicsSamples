function VulkanSDK()
    includedirs (_MAIN_SCRIPT_DIR .. "/External/VulkanSDK/Include")
    libdirs (_MAIN_SCRIPT_DIR .. "/External/VulkanSDK/Binaries")
    links { "vulkan-1" }
end

for _, dir in ipairs(os.matchdirs("Samples/*")) do
    include (dir)
end
