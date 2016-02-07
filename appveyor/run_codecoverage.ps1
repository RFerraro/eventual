$testExe = "test.exe"
$includePath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "include", "eventual")
$testPath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "bin", $Env:PLATFORM, $Env:Configuration)
$testCoveragePath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "coverage_results")
$testExeFullPath = [System.IO.Path]::Combine($testPath, $testExe)
$testConfiguration = ($env:CONFIGURATION + "-" + $env:PLATFORM)
$coberturaFile = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "coverage_results.xml")

Write-Host -ForegroundColor White ("Running Code Coverage For: " + $testExeFullPath)

# Redirect stderr, as OpenCppCoverage misuses it for informational messages
(& OpenCppCoverage.exe --export_type html:$testCoveragePath --export_type cobertura:$coberturaFile --modules $testPath --sources $includePath -- $testExeFullPath) 2>&1 | Write-Verbose 

while(-Not (Test-Path $coberturaFile))
{
    Start-Sleep -s 1
}

Write-Host -ForegroundColor White "Coverage analysis complete, pushing coverage file."

$coverageName = ("test_coverage(" + $testConfiguration + ")")
$coverageArchive = ($coverageName + ".zip")
(& 7z a -tzip $coverageArchive $testCoveragePath) | Write-Verbose 
Push-AppveyorArtifact $coverageArchive -DeploymentName $testConfiguration
Push-AppveyorArtifact $coberturaFile -DeploymentName $testConfiguration
