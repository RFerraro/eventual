$conanUrl = "http://downloads.conan.io/latest_windows"
$conanInstallerPath = [System.IO.Path]::Combine($Env:USERPROFILE, "Downloads", "conan_installer.exe")
$conanInstallPath = [System.IO.Path]::Combine(${Env:ProgramFiles(x86)}, "Conan", "conan")
$conanExe = [System.IO.Path]::Combine($conanInstallPath, "conan.exe")

if(-Not (Test-Path $conanInstallerPath))
{
    Write-Host -ForegroundColor White ("Downloading Conan from: " + $conanUrl)
    Start-FileDownload $conanUrl -FileName $conanInstallerPath
}

Write-Host -ForegroundColor White "Installing Conan..."

# Pipe the output so powershell waits for completion
(& $conanInstallerPath /VERYSILENT) | Out-Null

$env:Path="$env:Path;$conanInstallPath"

while(-Not (Test-Path $conanExe))
{
    Start-Sleep -s 1
}

Write-Host -ForegroundColor White "Conan Installed"

(& $conanExe --version) | Write-Host -ForegroundColor White
