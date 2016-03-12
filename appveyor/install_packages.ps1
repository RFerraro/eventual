$buildFolder = ".\msvc"
$buildCompilerOS = "Windows"
$buildCompiler = "Visual Studio"
$buildCompilerVersion = 14

$buildPlatform = "Undefined"
$buildConfiguration = "Undefined"
$buildCompilerRuntime = "Undefined"

if($env:PLATFORM -eq "Win32")
{
    $buildPlatform ="x86"
}
if($env:PLATFORM -eq "x64")
{
    $buildPlatform ="x86_64"
}
if($env:CONFIGURATION -eq "Debug")
{
    $buildConfiguration = "Debug"
    $buildCompilerRuntime = "MTd"
}
if($env:CONFIGURATION -eq "Release")
{
    $buildConfiguration = "Release"
    $buildCompilerRuntime = "MT"
}

Write-Host -ForegroundColor White ("Installing Conan packages for: " + $buildConfiguration  + "/" + $buildPlatform + "/" + $buildCompilerRuntime)
Write-Host -ForegroundColor White (&conan install $buildFolder -s arch=$buildPlatform -s build_type=$buildConfiguration -s compiler=$buildCompiler -s compiler.version=$buildCompilerVersion -s compiler.runtime=$buildCompilerRuntime -s os=$buildCompilerOS)

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Conan failed to install all the project dependencies, ExitCode: {0}.", $LASTEXITCODE) 
}
