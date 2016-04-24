if($env:PLATFORM -eq "Win32")
{
    Write-Host -ForegroundColor White (& cmake -Bbuild -H"." -G "Visual Studio 14 2015")
}
elseif($env:PLATFORM -eq "x64")
{
    Write-Host -ForegroundColor White (& cmake -Bbuild -H"." -G "Visual Studio 14 2015 Win64")
}
else
{
    throw "Platform Not Supported"
}

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("CMake failed to generate the Visual Studio solution, ExitCode: {0}.", $LASTEXITCODE) 
}