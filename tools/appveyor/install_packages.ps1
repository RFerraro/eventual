$buildCompilerOS = "Windows"
$buildCompiler = "Visual Studio"
$buildCompilerVersion = 14

$buildPlatform = "Undefined"

if($env:PLATFORM -eq "Win32")
{
    $buildPlatform ="x86"
}
if($env:PLATFORM -eq "x64")
{
    $buildPlatform ="x86_64"
}

#Write-Host -ForegroundColor White ("Installing Conan packages for: " + $buildConfiguration  + "/" + $buildPlatform + "/" + $buildCompilerRuntime)
Write-Host -ForegroundColor White ("Installing Conan packages for: " + $buildPlatform)
Write-Host -ForegroundColor White (&conan install $buildFolder -s arch=$buildPlatform -s build_type=Debug -s compiler=$buildCompiler -s compiler.version=$buildCompilerVersion -s compiler.runtime=MTd -s os=$buildCompilerOS -build=missing)
Move-Item .\conanbuildinfo.cmake .\conanbuildinfo_debug_$buildPlatform.cmake -Force
Write-Host -ForegroundColor White (&conan install $buildFolder -s arch=$buildPlatform -s build_type=Release -s compiler=$buildCompiler -s compiler.version=$buildCompilerVersion -s compiler.runtime=MT -s os=$buildCompilerOS -build=missing)
Move-Item .\conanbuildinfo.cmake .\conanbuildinfo_release_$buildPlatform.cmake -Force

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Conan failed to install all the project dependencies, ExitCode: {0}.", $LASTEXITCODE) 
}
