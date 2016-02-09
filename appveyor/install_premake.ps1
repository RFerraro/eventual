$premakeUrl = "https://github.com/premake/premake-core/releases/download/v5.0.0-alpha8/premake-5.0.0-alpha8-windows.zip"
$premakeZipPath = [System.IO.Path]::Combine($Env:USERPROFILE, "Downloads", "premake-5.0.0-alpha8-windows.zip")
$premakeInstallPath = "C:\Premake5"
$premakeExe = [System.IO.Path]::Combine($premakeInstallPath, "premake5.exe")

if(-Not (Test-Path $premakeZipPath))
{
    Write-Host -ForegroundColor White ("Downloading Premake from: " + $premakeUrl)
    Start-FileDownload $premakeUrl -FileName $premakeZipPath
}

Write-Host -ForegroundColor White "Installing Premake..."

(&7z e $premakeZipPath ("-o" + $premakeInstallPath)) | Write-Verbose
if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Failed to unzip the Premake Package , ExitCode: {0}.", $LASTEXITCODE)
}

$env:Path="$env:Path;$premakeInstallPath"

Write-Host -ForegroundColor White "Premake Installed"

(& $premakeExe --version) | Write-Host -ForegroundColor White

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Failed to check the Premake Package version, ExitCode: {0}.", $LASTEXITCODE)
}
