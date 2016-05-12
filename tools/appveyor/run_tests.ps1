$testStatusRegex = "^\[\s+(\w+)\s+\]\s+([\w\./]+)(?:\s\((\d+)[\w\s]+\))?$"
$testCaseLine = "^([\w/]+).+$"
$testNameLine = "^\s+(\w+)$"
$lastTestCase = "unknown"
$testExe = "test.exe"
$testPath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "bin", $Env:Configuration, $testExe)
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

function Discover-Tests
{
    PROCESS
    {
        (& $testPath --gtest_list_tests) | Write-Output

        if($LASTEXITCODE -ne 0)
        {
            throw "Failed to discover test cases."
        }
    }

}

function Run-Tests
{
    [CmdletBinding()]
    param([Parameter()] [Switch]$WithCodeCoverage)
    
    PROCESS
    {
        if($WithCodeCoverage)
        {
            Write-Host -ForegroundColor White ("Running Code Coverage For: " + $testPath)

            (& OpenCppCoverage.exe --quiet --export_type html:$testCoveragePath --export_type cobertura:$coberturaFile --modules $testPath --sources $includePath -- $testPath) | Write-Output

            if($LASTEXITCODE -ne 0)
            {
                throw [System.String]::Format("Failed to run tests to generate code coverage, ExitCode: {0}.", $LASTEXITCODE)
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
        else
        {
            (& $testPath) | Write-Output

            if($LASTEXITCODE -ne 0)
            {
                throw [System.String]::Format("TestRun failed, ExitCode: {0}.", $LASTEXITCODE)
            }
        }
    }
}

Write-Host -ForegroundColor White ("Testing: " + $testPath)

Discover-Tests | Out-String -Stream | Read-Test-Case | Create-AppVeyor-Test

if($env:CONFIGURATION -eq "Debug")
{
    Run-Tests -WithCodeCoverage | Out-String -Stream | Read-Test-Status | Report-AppVeyor-Test-Status
}
else
{
    Run-Tests | Out-String -Stream | Read-Test-Status | Report-AppVeyor-Test-Status
}
