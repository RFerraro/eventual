$testStatusRegex = "^\[\s+(\w+)\s+\]\s+([\w\./]+)(?:\s\((\d+)[\w\s]+\))?$"
$testCaseLine = "^([\w/]+).+$"
$testNameLine = "^\s+(\w+)$"
$lastTestCase = "unknown"
$testExe = "test.exe"
$testPath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "bin", $Env:PLATFORM, $Env:Configuration, $testExe)
$includePath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "include", "eventual")
$testCoveragePath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "coverage_results")
$testConfiguration = ($env:CONFIGURATION + "-" + $env:PLATFORM)
$coberturaFile = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "coverage_results.xml")

function Read-Test-Case
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] $testCase)

    PROCESS
    {
        $result = [regex]::Match($testCase, $testCaseLine)
        if($result.Success)
        {
            $lastTestCase = $result.Groups[1].Value.Trim()
            return
        }

        $result = [regex]::Match($testCase, $testNameLine)
        if($result.Success)
        {
            Write-Output ($lastTestCase + "." + $result.Groups[1].Value)
        }
    }
}

function Read-Test-Status
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] $statusLine)   
 
    PROCESS
    {
        $result = [regex]::Match($statusLine, $testStatusRegex)
        if($result.Success)
        {
            
            $testStatus = [PSCustomObject]@{
                State = $result.Groups[1].Value
                Name = $result.Groups[2].Value
                Time = $result.Groups[3].Value
            }

            Write-Output $testStatus
        }
    }
}

function Create-AppVeyor-Test
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] $testName)

    PROCESS
    {
        Add-AppveyorTest -Name $testName.Trim() -FileName $testExe
    }
}

function Report-AppVeyor-Test-Status
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] $testStatus)

    PROCESS
    {
        if($testStatus.State -eq "RUN")
        {
            Update-AppveyorTest -Name $testStatus.Name.Trim() -Filename $testExe -Outcome Running -Duration ($testStatus.Time -as [int])
        }
        if($testStatus.State -eq "OK")
        {
            Update-AppveyorTest -Name $testStatus.Name.Trim() -Filename $testExe -Outcome Passed -Duration ($testStatus.Time -as [int])
        }
        if($testStatus.State -eq "FAILED")
        {
            Update-AppveyorTest -Name $testStatus.Name.Trim() -Filename $testExe -Outcome Failed -Duration ($testStatus.Time -as [int])
        }
    }
}

function Run-CodeCoverage
{
    PROCESS
    {
        Write-Host -ForegroundColor White ("Running Code Coverage For: " + $testPath)

        # Redirect stderr, as OpenCppCoverage misuses it for informational messages
        (& OpenCppCoverage.exe --export_type html:$testCoveragePath --export_type cobertura:$coberturaFile --modules $testPath --sources $includePath -- $testPath) 2>&1 | Write-Verbose 

        if($LASTEXITCODE -ne 0)
        {
            throw [System.String]::Format("Failed to generate code coverage, ExitCode: {0}.", $LASTEXITCODE)
        }

        while(-Not (Test-Path $coberturaFile))
        {
            Start-Sleep -s 1
        }

        Write-Host -ForegroundColor White "Coverage analysis complete, pushing coverage file."

        $coverageName = ("test_coverage(" + $testConfiguration + ")")
        $coverageArchive = ($coverageName + ".zip")
        (& 7z a -tzip $coverageArchive $testCoveragePath) | Write-Verbose 

        if($LASTEXITCODE -ne 0)
        {
            Write-Error [System.String]::Format("Failed to generate code coverage artifact, ExitCode: {0}.", $LASTEXITCODE)
        }

        Push-AppveyorArtifact $coverageArchive -DeploymentName $testConfiguration
        Push-AppveyorArtifact $coberturaFile -DeploymentName $testConfiguration
    }
}

Write-Host -ForegroundColor White ("Testing: " + $testPath)

(& $testPath --gtest_list_tests) | Out-String -Stream | Read-Test-Case | Create-AppVeyor-Test

if($LASTEXITCODE -ne 0)
{
    throw "Failed to discover test cases."
}

(& $testPath) | Out-String -Stream | Read-Test-Status | Report-AppVeyor-Test-Status

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("TestRun failed, ExitCode: {0}.", $LASTEXITCODE)
}

if(($env:platform -eq "x64") -and ($env:configuration -eq "Debug"))
{
   Run-CodeCoverage
}

