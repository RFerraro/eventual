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

$installProcess = (Start-Process $conanInstallerPath -ArgumentList '/VERYSILENT' -PassThru -Wait)
if($installProcess.ExitCode -ne 0)
{
    throw [System.String]::Format("Failed to install the Conan Package Manager, ExitCode: {0}.", $installProcess.ExitCode)
}

$env:Path="$env:Path;$conanInstallPath"

Write-Host -ForegroundColor White "Conan Installed"

(& $conanExe --version) | Write-Host -ForegroundColor White

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Failed to check the Conan Package Manager version, ExitCode: {0}.", $LASTEXITCODE)
}
