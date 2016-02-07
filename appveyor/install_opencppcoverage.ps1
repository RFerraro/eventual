$downloadUrl = "http://download-codeplex.sec.s-msft.com/Download/Release?ProjectName=opencppcoverage&DownloadId=1539055&FileTime=130975366571670000&Build=21031"
$installerPath = [System.IO.Path]::Combine($Env:USERPROFILE, "Downloads", "OpenCppCoverageSetup.exe")
$installPath = [System.IO.Path]::Combine(${Env:ProgramFiles}, "OpenCppCoverage")
$openCppCoverageExe = [System.IO.Path]::Combine($installPath, "OpenCppCoverage.exe")

if(-Not (Test-Path $installerPath))
{
    Write-Host -ForegroundColor White ("Downloading OpenCppCoverage from: " + $downloadUrl)
    Start-FileDownload $downloadUrl -FileName $installerPath
}

Write-Host -ForegroundColor White "Installing OpenCppCoverage..."

# Pipe the output so powershell waits for completion
(& $installerPath /VERYSILENT) | Out-Null

$env:Path="$env:Path;$installPath"

while(-Not (Test-Path $openCppCoverageExe))
{
    Start-Sleep -s 1
}

# Redirect from stderr
(& $openCppCoverageExe -h) 2>&1 | Select -First 1 | Write-Host -ForegroundColor White
