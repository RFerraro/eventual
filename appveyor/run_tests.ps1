$testStatusRegex = "^\[\s+(\w+)\s+\]\s+([\w\./]+)(?:\s\((\d+)[\w\s]+\))?$"
$testCaseLine = "^([\w/]+).+$"
$testNameLine = "^\s+(\w+)$"
$lastTestCase = "unknown"
$testExe = "test.exe"
$testPath = [System.IO.Path]::Combine($Env:APPVEYOR_BUILD_FOLDER, "bin", $Env:PLATFORM, $Env:Configuration, $testExe)


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

Write-Host -ForegroundColor White ("Testing: " + $testPath)

(& $testPath --gtest_list_tests) | Out-String -Stream | Read-Test-Case | Create-AppVeyor-Test
(& $testPath) | Out-String -Stream | Read-Test-Status | Report-AppVeyor-Test-Status
