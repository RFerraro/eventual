#Set-PSDebug -Trace 1

#$RepoPath = Split-Path (Split-Path $PSScriptRoot -Parent)
$RepoPath = Split-Path $PSScriptRoot -Parent
$SourcePath="/usr/src/eventual/src"
$BuildPath="/usr/src/eventual/build"
$LocalConanData="$env:userprofile\.conan\data"
$ConanData="/tmp/conan"

$BuildScript="$SourcePath/tools/docker/build-project.sh"

function Test-Image
{
    param( 
        [string]$DockerTag,
        [string]$Configuration
        )

    Write-Host "running $DockerTag with configuration $Configuration"

    &docker run `
        -it `
        --rm `
        --privileged `
        -v ${RepoPath}:${SourcePath}:ro `
        -v ${LocalConanData}:${ConanData} `
        -e CONFIGURATION="$Configuration" `
        $DockerTag bash -ci "$BuildScript $BuildPath"

}

if(!(Test-Path $LocalConanData))
{
    New-Item -ItemType Directory -Force -Path $LocalConanData
}

Test-Image "rferraro/cxx-clang:3.7" "Debug"
Test-Image "rferraro/cxx-clang:3.8" "Debug"
Test-Image "rferraro/cxx-clang:3.9" "Debug"
Test-Image "rferraro/cxx-clang:4.0" "Debug"
Test-Image "rferraro/cxx-clang:5.0" "Debug"

Test-Image "rferraro/cxx-clang:3.7" "Release"
Test-Image "rferraro/cxx-clang:3.8" "Release"
Test-Image "rferraro/cxx-clang:3.9" "Release"
Test-Image "rferraro/cxx-clang:4.0" "Release"
Test-Image "rferraro/cxx-clang:5.0" "Release"

Test-Image "rferraro/cxx-gcc:5.x" "Debug"
Test-Image "rferraro/cxx-gcc:6.x" "Debug"
Test-Image "rferraro/cxx-gcc:7.x" "Debug"

Test-Image "rferraro/cxx-gcc:5.x" "Release"
Test-Image "rferraro/cxx-gcc:6.x" "Release"
Test-Image "rferraro/cxx-gcc:7.x" "Release"

&docker images