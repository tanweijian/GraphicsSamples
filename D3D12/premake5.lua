-- Copy a runtime dependency to the output subdirectory after build
function RuntimeDependency(sourceRelPath, destSubdir)
    local src = path.getabsolute(path.join(_MAIN_SCRIPT_DIR, sourceRelPath))
    local dst = path.join("$(OutDir)", destSubdir)
    postbuildcommands {
        '{MKDIR} "' .. dst .. '"',
        '{COPYFILE} "' .. src .. '" "' .. path.join(dst, path.getname(sourceRelPath)) .. '"'
    }
end

function AgilitySDK()
    includedirs (_MAIN_SCRIPT_DIR.."/External/AgilitySDK/Include")
    includedirs (_MAIN_SCRIPT_DIR.."/External/AgilitySDK/Include/d3dx12")
    files { _MAIN_SCRIPT_DIR .. "/External/AgilitySDK/Source/**.cpp" }
    RuntimeDependency("External/AgilitySDK/Binaries/D3D12Core.dll", "D3D12")
    filter "configurations:Debug"
        RuntimeDependency("External/AgilitySDK/Binaries/D3D12Core.pdb", "D3D12")
        RuntimeDependency("External/AgilitySDK/Binaries/d3d12SDKLayers.dll", "D3D12")
        RuntimeDependency("External/AgilitySDK/Binaries/d3d12SDKLayers.pdb", "D3D12")
    filter "configurations:Development"
        RuntimeDependency("External/AgilitySDK/Binaries/d3d12SDKLayers.dll", "D3D12")
    filter {}
    defines { "USING_D3D12_AGILITY_SDK" }
end

for _, dir in ipairs(os.matchdirs("Samples/*")) do
        include (dir)
end