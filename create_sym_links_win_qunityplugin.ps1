# 'qunitysdk sym link'
Write-Host 'Creating qunitysdk symlink'

# Remove existing symlink if it exists
Remove-Item -Path './qunityplugin/qunityplugin_unity_proj/Assets/Plugins/qunitysdk' -Recurse -Force

# Change directory to the Plugins directory
Set-Location './qunityplugin/qunityplugin_unity_proj/Assets/Plugins'

# Create the symlink for qunitysdk if it doesn't exist
if (!(Test-Path './qunitysdk')) {
    New-Item -ItemType SymbolicLink -Name 'qunitysdk' -Target './../../qunitysdk_module/source'
}

# Remove and recreate the libs directory
Remove-Item -Path './qunitysdk/libs' -Recurse -Force
New-Item -ItemType Directory -Path './qunitysdk/libs'

# Change directory to the libs directory
Set-Location './qunitysdk/libs'

# D:\workshop\gearsdk\qunityplugin\qunityplugin_unity_proj\qunitysdk_module\libs\windows
# Create symbolic links for platform-specific libraries if they don't exist
if (!(Test-Path './windows')) {
    New-Item -ItemType SymbolicLink -Name 'windows' -Target './../../../../../qunityplugin_unity_proj/qunitysdk_module/libs/windows'
}

# Create symbolic links for platform-specific libraries if they don't exist
if (!(Test-Path './android')) {
    New-Item -ItemType SymbolicLink -Name 'android' -Target './../../../../../qunityplugin_unity_proj/qunitysdk_module/libs/android'
}

# if (!(Test-Path './macos')) {
#     # Assuming $arch is set to the appropriate architecture (e.g., x64, x86)
#     $arch = 'x64'  # Set the architecture (or detect it dynamically)
#     New-Item -ItemType SymbolicLink -Name 'macos' -Target './../../../../../qunitysdk_module/libs/macos_' + $arch
# }

# if (!(Test-Path './linux')) {
#     New-Item -ItemType SymbolicLink -Name 'linux' -Target './../../../../../qunitysdk_module/libs/linux'
# }

# Return to the original directory
Set-Location '../../../../../../'

Write-Host 'Symlink script finished.'
